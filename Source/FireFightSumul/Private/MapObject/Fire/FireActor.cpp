#include "MapObject/Fire/FireActor.h"

#include "AimTraceChannel.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "MapObject/Fire/FireManager.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr float SmokeOnlyEmitterCutoff = 0.25f;
	const FName SmokeEmitterName(TEXT("Smoke"));
	const FName NonSmokeEmitterNames[] =
	{
		TEXT("Fire"),
		TEXT("Fire_Top"),
		TEXT("Blow_ember"),
		TEXT("Blow_ember2"),
		TEXT("Distortion")
	};

	const FName WindowHealthPropertyName(TEXT("Health"));
	const FName WindowFireDpsPropertyName(TEXT("FireDPS"));
	const FName WindowBrokenPropertyName(TEXT("bBroken"));
	const FName BreakWindowFunctionName(TEXT("BreakWindow"));

	FNumericProperty* FindFloatingPointProperty(const UClass* Class, const FName PropertyName)
	{
		FNumericProperty* Property = FindFProperty<FNumericProperty>(Class, PropertyName);
		return Property && Property->IsFloatingPoint() ? Property : nullptr;
	}

	bool ReadFloatingPointProperty(const UObject& Object, const FName PropertyName, double& OutValue)
	{
		const FNumericProperty* Property = FindFloatingPointProperty(Object.GetClass(), PropertyName);
		if (!Property)
		{
			return false;
		}

		const void* ValueAddress = Property->ContainerPtrToValuePtr<void>(&Object);
		OutValue = Property->GetFloatingPointPropertyValue(ValueAddress);
		return true;
	}

	bool WriteFloatingPointProperty(UObject& Object, const FName PropertyName, const double Value)
	{
		FNumericProperty* Property = FindFloatingPointProperty(Object.GetClass(), PropertyName);
		if (!Property)
		{
			return false;
		}

		void* ValueAddress = Property->ContainerPtrToValuePtr<void>(&Object);
		Property->SetFloatingPointPropertyValue(ValueAddress, Value);
		return true;
	}
}

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

	// AimSphere remains dedicated to aiming. HeatCollision is independent from
	// UFireManager's aim-collision debug toggle.
	HeatCollision = CreateDefaultSubobject<USphereComponent>(TEXT("HeatCollision"));
	HeatCollision->SetupAttachment(GetRootComponent());
	HeatCollision->SetRelativeLocation(FVector(0.0, 0.0, 50.0));
	HeatCollision->InitSphereRadius(50.0f);
	HeatCollision->SetCollisionObjectType(ECC_WorldDynamic);
	HeatCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeatCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	HeatCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	HeatCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	HeatCollision->SetGenerateOverlapEvents(true);
	HeatCollision->OnComponentBeginOverlap.AddDynamic(
		this, &AFireActor::HandleHeatOverlapBegin);
	HeatCollision->OnComponentEndOverlap.AddDynamic(
		this, &AFireActor::HandleHeatOverlapEnd);

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
	GetWorld()->GetTimerManager().SetTimer(HeatDamageTimer, this,
		&AFireActor::ApplyHeatDamage, FMath::Max(0.02f, HeatDamageInterval), true);
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
		World->GetTimerManager().ClearTimer(HeatDamageTimer);
		World->GetTimerManager().ClearTimer(ReignitionTimer);
	}
	HeatedWindows.Reset();

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

void AFireActor::HandleHeatOverlapBegin(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsWindowHeatOverlap(OtherActor, OtherComp))
	{
		HeatedWindows.Add(OtherActor);
	}
}

void AFireActor::HandleHeatOverlapEnd(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherComp && OtherComp->GetName().StartsWith(TEXT("HeatSphere")))
	{
		HeatedWindows.Remove(OtherActor);
	}
}

bool AFireActor::IsWindowHeatOverlap(const AActor* OtherActor,
	const UPrimitiveComponent* OtherComp) const
{
	if (!OtherActor || OtherActor == this || !OtherComp ||
		!OtherComp->GetName().StartsWith(TEXT("HeatSphere")))
	{
		return false;
	}

	return FindFloatingPointProperty(OtherActor->GetClass(), WindowHealthPropertyName) &&
		OtherActor->FindFunction(BreakWindowFunctionName);
}

void AFireActor::RefreshHeatedWindows()
{
	if (!HeatCollision || !GetWorld() || CurrentIntensity <= 0.0f)
	{
		HeatedWindows.Reset();
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FireWindowHeatOverlap), false, this);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, HeatCollision->GetComponentLocation(),
		FQuat::Identity, ObjectQueryParams,
		FCollisionShape::MakeSphere(HeatCollision->GetScaledSphereRadius()), QueryParams);

	TSet<TWeakObjectPtr<AActor>> DetectedWindows;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OtherActor = Overlap.GetActor();
		UPrimitiveComponent* OtherComponent = Overlap.GetComponent();
		if (IsWindowHeatOverlap(OtherActor, OtherComponent))
		{
			DetectedWindows.Add(OtherActor);
			if (!HeatedWindows.Contains(OtherActor))
			{
				UE_LOG(LogTemp, Log, TEXT("Fire heat overlap started: Fire='%s', Window='%s'."),
					*GetNameSafe(this), *GetNameSafe(OtherActor));
			}
		}
	}

	HeatedWindows = MoveTemp(DetectedWindows);
}

void AFireActor::SetHeatCollisionEnabled(bool bEnabled)
{
	const ECollisionEnabled::Type DesiredState = bEnabled
		? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision;
	if (HeatCollision->GetCollisionEnabled() == DesiredState)
	{
		return;
	}

	HeatCollision->SetCollisionEnabled(DesiredState);
	if (!bEnabled)
	{
		HeatedWindows.Reset();
	}
}

void AFireActor::ApplyHeatDamage()
{
	RefreshHeatedWindows();
	if (CurrentIntensity <= 0.0f || HeatedWindows.IsEmpty())
	{
		return;
	}

	const float DamageInterval = FMath::Max(0.02f, HeatDamageInterval);
	for (auto It = HeatedWindows.CreateIterator(); It; ++It)
	{
		AActor* Window = It->Get();
		if (!IsValid(Window))
		{
			It.RemoveCurrent();
			continue;
		}

		if (const FBoolProperty* BrokenProperty =
			FindFProperty<FBoolProperty>(Window->GetClass(), WindowBrokenPropertyName);
			BrokenProperty && BrokenProperty->GetPropertyValue_InContainer(Window))
		{
			It.RemoveCurrent();
			continue;
		}

		UFunction* BreakWindowFunction = Window->FindFunction(BreakWindowFunctionName);
		double Health = 0.0;
		if (!ReadFloatingPointProperty(*Window, WindowHealthPropertyName, Health) ||
			!BreakWindowFunction)
		{
			It.RemoveCurrent();
			continue;
		}

		double DamagePerSecond = DefaultWindowFireDamagePerSecond;
		double ConfiguredDamage = 0.0;
		if (ReadFloatingPointProperty(*Window, WindowFireDpsPropertyName, ConfiguredDamage))
		{
			if (FMath::IsFinite(ConfiguredDamage) && ConfiguredDamage > 0.0f)
			{
				DamagePerSecond = ConfiguredDamage;
			}
		}

		Health = FMath::Max(0.0, Health - DamagePerSecond * DamageInterval);
		WriteFloatingPointProperty(*Window, WindowHealthPropertyName, Health);
		if (Health <= 0.0f)
		{
			It.RemoveCurrent();
			UE_LOG(LogTemp, Log, TEXT("Fire heat depleted window health; calling BreakWindow on '%s'."),
				*GetNameSafe(Window));
			Window->ProcessEvent(BreakWindowFunction, nullptr);
		}
	}
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
	const bool bWasBurning = CurrentIntensity > 0.0f;
	const bool bWasSmokeOnly = bWasBurning &&
		GetIntensityRatio() <= SmokeOnlyEmitterCutoff;
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
		const bool bSmokeOnly = IntensityRatio <= SmokeOnlyEmitterCutoff;
		if (!bWasBurning || bWasSmokeOnly != bSmokeOnly)
		{
			FireEffect->SetEmitterEnable(SmokeEmitterName, true);
			for (const FName& EmitterName : NonSmokeEmitterNames)
			{
				FireEffect->SetEmitterEnable(EmitterName, !bSmokeOnly);
			}
		}

		const float EffectScale = FMath::Lerp(0.1f, 1.0f, IntensityRatio);
		FireEffect->SetRelativeScale3D(InitialEffectScale * EffectScale);
		if (CurrentIntensity >= IntensityLimit)
		{
			SetActorTickEnabled(false);
		}
	}
	SetHeatCollisionEnabled(CurrentIntensity > 0.0f);

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
