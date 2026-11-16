[CmdletBinding()]
param(
    [string]$EngineRoot,
    [switch]$CheckOnly,
    [switch]$NoLaunch
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

function Stop-Setup {
    param([string]$Message)

    Write-Host "ERROR: $Message" -ForegroundColor Red
    throw $Message
}

function Get-EngineVersion {
    param([string]$CandidateRoot)

    $versionFile = Join-Path $CandidateRoot 'Engine\Build\Build.version'
    if (-not (Test-Path -LiteralPath $versionFile -PathType Leaf)) {
        return $null
    }

    try {
        $version = Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json
        return [PSCustomObject]@{
            Root = (Resolve-Path -LiteralPath $CandidateRoot).Path
            Major = [int]$version.MajorVersion
            Minor = [int]$version.MinorVersion
            Patch = [int]$version.PatchVersion
            Changelist = [int64]$version.Changelist
        }
    }
    catch {
        return $null
    }
}

function Add-EngineCandidate {
    param(
        [System.Collections.Generic.List[string]]$Candidates,
        [string]$Candidate
    )

    if ([string]::IsNullOrWhiteSpace($Candidate)) {
        return
    }

    $expanded = [Environment]::ExpandEnvironmentVariables($Candidate.Trim('"'))
    if ((Test-Path -LiteralPath $expanded -PathType Container) -and -not $Candidates.Contains($expanded)) {
        $Candidates.Add($expanded)
    }
}

function Find-UnrealEngine {
    param([string]$PreferredRoot)

    $candidates = New-Object 'System.Collections.Generic.List[string]'
    Add-EngineCandidate -Candidates $candidates -Candidate $PreferredRoot

    $launcherData = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path -LiteralPath $launcherData -PathType Leaf) {
        try {
            $launcherJson = Get-Content -LiteralPath $launcherData -Raw | ConvertFrom-Json
            foreach ($installation in $launcherJson.InstallationList) {
                if ($installation.AppName -eq 'UE_5.5' -or $installation.ArtifactId -eq 'UE_5.5') {
                    Add-EngineCandidate -Candidates $candidates -Candidate $installation.InstallLocation
                }
            }
        }
        catch {
            Write-Warning "Could not read Epic Launcher installation data: $launcherData"
        }
    }

    $registryPath = 'HKCU:\Software\Epic Games\Unreal Engine\Builds'
    if (Test-Path -LiteralPath $registryPath) {
        try {
            $registryKey = Get-Item -LiteralPath $registryPath
            foreach ($valueName in $registryKey.GetValueNames()) {
                Add-EngineCandidate -Candidates $candidates -Candidate ([string]$registryKey.GetValue($valueName))
            }
        }
        catch {
            Write-Warning 'Could not inspect the Unreal Engine registry entries.'
        }
    }

    Add-EngineCandidate -Candidates $candidates -Candidate (Join-Path $env:ProgramFiles 'Epic Games\UE_5.5')
    Add-EngineCandidate -Candidates $candidates -Candidate 'C:\Epic Games\UE_5.5'
    Add-EngineCandidate -Candidates $candidates -Candidate 'C:\EpicGames\UE_5.5'

    $detected = @()
    foreach ($candidate in $candidates) {
        $version = Get-EngineVersion -CandidateRoot $candidate
        if ($null -ne $version) {
            $detected += $version
        }
    }

    $exact = $detected | Where-Object { $_.Major -eq 5 -and $_.Minor -eq 5 -and $_.Patch -eq 4 } | Select-Object -First 1
    if ($null -ne $exact) {
        return $exact
    }

    if ($detected.Count -gt 0) {
        $found = ($detected | ForEach-Object { "$($_.Major).$($_.Minor).$($_.Patch) at $($_.Root)" }) -join '; '
        Stop-Setup "Unreal Engine 5.5.4 is required. Detected: $found"
    }

    Stop-Setup 'Unreal Engine 5.5.4 was not found. Install it with Epic Games Launcher or rerun with -EngineRoot.'
}

function Find-VisualStudio {
    $vsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vsWhere -PathType Leaf)) {
        Stop-Setup 'Visual Studio Installer was not found. Open .vsconfig with Visual Studio Installer and install all requested components.'
    }

    $requiredComponents = @(
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
        'Microsoft.VisualStudio.Component.VC.14.38.17.8.x86.x64',
        'Microsoft.VisualStudio.Component.Windows11SDK.22621'
    )

    $arguments = @('-latest', '-products', '*', '-version', '[17.0,18.0)', '-property', 'installationPath')
    foreach ($component in $requiredComponents) {
        $arguments += @('-requires', $component)
    }

    $installationPath = (& $vsWhere @arguments | Select-Object -First 1)
    if ([string]::IsNullOrWhiteSpace($installationPath)) {
        Stop-Setup 'Visual Studio 2022 C++ components are incomplete. Open .vsconfig with Visual Studio Installer and install all requested components.'
    }

    $compiler = Join-Path $installationPath 'VC\Tools\MSVC\14.38.33130\bin\Hostx64\x64\cl.exe'
    if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) {
        Stop-Setup "MSVC 14.38.33130 was not found under: $installationPath"
    }

    $sdkInclude = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Include\10.0.22621.0'
    if (-not (Test-Path -LiteralPath $sdkInclude -PathType Container)) {
        Stop-Setup 'Windows SDK 10.0.22621.0 was not found. Install the Windows 11 SDK 22621 component from .vsconfig.'
    }

    return [PSCustomObject]@{
        Root = $installationPath
        Compiler = $compiler
        Sdk = $sdkInclude
    }
}

function Show-BuildErrors {
    param([string]$LogPath)

    if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf)) {
        return
    }

    Write-Host ''
    Write-Host 'First relevant errors:' -ForegroundColor Yellow
    Select-String -LiteralPath $LogPath -Pattern 'fatal error|error [A-Z]+[0-9]+|error:|BUILD FAILED' -CaseSensitive:$false |
        Select-Object -First 12 |
        ForEach-Object { Write-Host $_.Line -ForegroundColor Yellow }
}

try {
    $projectRoot = (Resolve-Path -LiteralPath $PSScriptRoot).Path
    $projectFile = Join-Path $projectRoot 'FireFightSumul.uproject'

    if (-not (Test-Path -LiteralPath $projectFile -PathType Leaf)) {
        Stop-Setup "FireFightSumul.uproject was not found in: $projectRoot"
    }

    if ($projectRoot -match '[^\x00-\x7F]') {
        Stop-Setup "The project path contains non-ASCII characters. Move the folder to C:\Work\FireFightSumul and run SetupProject.cmd again. Current path: $projectRoot"
    }

    if ($projectRoot.Length -gt 120) {
        Stop-Setup "The project path is too long ($($projectRoot.Length) characters). Move it to C:\Work\FireFightSumul."
    }

    $writeProbe = Join-Path $projectRoot ('.setup-write-test-' + [Guid]::NewGuid().ToString('N') + '.tmp')
    try {
        [IO.File]::WriteAllText($writeProbe, 'write test')
    }
    finally {
        if (Test-Path -LiteralPath $writeProbe) {
            Remove-Item -LiteralPath $writeProbe -Force
        }
    }

    $logsDirectory = Join-Path $projectRoot 'SetupLogs'
    New-Item -ItemType Directory -Path $logsDirectory -Force | Out-Null

    Write-Host 'Checking Unreal Engine 5.5.4...' -ForegroundColor Cyan
    $engine = Find-UnrealEngine -PreferredRoot $EngineRoot
    Write-Host "  Engine: $($engine.Root)"
    Write-Host "  Version: $($engine.Major).$($engine.Minor).$($engine.Patch) (CL $($engine.Changelist))"

    $editorExecutable = Join-Path $engine.Root 'Engine\Binaries\Win64\UnrealEditor.exe'
    if (-not (Test-Path -LiteralPath $editorExecutable -PathType Leaf)) {
        Stop-Setup "UnrealEditor.exe was not found: $editorExecutable"
    }

    $unrealBuildTool = Join-Path $engine.Root 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
    if (-not (Test-Path -LiteralPath $unrealBuildTool -PathType Leaf)) {
        Stop-Setup "UnrealBuildTool was not found: $unrealBuildTool"
    }

    $bundledDotNetRoot = Join-Path $engine.Root 'Engine\Binaries\ThirdParty\DotNet'
    $dotNetExecutable = Get-ChildItem -LiteralPath $bundledDotNetRoot -Filter 'dotnet.exe' -File -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '[\\/]win-x64[\\/]dotnet\.exe$' } |
        Sort-Object FullName -Descending |
        Select-Object -First 1
    if ($null -eq $dotNetExecutable) {
        Stop-Setup "The Unreal Engine bundled dotnet.exe was not found under: $bundledDotNetRoot"
    }

    Write-Host 'Checking Visual Studio 2022 components...' -ForegroundColor Cyan
    $visualStudio = Find-VisualStudio
    Write-Host "  Visual Studio: $($visualStudio.Root)"
    Write-Host "  Compiler: $($visualStudio.Compiler)"
    Write-Host "  SDK: $($visualStudio.Sdk)"

    if ($CheckOnly) {
        Write-Host 'Environment check completed successfully.' -ForegroundColor Green
        exit 0
    }

    $generateLog = Join-Path $logsDirectory 'GenerateProjectFiles.log'
    Write-Host 'Generating project files for this computer...' -ForegroundColor Cyan
    & $dotNetExecutable.FullName $unrealBuildTool '-ProjectFiles' "-Project=$projectFile" '-Game' '-Engine' '-Progress' "-Log=$generateLog"
    if ($LASTEXITCODE -ne 0) {
        Show-BuildErrors -LogPath $generateLog
        Stop-Setup "Project file generation failed with exit code $LASTEXITCODE. Log: $generateLog"
    }

    $buildLog = Join-Path $logsDirectory 'Build.log'
    Write-Host 'Building FireFightSumulEditor (first build can take several minutes)...' -ForegroundColor Cyan
    & $dotNetExecutable.FullName $unrealBuildTool 'FireFightSumulEditor' 'Win64' 'Development' "-Project=$projectFile" '-WaitMutex' '-NoHotReloadFromIDE' "-Log=$buildLog"
    if ($LASTEXITCODE -ne 0) {
        Show-BuildErrors -LogPath $buildLog
        Stop-Setup "Editor build failed with exit code $LASTEXITCODE. Log: $buildLog"
    }

    Write-Host 'Build completed successfully.' -ForegroundColor Green
    if (-not $NoLaunch) {
        Write-Host 'Launching Unreal Editor...' -ForegroundColor Cyan
        Start-Process -FilePath $editorExecutable -ArgumentList @($projectFile)
    }

    exit 0
}
catch {
    Write-Host ''
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
