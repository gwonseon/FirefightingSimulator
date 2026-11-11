#include "MapObject/Fire/FireAlarmActor.h"

#include "Components/AudioComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/FireScenarioDataAsset.h"
#include "Engine/World.h"
#include "MapObject/Fire/FireManager.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName AlarmTrashBinScenarioId(TEXT("TrashBin"));
	const FName AlarmKitchenPanScenarioId(TEXT("KitchenPan"));
	const FName AlarmOutsideScenarioId(TEXT("Outside"));
	const FName TrashAlarmTag(TEXT("Trash"));
	const FName OilAlarmTag(TEXT("Oil"));
	const FName OutsideAlarmTag(TEXT("Outside"));
}

AFireAlarmActor::AFireAlarmActor()
{
	PrimaryActorTick.bCanEverTick = false;

	AlarmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AlarmMesh"));
	AlarmMesh->SetupAttachment(GetRootComponent());
	AlarmMesh->SetRelativeScale3D(FVector(0.01f));

	AlarmAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("AlarmAudio"));
	AlarmAudio->SetupAttachment(GetRootComponent());
	AlarmAudio->SetAutoActivate(false);
	AlarmAudio->bAllowSpatialization = true;
	AlarmAudio->bOverrideAttenuation = true;
	AlarmAudio->bCanPlayMultipleInstances = false;
	AlarmAudio->AttenuationOverrides.bAttenuate = true;
	AlarmAudio->AttenuationOverrides.bSpatialize = true;
	AlarmAudio->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::Logarithmic;
	AlarmAudio->AttenuationOverrides.AttenuationShape = EAttenuationShape::Sphere;
	AlarmAudio->AttenuationOverrides.AttenuationShapeExtents = FVector(200.0f, 0.0f, 0.0f);
	AlarmAudio->AttenuationOverrides.FalloffDistance = 3800.0f;
	AlarmAudio->AttenuationOverrides.SpatializationAlgorithm =
		ESoundSpatializationAlgorithm::SPATIALIZATION_Default;

	AlarmLight = CreateDefaultSubobject<URectLightComponent>(TEXT("AlarmLight"));
	AlarmLight->SetupAttachment(GetRootComponent());
	AlarmLight->SetMobility(EComponentMobility::Movable);
	AlarmLight->SetRelativeLocation(FVector(8.0f, 0.0f, 0.0f));
	AlarmLight->SetLightColor(FLinearColor::Red);
	AlarmLight->SetIntensity(5000.0f);
	AlarmLight->SetAttenuationRadius(1500.0f);
	AlarmLight->SetSourceWidth(35.0f);
	AlarmLight->SetSourceHeight(35.0f);
	AlarmLight->SetCastShadows(false);
	AlarmLight->SetVisibility(false);

	static const ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultAlarmMesh(
		TEXT("/Game/_Model/scene.scene"));
	if (DefaultAlarmMesh.Succeeded())
	{
		AlarmMesh->SetStaticMesh(DefaultAlarmMesh.Object);
	}

	static const ConstructorHelpers::FObjectFinder<USoundBase> DefaultAlarmSound(
		TEXT("/Game/_Sound/SC_Alarm.SC_Alarm"));
	if (DefaultAlarmSound.Succeeded())
	{
		AlarmAudio->SetSound(DefaultAlarmSound.Object);
	}
}

void AFireAlarmActor::BeginPlay()
{
	Super::BeginPlay();
	AlarmAudio->OnAudioFinishedNative.AddUObject(
		this, &AFireAlarmActor::HandleAlarmFinished);

	if (UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>())
	{
		FireManager->OnScenarioStarted.AddUObject(this, &AFireAlarmActor::HandleScenarioStarted);

		// Actor BeginPlay order is not guaranteed. Play now if the director already
		// started the scenario before this alarm subscribed to the event.
		if (FireManager->GetScenarioState() == EFireScenarioState::Active)
		{
			HandleScenarioStarted();
		}
	}
}

void AFireAlarmActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LightFlashTimer);
		if (UFireManager* FireManager = World->GetSubsystem<UFireManager>())
		{
			FireManager->OnScenarioStarted.RemoveAll(this);
		}
	}
	AlarmAudio->OnAudioFinishedNative.RemoveAll(this);
	AlarmLight->SetVisibility(false);

	Super::EndPlay(EndPlayReason);
}

void AFireAlarmActor::HandleScenarioStarted()
{
	const UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>();
	const UFireScenarioDataAsset* ScenarioData = FireManager
		? FireManager->GetActiveScenarioData() : nullptr;
	if (ScenarioData && MatchesScenario(*ScenarioData))
	{
		PlayAlarmOnce();
	}
}

bool AFireAlarmActor::MatchesScenario(const UFireScenarioDataAsset& ScenarioData) const
{
	FName RequiredAlarmTag = NAME_None;
	if (ScenarioData.ScenarioId == AlarmTrashBinScenarioId)
	{
		RequiredAlarmTag = TrashAlarmTag;
	}
	else if (ScenarioData.ScenarioId == AlarmKitchenPanScenarioId)
	{
		RequiredAlarmTag = OilAlarmTag;
	}
	else if (ScenarioData.ScenarioId == AlarmOutsideScenarioId)
	{
		RequiredAlarmTag = OutsideAlarmTag;
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("FireAlarmActor '%s' has no alarm-tag mapping for scenario '%s'."),
			*GetName(), *ScenarioData.ScenarioId.ToString());
		return false;
	}

	return ActorHasTag(RequiredAlarmTag);
}

void AFireAlarmActor::PlayAlarmOnce()
{
	if (bHasPlayed)
	{
		return;
	}

	if (!AlarmAudio || !AlarmAudio->Sound)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("FireAlarmActor '%s' cannot play because AlarmAudio has no Sound asset."),
			*GetName());
		return;
	}

	bHasPlayed = true;
	AlarmAudio->Play();
	StartLightFlashing();
	UE_LOG(LogTemp, Log, TEXT("Fire alarm '%s' started."), *GetName());
}

void AFireAlarmActor::StartLightFlashing()
{
	bAlarmLightOn = true;
	AlarmLight->SetVisibility(true);
	GetWorldTimerManager().SetTimer(LightFlashTimer, this,
		&AFireAlarmActor::ToggleAlarmLight,
		FMath::Max(0.05f, LightFlashIntervalSeconds), true);
}

void AFireAlarmActor::StopLightFlashing()
{
	GetWorldTimerManager().ClearTimer(LightFlashTimer);
	bAlarmLightOn = false;
	AlarmLight->SetVisibility(false);
}

void AFireAlarmActor::ToggleAlarmLight()
{
	bAlarmLightOn = !bAlarmLightOn;
	AlarmLight->SetVisibility(bAlarmLightOn);
}

void AFireAlarmActor::HandleAlarmFinished(UAudioComponent* FinishedComponent)
{
	if (FinishedComponent == AlarmAudio)
	{
		StopLightFlashing();
	}
}
