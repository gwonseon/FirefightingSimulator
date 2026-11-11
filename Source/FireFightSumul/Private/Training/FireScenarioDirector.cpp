#include "Training/FireScenarioDirector.h"

#include "Components/SceneComponent.h"
#include "Data/FireScenarioDataAsset.h"
#include "Engine/World.h"
#include "Game/FireTrainingGameInstance.h"
#include "MapObject/Fire/FireManager.h"

AFireScenarioDirector::AFireScenarioDirector()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void AFireScenarioDirector::BeginPlay()
{
	Super::BeginPlay();

	UFireScenarioDataAsset* ScenarioToStart = ScenarioData;
	if (const UFireTrainingGameInstance* TrainingGameInstance =
		GetGameInstance<UFireTrainingGameInstance>())
	{
		if (UFireScenarioDataAsset* SelectedScenario = TrainingGameInstance->GetSelectedScenario())
		{
			ScenarioToStart = SelectedScenario;
		}
	}

	if (!ScenarioToStart)
	{
		UE_LOG(LogTemp, Error,
			TEXT("FireScenarioDirector '%s' has no selected scenario or assigned fallback scenario."),
			*GetName());
		return;
	}

	UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>();
	if (!FireManager || !FireManager->StartScenario(ScenarioToStart))
	{
		UE_LOG(LogTemp, Error, TEXT("FireScenarioDirector '%s' failed to start scenario '%s'."),
			*GetName(), *ScenarioToStart->ScenarioId.ToString());
	}
}
