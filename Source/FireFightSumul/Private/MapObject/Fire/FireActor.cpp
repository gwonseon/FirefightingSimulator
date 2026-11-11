#include "MapObject/Fire/FireActor.h"

#include "AimTraceChannel.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
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
	if (GetIntensityRatio() < 1.0f && GrowthMultiplier > UE_KINDA_SMALL_NUMBER)
	{
		BeginGrowth(GetWorld()->GetTimeSeconds());
	}
	else
	{
		SetActorTickEnabled(false);
	}
}

void AFireActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReignitionTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void AFireActor::SetInitialIntensityRatio(float Ratio)
{
	InitialIntensityRatio = FMath::Clamp(Ratio, 0.01f, 1.0f);
}

void AFireActor::ConfigureMaterial(FName InMaterialTag, EFireClass InFireClass,
	float InGrowthMultiplier, float InSuppressionMultiplier)
{
	MaterialTag = InMaterialTag;
	FireClass = InFireClass;
	GrowthMultiplier = FMath::Max(0.0f, InGrowthMultiplier);
	SuppressionMultiplier = FMath::Max(0.0f, InSuppressionMultiplier);
}

void AFireActor::SetGrowthToPeakSeconds(float Seconds)
{
	GrowthToPeakSeconds = FMath::Max(0.1f, Seconds) /
		FMath::Max(GrowthMultiplier, 0.01f);
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
	SetIntensity(CurrentIntensity - Amount * SuppressionMultiplier);
	if (CurrentIntensity > 0.0f && GrowthMultiplier > UE_KINDA_SMALL_NUMBER)
	{
		BeginGrowth(LastSuppressionTime + FMath::Max(0.0f, RecoveryDelay));
	}
}

void AFireActor::ApplySuppressionRatio(float RatioAmount)
{
	if (!FMath::IsFinite(RatioAmount) || RatioAmount <= 0.0f)
	{
		return;
	}

	ApplySuppression(FMath::Max(MaxIntensity, 1.0f) * RatioAmount);
}

bool AFireActor::ReigniteFromNearbyFire(float StartIntensityRatio)
{
	if (CurrentIntensity > 0.0f || !GetWorld())
	{
		return false;
	}

	GetWorld()->GetTimerManager().ClearTimer(ReignitionTimer);
	const float StartRatio = FMath::Clamp(StartIntensityRatio, 0.01f, 1.0f);
	SetIntensity(FMath::Max(1.0f, MaxIntensity) * StartRatio);
	BeginGrowth(GetWorld()->GetTimeSeconds());
	if (UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>())
	{
		FireManager->NotifyFireReignited(*this);
	}

	UE_LOG(LogTemp, Log, TEXT("Fire %s reignited from nearby fire at %.1f%% intensity."),
		*GetName(), GetIntensityRatio() * 100.0f);
	return true;
}

void AFireActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const double TimeSinceRecoveryStart = GetWorld()->GetTimeSeconds() - RecoveryStartTime;
	if (TimeSinceRecoveryStart <= 0.0)
	{
		return;
	}

	const float GrowthAlpha = FMath::Clamp(
		static_cast<float>(TimeSinceRecoveryStart) / FMath::Max(0.1f, GrowthToPeakSeconds),
		0.0f, 1.0f);
	const float StartRoot = FMath::Sqrt(FMath::Clamp(GrowthStartIntensityRatio, 0.0f, 1.0f));
	const float NewIntensityRatio = FMath::Square(FMath::Lerp(StartRoot, 1.0f, GrowthAlpha));
	SetIntensity(FMath::Max(1.0f, MaxIntensity) * NewIntensityRatio);
}

void AFireActor::BeginGrowth(double StartTime)
{
	GrowthStartIntensityRatio = GetIntensityRatio();
	RecoveryStartTime = StartTime;
	SetActorTickEnabled(CurrentIntensity > 0.0f && GrowthStartIntensityRatio < 1.0f &&
		GrowthMultiplier > UE_KINDA_SMALL_NUMBER);
}

void AFireActor::SetIntensity(float NewIntensity)
{
	constexpr float FireEmitterCutoff = 0.0f;
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
		if (!bWasBurning)
		{
			GetWorld()->GetTimerManager().ClearTimer(ReignitionTimer);
			FireEffect->Activate(true);
			if (const UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>())
			{
				SetAimCollisionEnabled(FireManager->IsFireCollisionEnabled());
			}
			else
			{
				SetAimCollisionEnabled(true);
			}
		}

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
		ScheduleReignition();
		if (UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>())
		{
			FireManager->NotifyFireExtinguished(*this);
		}
	}
}

void AFireActor::ScheduleReignition()
{
	if (const UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>())
	{
		if (!FireManager->CanScheduleReignition(FireClass, ReignitionChance))
		{
			return;
		}
	}
	else if (ReignitionChance <= 0.0f)
	{
		return;
	}

	const UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>();
	const float Delay = FireManager
		? FireManager->ResolveReignitionDelay(FireClass, ReignitionDelay)
		: FMath::Max(0.1f, ReignitionDelay);
	GetWorld()->GetTimerManager().SetTimer(ReignitionTimer, this,
		&AFireActor::TryReignite, Delay, false);
}

void AFireActor::TryReignite()
{
	if (CurrentIntensity > 0.0f)
	{
		return;
	}

	UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>();
	if (FireManager)
	{
		const EFireScenarioState ScenarioState = FireManager->GetScenarioState();
		const bool bForcedReignition = FireManager->IsForcedReignition(FireClass);
		if (FireManager->HasFinishedTraining() ||
			(ScenarioState == EFireScenarioState::EvacuationRequired && !bForcedReignition) ||
			ScenarioState == EFireScenarioState::Completed)
		{
			return;
		}
	}

	const bool bShouldReignite = FireManager
		? FireManager->ShouldReignite(FireClass, ReignitionChance)
		: FMath::FRand() < FMath::Clamp(ReignitionChance, 0.0f, 1.0f);
	if (!bShouldReignite)
	{
		UE_LOG(LogTemp, Log, TEXT("Fire %s did not reignite."), *GetName());
		return;
	}

	const float StartRatio = FireManager
		? FireManager->ResolveReignitionStartRatio(FireClass, ReignitionStartRatio)
		: FMath::Clamp(ReignitionStartRatio, 0.01f, 1.0f);
	const float StartIntensity = FMath::Max(1.0f, MaxIntensity) * StartRatio;
	SetIntensity(StartIntensity);
	BeginGrowth(GetWorld()->GetTimeSeconds());
	if (FireManager)
	{
		FireManager->NotifyFireReignited(*this);
	}

	UE_LOG(LogTemp, Log, TEXT("Fire %s reignited at %.1f%% intensity."),
		*GetName(), GetIntensityRatio() * 100.0f);
}
