#include "MapObject/Fire/FireActor.h"

#include "AimTraceChannel.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "MapObject/Fire/FireManager.h"
#include "MapObject/Fire/FireManager.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

const FName AFireActor::AimTag(TEXT("Fire"));

AFireActor::AFireActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	FireEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireEffect"));
	FireEffect->SetupAttachment(GetRootComponent());
	FireEffect->SetAutoActivate(false);

	AimSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AimSphere"));
	AimSphere->SetupAttachment(GetRootComponent());
	AimSphere->SetRelativeLocation(FVector(0.0, 0.0, 50.0));
	AimSphere->InitSphereRadius(50.0f);
	AimSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AimSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AimSphere->SetCollisionResponseToChannel(AimTraceChannel, ECR_Block);
	AimSphere->SetGenerateOverlapEvents(false);

	static const ConstructorHelpers::FObjectFinder<UNiagaraSystem> DefaultFireEffect(
		TEXT("/Game/_Effect/NS_Fire.NS_Fire"));
	if (DefaultFireEffect.Succeeded())
	{
		FireEffect->SetAsset(DefaultFireEffect.Object);
	}
}

void AFireActor::BeginPlay()
{
	Tags.AddUnique(AimTag);
	Super::BeginPlay();

	CurrentIntensity = FMath::Max(1.0f, MaxIntensity);
	InitialEffectScale = FireEffect->GetRelativeScale3D();
	FireEffect->Activate(true);
	RecoveryStartTime = GetWorld()->GetTimeSeconds();
	SetIntensity(CurrentIntensity * InitialIntensityRatio);
	if (const UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>())
	{
		SetAimCollisionEnabled(FireManager->IsFireCollisionEnabled());
	}
	SetActorTickEnabled(InitialIntensityRatio < 1.0f && RecoveryRate > 0.0f);
}

void AFireActor::SetInitialIntensityRatio(float Ratio)
{
	InitialIntensityRatio = FMath::Clamp(Ratio, 0.01f, 1.0f);
}

float AFireActor::GetIntensityRatio() const
{
	return CurrentIntensity / FMath::Max(1.0f, MaxIntensity);
}

void AFireActor::SetAimCollisionEnabled(bool bEnabled)
{
	AimSphere->SetCollisionEnabled(bEnabled && CurrentIntensity > 0.0f
		? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void AFireActor::ApplySuppression(float Amount)
{
	if (!FMath::IsFinite(Amount) || Amount <= 0.0f || CurrentIntensity <= 0.0f)
	{
		return;
	}

	LastSuppressionTime = GetWorld()->GetTimeSeconds();
	RecoveryStartTime = LastSuppressionTime + FMath::Max(0.0f, RecoveryDelay);
	SetIntensity(CurrentIntensity - Amount);
	if (CurrentIntensity > 0.0f && RecoveryRate > 0.0f)
	{
		SetActorTickEnabled(true);
	}
}

void AFireActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const double TimeSinceRecoveryStart = GetWorld()->GetTimeSeconds() - RecoveryStartTime;
	const float RecoveryTime = FMath::Clamp(static_cast<float>(TimeSinceRecoveryStart), 0.0f, DeltaSeconds);
	if (RecoveryTime > 0.0f)
	{
		SetIntensity(CurrentIntensity + RecoveryRate * RecoveryTime);
	}
}

void AFireActor::SetIntensity(float NewIntensity)
{
	constexpr float FireEmitterCutoff = 0.25f;
	const bool bWasBurning = CurrentIntensity > 0.0f;
	const bool bWasFireEmitterEnabled = GetIntensityRatio() > FireEmitterCutoff;
	const float IntensityLimit = FMath::Max(1.0f, MaxIntensity);
	CurrentIntensity = FMath::Clamp(NewIntensity, 0.0f, IntensityLimit);
	if (CurrentIntensity <= UE_KINDA_SMALL_NUMBER)
	{
		CurrentIntensity = 0.0f;
		SetActorTickEnabled(false);
		SetAimCollisionEnabled(false);
		FireEffect->DeactivateImmediate();
	}
	else
	{
		const float IntensityRatio = GetIntensityRatio();
		const bool bEnableFireEmitter = IntensityRatio > FireEmitterCutoff;
		if (bWasFireEmitterEnabled != bEnableFireEmitter)
		{
			static const FName FireEmitterName(TEXT("Fire"));
			FireEffect->SetEmitterEnable(FireEmitterName, bEnableFireEmitter);
		}

		const float EffectScale = FMath::Lerp(0.1f, 1.0f, IntensityRatio);
		FireEffect->SetRelativeScale3D(InitialEffectScale * EffectScale);
		if (CurrentIntensity >= IntensityLimit)
		{
			SetActorTickEnabled(false);
		}
	}

	OnIntensityChanged.Broadcast(GetIntensityRatio());
	if (bWasBurning && CurrentIntensity == 0.0f)
	{
		if (UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>())
		{
			FireManager->NotifyFireExtinguished();
		}
	}
}
