#include "Training/FireScenarioDirector.h"

#include "Components/SceneComponent.h"
#include "Data/FireScenarioDataAsset.h"
#include "Engine/World.h"
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

	if (!ScenarioData)
	{
		UE_LOG(LogTemp, Error,
			TEXT("FireScenarioDirector '%s' has no assigned scenario."),
			*GetName());
		return;
	}

	UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>();
	if (!FireManager || !FireManager->StartScenario(ScenarioData))
	{
		UE_LOG(LogTemp, Error, TEXT("FireScenarioDirector '%s' failed to start scenario '%s'."),
			*GetName(), *ScenarioData->ScenarioId.ToString());
	}
}
