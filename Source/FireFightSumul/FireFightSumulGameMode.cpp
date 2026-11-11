// Copyright Epic Games, Inc. All Rights Reserved.

#include "FireFightSumulGameMode.h"

#include "Data/FireScenarioDataAsset.h"
#include "EngineUtils.h"
#include "Game/FireTrainingGameInstance.h"
#include "GameFramework/PlayerStart.h"
#include "Player/FireFighter.h"

AFireFightSumulGameMode::AFireFightSumulGameMode()
{
	DefaultPawnClass = AFireFighter::StaticClass();
}

AActor* AFireFightSumulGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const UFireTrainingGameInstance* TrainingGameInstance =
		GetGameInstance<UFireTrainingGameInstance>();
	const UFireScenarioDataAsset* SelectedScenario =
		TrainingGameInstance ? TrainingGameInstance->GetSelectedScenario() : nullptr;

	if (!IsValid(SelectedScenario) || SelectedScenario->PlayerStartTag.IsNone())
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	APlayerStart* SelectedStart = nullptr;
	int32 MatchingStartCount = 0;
	for (TActorIterator<APlayerStart> StartIt(GetWorld()); StartIt; ++StartIt)
	{
		APlayerStart* Candidate = *StartIt;
		if (!IsValid(Candidate) || Candidate->PlayerStartTag != SelectedScenario->PlayerStartTag)
		{
			continue;
		}

		++MatchingStartCount;
		if (!SelectedStart || Candidate->GetFName().LexicalLess(SelectedStart->GetFName()))
		{
			SelectedStart = Candidate;
		}
	}

	if (!SelectedStart)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Scenario '%s' found no PlayerStart with Player Start Tag '%s'. Using the default PlayerStart selection."),
			*SelectedScenario->ScenarioId.ToString(), *SelectedScenario->PlayerStartTag.ToString());
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	if (MatchingStartCount > 1)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Scenario '%s' found %d PlayerStarts with Player Start Tag '%s'. Using '%s'. Leave only one matching PlayerStart."),
			*SelectedScenario->ScenarioId.ToString(), MatchingStartCount,
			*SelectedScenario->PlayerStartTag.ToString(), *SelectedStart->GetName());
	}

	UE_LOG(LogTemp, Log, TEXT("Scenario '%s' will spawn at PlayerStart '%s' (Tag=%s)."),
		*SelectedScenario->ScenarioId.ToString(), *SelectedStart->GetName(),
		*SelectedScenario->PlayerStartTag.ToString());
	return SelectedStart;
}
