[CmdletBinding()]
param(
    [string]$OutputDirectory,
    [switch]$IncludeArtSource,
    [switch]$IncludeTools
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

function Add-PackageFile {
    param(
        [System.Collections.Generic.List[object]]$Files,
        [string]$ProjectRoot,
        [string]$RelativePath
    )

    $fullPath = Join-Path $ProjectRoot $RelativePath
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        throw "Required package file is missing: $RelativePath"
    }

    $Files.Add([PSCustomObject]@{
        FullPath = (Resolve-Path -LiteralPath $fullPath).Path
        RelativePath = $RelativePath.Replace('\', '/')
    })
}

function Add-PackageTree {
    param(
        [System.Collections.Generic.List[object]]$Files,
        [string]$ProjectRoot,
        [string]$RelativeDirectory
    )

    $fullDirectory = Join-Path $ProjectRoot $RelativeDirectory
    if (-not (Test-Path -LiteralPath $fullDirectory -PathType Container)) {
        throw "Required package directory is missing: $RelativeDirectory"
    }

    foreach ($file in Get-ChildItem -LiteralPath $fullDirectory -Recurse -File -Force) {
        if (($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Reparse points are not allowed in the package: $($file.FullName)"
        }

        $relative = $file.FullName.Substring($ProjectRoot.Length).TrimStart('\', '/')
        $Files.Add([PSCustomObject]@{
            FullPath = $file.FullName
            RelativePath = $relative.Replace('\', '/')
        })
    }
}

function Test-ContentAssets {
    param([string]$ContentRoot)

    $assets = @(Get-ChildItem -LiteralPath $ContentRoot -Recurse -File | Where-Object { $_.Extension -in '.uasset', '.umap' })
    $zeroLength = @($assets | Where-Object Length -eq 0)
    if ($zeroLength.Count -gt 0) {
        throw "Found zero-length Unreal assets: $($zeroLength[0].FullName)"
    }

    $pointers = New-Object 'System.Collections.Generic.List[string]'
    foreach ($asset in $assets | Where-Object Length -lt 1024) {
        try {
            $text = [IO.File]::ReadAllText($asset.FullName)
            if ($text.StartsWith('version https://git-lfs.github.com/spec/v1')) {
                $pointers.Add($asset.FullName)
            }
        }
        catch {
            # Small binary assets are valid; only readable LFS pointer text matters here.
        }
    }

    if ($pointers.Count -gt 0) {
        throw "Git LFS pointer files remain in Content. Run git lfs pull first. Example: $($pointers[0])"
    }

    return [PSCustomObject]@{
        AssetCount = $assets.Count
        AssetBytes = ($assets | Measure-Object -Property Length -Sum).Sum
        PointerCount = $pointers.Count
    }
}

$projectRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $projectRoot 'VendorPackages'
}
elseif (-not [IO.Path]::IsPathRooted($OutputDirectory)) {
    $OutputDirectory = Join-Path $projectRoot $OutputDirectory
}

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$resolvedOutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path

$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$zipName = "FireFightSumul_Source_UE5.5.4_$timestamp.zip"
$zipPath = Join-Path $resolvedOutputDirectory $zipName
$hashPath = "$zipPath.sha256.txt"

$assetStatus = Test-ContentAssets -ContentRoot (Join-Path $projectRoot 'Content')
Write-Host "Validated $($assetStatus.AssetCount) Unreal assets; no LFS pointers or empty assets." -ForegroundColor Green

$files = New-Object 'System.Collections.Generic.List[object]'
Add-PackageTree -Files $files -ProjectRoot $projectRoot -RelativeDirectory 'Config'
Add-PackageTree -Files $files -ProjectRoot $projectRoot -RelativeDirectory 'Content'
Add-PackageTree -Files $files -ProjectRoot $projectRoot -RelativeDirectory 'Source'

$pipelineCache = 'Build\Windows\PipelineCaches\FireFightSumul_PCD3D_SM6.spc'
if (Test-Path -LiteralPath (Join-Path $projectRoot $pipelineCache) -PathType Leaf) {
    Add-PackageFile -Files $files -ProjectRoot $projectRoot -RelativePath $pipelineCache
}

if ($IncludeArtSource) {
    Add-PackageTree -Files $files -ProjectRoot $projectRoot -RelativeDirectory 'ArtSource'
}
if ($IncludeTools) {
    Add-PackageTree -Files $files -ProjectRoot $projectRoot -RelativeDirectory 'Tools'
}

$topLevelFiles = @(
    'FireFightSumul.uproject',
    '.vsconfig',
    '.gitattributes',
    '.gitignore',
    'README_VENDOR.md',
    'SetupProject.cmd',
    'SetupProject.ps1',
    'CreateVendorPackage.ps1'
)
foreach ($relativePath in $topLevelFiles) {
    Add-PackageFile -Files $files -ProjectRoot $projectRoot -RelativePath $relativePath
}

$files = @($files | Sort-Object RelativePath -Unique)
if (Test-Path -LiteralPath $zipPath) {
    throw "Refusing to overwrite an existing package: $zipPath"
}

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$archive = [IO.Compression.ZipFile]::Open($zipPath, [IO.Compression.ZipArchiveMode]::Create)
try {
    for ($index = 0; $index -lt $files.Count; $index++) {
        $file = $files[$index]
        $entryName = 'FireFightSumul/' + $file.RelativePath
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $archive,
            $file.FullPath,
            $entryName,
            [IO.Compression.CompressionLevel]::Optimal
        ) | Out-Null

        if (($index + 1) % 25 -eq 0 -or $index -eq $files.Count - 1) {
            $percent = [int](100 * ($index + 1) / $files.Count)
            Write-Progress -Activity 'Creating vendor package' -Status "$($index + 1) / $($files.Count) files" -PercentComplete $percent
        }
    }

    $manifestEntry = $archive.CreateEntry('FireFightSumul/PACKAGE_MANIFEST.txt', [IO.Compression.CompressionLevel]::Optimal)
    $writer = New-Object IO.StreamWriter($manifestEntry.Open(), [Text.UTF8Encoding]::new($false))
    try {
        $writer.WriteLine('FireFightSumul vendor source package')
        $writer.WriteLine("Created: $((Get-Date).ToString('yyyy-MM-dd HH:mm:ss K'))")
        $writer.WriteLine('Required Unreal Engine: 5.5.4')
        $writer.WriteLine("Files: $($files.Count)")
        $writer.WriteLine("Unreal assets: $($assetStatus.AssetCount)")
        $writer.WriteLine("Unreal asset bytes: $($assetStatus.AssetBytes)")
        $writer.WriteLine('Git LFS pointers: 0')
        $writer.WriteLine('Snapshot source: current working directory, including uncommitted files')
    }
    finally {
        $writer.Dispose()
    }
}
catch {
    $archive.Dispose()
    if (Test-Path -LiteralPath $zipPath) {
        Remove-Item -LiteralPath $zipPath -Force
    }
    throw
}
finally {
    if ($null -ne $archive) {
        $archive.Dispose()
    }
    Write-Progress -Activity 'Creating vendor package' -Completed
}

$forbiddenPattern = '^FireFightSumul/(\.git|\.vs|\.idea|Binaries|DerivedDataCache|Intermediate|Saved)(/|$)|\.sln$'
$requiredEntries = @(
    'FireFightSumul/FireFightSumul.uproject',
    'FireFightSumul/.vsconfig',
    'FireFightSumul/SetupProject.cmd',
    'FireFightSumul/SetupProject.ps1',
    'FireFightSumul/README_VENDOR.md'
)

$readArchive = [IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $entryNames = @($readArchive.Entries | ForEach-Object FullName)
    $forbidden = @($entryNames | Where-Object { $_ -match $forbiddenPattern })
    if ($forbidden.Count -gt 0) {
        throw "Forbidden generated file entered the package: $($forbidden[0])"
    }
    foreach ($requiredEntry in $requiredEntries) {
        if ($entryNames -notcontains $requiredEntry) {
            throw "Package verification failed; missing: $requiredEntry"
        }
    }
}
finally {
    $readArchive.Dispose()
}

$hash = Get-FileHash -LiteralPath $zipPath -Algorithm SHA256
[IO.File]::WriteAllText($hashPath, "$($hash.Hash.ToLowerInvariant())  $zipName`r`n", [Text.UTF8Encoding]::new($false))

$zipInfo = Get-Item -LiteralPath $zipPath
Write-Host ''
Write-Host 'Vendor package created and verified.' -ForegroundColor Green
Write-Host "ZIP: $zipPath"
Write-Host "SHA256: $hashPath"
Write-Host ("Size: {0:N2} GiB" -f ($zipInfo.Length / 1GB))
Write-Host 'The ZIP contains no Binaries, Intermediate, Saved, IDE metadata, or solution files.'
