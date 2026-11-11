#include "Commandlets/FireTrainingStringTableImportCommandlet.h"

#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogFireTrainingStringTableImport, Log, All);

UFireTrainingStringTableImportCommandlet::UFireTrainingStringTableImportCommandlet(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 UFireTrainingStringTableImportCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamValues;
	ParseCommandLine(*Params, Tokens, Switches, ParamValues);

	FString CsvPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("_Data/ST_FireTraining.csv"));
	if (const FString* CsvParam = ParamValues.Find(TEXT("Csv")))
	{
		CsvPath = *CsvParam;
	}
	CsvPath = FPaths::ConvertRelativePathToFull(CsvPath);

	FString AssetPath = TEXT("/Game/_Data/ST_FireTraining.ST_FireTraining");
	if (const FString* AssetParam = ParamValues.Find(TEXT("Asset")))
	{
		AssetPath = *AssetParam;
	}

	if (!FPaths::FileExists(CsvPath))
	{
		UE_LOG(LogFireTrainingStringTableImport, Error, TEXT("CSV file was not found: %s"), *CsvPath);
		return 1;
	}

	UStringTable* StringTable = LoadObject<UStringTable>(nullptr, *AssetPath);
	if (!StringTable)
	{
		UE_LOG(LogFireTrainingStringTableImport, Error, TEXT("String Table asset was not found: %s"), *AssetPath);
		return 2;
	}

	StringTable->Modify();
	if (!StringTable->GetMutableStringTable()->ImportStrings(CsvPath))
	{
		UE_LOG(LogFireTrainingStringTableImport, Error, TEXT("Failed to import CSV: %s"), *CsvPath);
		return 3;
	}

	UPackage* Package = StringTable->GetOutermost();
	Package->GetMetaData();
	Package->MarkPackageDirty();
	const FString PackageFilename = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, StringTable, *PackageFilename, SaveArgs))
	{
		UE_LOG(LogFireTrainingStringTableImport, Error,
			TEXT("Imported the CSV but failed to save the String Table package: %s"), *PackageFilename);
		return 4;
	}

	UE_LOG(LogFireTrainingStringTableImport, Display,
		TEXT("Imported %s into %s and saved %s"), *CsvPath, *AssetPath, *PackageFilename);
	return 0;
}
