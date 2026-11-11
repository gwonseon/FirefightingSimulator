#include "Game/FireTrainingGameInstance.h"

#include "Data/FireScenarioDataAsset.h"
#include "Kismet/GameplayStatics.h"

namespace FireTrainingLevels
{
	const FName Start(TEXT("/Game/_Map/start"));
	const FName Select(TEXT("/Game/_Map/select"));
	const FName Main(TEXT("/Game/_Map/Main"));
}

bool UFireTrainingGameInstance::SelectScenario(UFireScenarioDataAsset* InScenario)
{
	if (!IsValid(InScenario))
	{
		UE_LOG(LogTemp, Warning, TEXT("SelectScenario was called without a valid scenario Data Asset."));
		return false;
	}

	SelectedScenario = InScenario;
	return true;
}

bool UFireTrainingGameInstance::StartSelectedScenario()
{
	if (!IsValid(SelectedScenario))
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot start training because no scenario is selected."));
		return false;
	}

	FName LevelName = FireTrainingLevels::Main;
	if (!SelectedScenario->ScenarioLevel.IsNull())
	{
		const FString PackageName = SelectedScenario->ScenarioLevel.ToSoftObjectPath().GetLongPackageName();
		if (!PackageName.IsEmpty())
		{
			LevelName = FName(*PackageName);
		}
	}

	UGameplayStatics::OpenLevel(this, LevelName);
	return true;
}

void UFireTrainingGameInstance::OpenScenarioSelect()
{
	UGameplayStatics::OpenLevel(this, FireTrainingLevels::Select);
}

void UFireTrainingGameInstance::OpenStartMenu()
{
	SelectedScenario = nullptr;
	UGameplayStatics::OpenLevel(this, FireTrainingLevels::Start);
}
