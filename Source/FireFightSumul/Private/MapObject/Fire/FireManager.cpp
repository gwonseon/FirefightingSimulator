#include "MapObject/Fire/FireManager.h"

#include "Data/FireScenarioDataAsset.h"
#include "Data/FireSimulationDataAsset.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/FireTrainingGameInstance.h"
#include "MapObject/Fire/FireActor.h"
#include "MapObject/Fire/FireMaterialTags.h"
#include "MapObject/Fire/FireSpreadAnchor.h"
#include "TimerManager.h"
#include "UI/TrainingResultGameMode.h"

namespace
{
	constexpr float InitialAnchorMatchDistanceCm = 100.0f;
	const FName TrashBinScenarioId(TEXT("TrashBin"));
	const FName TrainingStringTableId(TEXT("/Game/_Data/ST_FireTraining.ST_FireTraining"));

	FText ResolveResultMessage(ETrainingOutcome Outcome, ETrainingFailureReason FailureReason)
	{
		switch (Outcome)
		{
		case ETrainingOutcome::Extinguished:
			return FText::FromStringTable(TrainingStringTableId, TEXT("Result.Description.Extinguished"));
		case ETrainingOutcome::Evacuated:
			return FText::FromStringTable(TrainingStringTableId, TEXT("Result.Description.Evacuated"));
		case ETrainingOutcome::Failed:
			switch (FailureReason)
			{
			case ETrainingFailureReason::EnteredDangerZone:
				return FText::FromStringTable(TrainingStringTableId, TEXT("Result.FailedDangerZone"));
			case ETrainingFailureReason::IgnoredEvacuation:
				return FText::FromStringTable(TrainingStringTableId, TEXT("Result.FailedEvacuation"));
			case ETrainingFailureReason::ScenarioTimeout:
			default:
				return FText::FromStringTable(TrainingStringTableId, TEXT("Result.FailedTimeout"));
			}
		default:
			return FText::GetEmpty();
		}
	}
}

void UFireManager::ToggleFireCollision()
{
	bFireCollisionEnabled = !bFireCollisionEnabled;
	for (TActorIterator<AFireActor> Fire(&GetWorldRef()); Fire; ++Fire)
	{
		Fire->SetAimCollisionEnabled(bFireCollisionEnabled);
	}
	UE_LOG(LogTemp, Log, TEXT("Fire colliders: %s"), bFireCollisionEnabled ? TEXT("On") : TEXT("Off"));
}

void UFireManager::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	bTrainingWorld = false;
	bTrainingFinished = false;
	bScenarioStarted = false;
	bTrashBinGuaranteedReignitionUsed = false;
	ScenarioStartTimeSeconds = 0.0;
	TotalUsedAgentKg = 0.0f;
	ScenarioState = EFireScenarioState::None;
	ActiveScenarioData = nullptr;
}

bool UFireManager::StartScenario(UFireScenarioDataAsset* InScenarioData)
{
	if (!InScenarioData || !GetWorldRef().IsGameWorld())
	{
		return false;
	}

	if (bScenarioStarted)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Scenario '%s' is already active. Ignoring a second start request for '%s'."),
			*GetNameSafe(ActiveScenarioData), *GetNameSafe(InScenarioData));
		return false;
	}

	ATargetPoint* StartPoint = SelectStartPoint(*InScenarioData);
	if (!StartPoint)
	{
		return false;
	}

	ActiveScenarioData = InScenarioData;
	AnchorFires.Reset();
	bTrainingWorld = true;
	bTrainingFinished = false;
	bScenarioStarted = true;
	bTrashBinGuaranteedReignitionUsed = false;
	ScenarioStartTimeSeconds = 0.0;
	TotalUsedAgentKg = 0.0f;
	ScenarioState = EFireScenarioState::Ready;

	const float InitialIntensity = FMath::Clamp(InScenarioData->InitialFireIntensityRatio, 0.01f, 1.0f);
	FName InitialMaterialTag;
	if (!IgnitePoint(*StartPoint, InitialIntensity, InitialMaterialTag))
	{
		UE_LOG(LogTemp, Error, TEXT("Scenario '%s' could not spawn its initial fire at '%s'."),
			*InScenarioData->ScenarioId.ToString(), *GetNameSafe(StartPoint));
		ActiveScenarioData = nullptr;
		bTrainingWorld = false;
		bScenarioStarted = false;
		ScenarioState = EFireScenarioState::None;
		return false;
	}

	ScenarioState = EFireScenarioState::Active;
	ScenarioStartTimeSeconds = GetWorldRef().GetTimeSeconds();
	const float ScenarioTimeoutSeconds = FMath::Max(0.0f, InScenarioData->ScenarioTimeoutSeconds);
	if (ScenarioTimeoutSeconds > 0.0f)
	{
		GetWorldRef().GetTimerManager().SetTimer(ScenarioTimeoutTimer, this,
			&UFireManager::HandleScenarioTimeout, ScenarioTimeoutSeconds, false);
	}
	InitializeSpreadSystem(StartPoint->GetActorLocation(), InitialMaterialTag);
	OnScenarioStarted.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("Started fire scenario '%s' at TargetPoint '%s' (Tag=%s, Mode=%s)."),
		*InScenarioData->ScenarioId.ToString(), *GetNameSafe(StartPoint),
		*InScenarioData->StartPointTag.ToString(),
		InScenarioData->StartSelectionMode == EFireStartSelectionMode::Random
			? TEXT("Random") : TEXT("Fixed"));
	return true;
}

bool UFireManager::CanScheduleReignition(EFireClass FireClass, float DefaultChance) const
{
	if (IsForcedReignition(FireClass))
	{
		return true;
	}

	if (IsTrashBinScenario())
	{
		return !bTrashBinGuaranteedReignitionUsed;
	}

	return DefaultChance > 0.0f;
}

bool UFireManager::ShouldReignite(EFireClass FireClass, float DefaultChance)
{
	if (IsForcedReignition(FireClass))
	{
		return true;
	}

	if (IsTrashBinScenario())
	{
		if (bTrashBinGuaranteedReignitionUsed)
		{
			return false;
		}

		bTrashBinGuaranteedReignitionUsed = true;
		UE_LOG(LogTemp, Log, TEXT("TrashBin scenario consumed its guaranteed reignition."));
		return true;
	}

	return FMath::FRand() < FMath::Clamp(DefaultChance, 0.0f, 1.0f);
}

bool UFireManager::IsForcedReignition(EFireClass FireClass) const
{
	return FireClass == EFireClass::K && ActiveScenarioData &&
		ActiveScenarioData->bForceClassKReignition;
}

float UFireManager::ResolveReignitionDelay(EFireClass FireClass, float DefaultDelay) const
{
	if (IsForcedReignition(FireClass))
	{
		return FMath::Max(0.1f, ActiveScenarioData->ClassKReignitionDelaySeconds);
	}

	return FMath::Max(0.1f, DefaultDelay);
}

float UFireManager::ResolveReignitionStartRatio(EFireClass FireClass, float DefaultRatio) const
{
	if (IsForcedReignition(FireClass))
	{
		return FMath::Clamp(ActiveScenarioData->ClassKReignitionStartRatio, 0.01f, 1.0f);
	}

	return FMath::Clamp(DefaultRatio, 0.01f, 1.0f);
}

bool UFireManager::IsTrashBinScenario() const
{
	return ActiveScenarioData && ActiveScenarioData->ScenarioId == TrashBinScenarioId;
}

bool UFireManager::IsEvacuationOnlyScenario() const
{
	return ActiveScenarioData &&
		ActiveScenarioData->CompletionMode == EFireScenarioCompletionMode::EvacuationOnly;
}

float UFireManager::ResolveGrowthToPeakSeconds() const
{
	const UFireSimulationDataAsset* FireRules = ActiveScenarioData
		? ActiveScenarioData->FireRules.Get() : nullptr;
	if (!FireRules)
	{
		return 20.0f;
	}

	EFireGrowthSpeedMode GrowthMode = EFireGrowthSpeedMode::Training;
	if (const UFireTrainingGameInstance* TrainingGameInstance =
		Cast<UFireTrainingGameInstance>(GetWorldRef().GetGameInstance()))
	{
		GrowthMode = TrainingGameInstance->GetFireGrowthSpeedMode();
	}

	const float GrowthSeconds = GrowthMode == EFireGrowthSpeedMode::Realistic
		? FireRules->RealisticGrowthToPeakSeconds
		: FireRules->TrainingGrowthToPeakSeconds;
	return FMath::Max(0.1f, GrowthSeconds);
}

ATargetPoint* UFireManager::SelectStartPoint(const UFireScenarioDataAsset& ScenarioData) const
{
	if (ScenarioData.StartPointTag.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("Scenario '%s' has no StartPointTag."),
			*ScenarioData.ScenarioId.ToString());
		return nullptr;
	}

	TArray<ATargetPoint*> Candidates;
	for (TActorIterator<ATargetPoint> Point(&GetWorldRef()); Point; ++Point)
	{
		if (Point->ActorHasTag(ScenarioData.StartPointTag))
		{
			Candidates.Add(*Point);
		}
	}

	if (Candidates.IsEmpty())
	{
		UE_LOG(LogTemp, Error,
			TEXT("Scenario '%s' found no TargetPoint with Actor Tag '%s'."),
			*ScenarioData.ScenarioId.ToString(), *ScenarioData.StartPointTag.ToString());
		return nullptr;
	}

	if (ScenarioData.StartSelectionMode == EFireStartSelectionMode::Random)
	{
		return Candidates[FMath::RandHelper(Candidates.Num())];
	}

	ATargetPoint* SelectedPoint = Candidates[0];
	for (ATargetPoint* Candidate : Candidates)
	{
		if (Candidate && Candidate->GetName().Compare(SelectedPoint->GetName(),
			ESearchCase::IgnoreCase) < 0)
		{
			SelectedPoint = Candidate;
		}
	}

	if (Candidates.Num() > 1)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Fixed scenario '%s' found %d TargetPoints tagged '%s'. Using '%s'. Leave only one for a fixed start."),
			*ScenarioData.ScenarioId.ToString(), Candidates.Num(),
			*ScenarioData.StartPointTag.ToString(), *GetNameSafe(SelectedPoint));
	}

	return SelectedPoint;
}

bool UFireManager::IgnitePoint(const ATargetPoint& Point, float InitialIntensityRatio,
	FName& OutMaterialTag)
{
	OutMaterialTag = NAME_None;
	const EFireMaterialTagResolveResult TagResult =
		FireMaterialTags::ResolveActorMaterialTag(Point, OutMaterialTag);
	if (TagResult == EFireMaterialTagResolveResult::Ambiguous)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Initial fire point '%s' has more than one FireMaterial tag."),
			*GetNameSafe(&Point));
		return false;
	}
	if (TagResult == EFireMaterialTagResolveResult::Missing)
	{
		const AFireSpreadAnchor* InitialAnchor = FindClosestSpreadAnchor(
			Point.GetActorLocation(), InitialAnchorMatchDistanceCm);
		OutMaterialTag = InitialAnchor ? InitialAnchor->GetMaterialTag() : NAME_None;

		if (!InitialAnchor)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Initial fire point '%s' has no FireMaterial Actor Tag or nearby legacy FireSpreadAnchor. Using Class A."),
				*GetNameSafe(&Point));
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Initial fire point '%s' has no FireMaterial Actor Tag. Using nearby legacy anchor material '%s'."),
				*GetNameSafe(&Point), *OutMaterialTag.ToString());
		}
	}

	OutMaterialTag = FireMaterialTags::Canonicalize(OutMaterialTag);
	AFireSpreadAnchor* InitialAnchor = FindClosestSpreadAnchor(
		Point.GetActorLocation(), InitialAnchorMatchDistanceCm, OutMaterialTag);
	return IgniteLocation(Point.GetActorLocation(), InitialIntensityRatio, OutMaterialTag,
		InitialAnchor);
}

bool UFireManager::IgniteLocation(const FVector& Location, float InitialIntensityRatio,
	FName MaterialTag, AFireSpreadAnchor* SourceAnchor)
{
	MaterialTag = FireMaterialTags::Canonicalize(MaterialTag);
	const FFireMaterialSetting* Material = FindMaterialSetting(MaterialTag);
	const FTransform Transform(FQuat::Identity, Location);
	AFireActor* Fire = GetWorldRef().SpawnActorDeferred<AFireActor>(AFireActor::StaticClass(), Transform);
	if (!Fire)
	{
		return false;
	}

	Fire->SetInitialIntensityRatio(InitialIntensityRatio);
	Fire->ConfigureMaterial(MaterialTag, ResolveFireClass(MaterialTag),
		Material ? Material->GrowthMultiplier : 1.0f,
		Material ? Material->SuppressionMultiplier : 1.0f);
	Fire->SetGrowthToPeakSeconds(ResolveGrowthToPeakSeconds());
	Fire->FinishSpawning(Transform);
	RegisterFire(*Fire);
	if (SourceAnchor)
	{
		AnchorFires.FindOrAdd(TWeakObjectPtr<AFireSpreadAnchor>(SourceAnchor)) = Fire;
	}
	return true;
}

void UFireManager::InitializeSpreadSystem(const FVector& InitialLocation, FName InitialMaterialTag)
{
	ResetSpreadSystem();
	InitialFireLocation = InitialLocation;

	const UFireSimulationDataAsset* FireRules = ActiveScenarioData
		? ActiveScenarioData->FireRules.Get() : nullptr;
	if (!FireRules)
	{
		UE_LOG(LogTemp, Warning, TEXT("Scenario '%s' has no FireRules. Fire spread is disabled."),
			*GetNameSafe(ActiveScenarioData));
		return;
	}

	TSet<FName> MissingMaterialTags;
	for (TActorIterator<AFireSpreadAnchor> Anchor(&GetWorldRef()); Anchor; ++Anchor)
	{
		Anchor->ResetSpreadState();
		SpreadAnchors.Add(*Anchor);

		if (!Anchor->GetMaterialTag().IsNone() && !FindMaterialSetting(Anchor->GetMaterialTag()))
		{
			MissingMaterialTags.Add(Anchor->GetMaterialTag());
		}
	}

	for (const FName MissingTag : MissingMaterialTags)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Fire spread MaterialTag '%s' has no matching entry in FireRules.MaterialSettings. Using fallback values."),
			*MissingTag.ToString());
	}

	if (AFireSpreadAnchor* ClosestInitialAnchor = FindClosestSpreadAnchor(
		InitialLocation, InitialAnchorMatchDistanceCm, InitialMaterialTag))
	{
		// This represents the object on which the Target Point already created the initial fire.
		ClosestInitialAnchor->MarkIgnited();
	}

	if (SpreadAnchors.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("No FireSpreadAnchor actors found. Fire spread is disabled for this level."));
		return;
	}

	const float CheckInterval = FMath::Max(0.05f, FireRules->SpreadCheckInterval);
	GetWorldRef().GetTimerManager().SetTimer(SpreadEvaluationTimer, this,
		&UFireManager::EvaluateFireSpread, CheckInterval, true, CheckInterval);
	UE_LOG(LogTemp, Log, TEXT("Fire spread started with %d anchors (interval %.2fs, range %.0fcm)."),
		SpreadAnchors.Num(), CheckInterval, FireRules->SpreadDistanceCm);
}

void UFireManager::EvaluateFireSpread()
{
	if (!bTrainingWorld || bTrainingFinished || !ActiveScenarioData)
	{
		return;
	}

	const UFireSimulationDataAsset* FireRules = ActiveScenarioData->FireRules.Get();
	if (!FireRules)
	{
		return;
	}

	TArray<AFireActor*> BurningFires;
	for (TActorIterator<AFireActor> Fire(&GetWorldRef()); Fire; ++Fire)
	{
		if (Fire->GetIntensityRatio() > 0.0f)
		{
			BurningFires.Add(*Fire);
		}
	}

	const float CheckInterval = FMath::Max(0.05f, FireRules->SpreadCheckInterval);
	const float SpreadDistance = FMath::Max(1.0f, FireRules->SpreadDistanceCm);
	const float SpreadExposureSeconds = FMath::Max(0.1f, FireRules->SpreadIgnitionExposureSeconds);
	const float HeatPerSecond = 1.0f / SpreadExposureSeconds;
	const float CoolingPerSecond = FMath::Max(0.0f, FireRules->SpreadCoolingPerSecond);
	const float NewFireIntensity = FMath::Clamp(
		FireRules->SpreadFireInitialIntensityRatio, 0.01f, 1.0f);
	const float ReignitionMinExposure = FMath::Max(0.0f,
		FireRules->SpreadReignitionMinExposure);
	const double CurrentTimeSeconds = GetWorldRef().GetTimeSeconds();

	for (const TWeakObjectPtr<AFireSpreadAnchor>& WeakAnchor : SpreadAnchors)
	{
		AFireSpreadAnchor* Anchor = WeakAnchor.Get();
		if (!Anchor || Anchor->IsIgnited() || !Anchor->CanIgnite() ||
			!Anchor->IsReignitionReady(CurrentTimeSeconds) ||
			Anchor->GetMaterialTag().IsNone())
		{
			continue;
		}

		const FFireMaterialSetting* Material = FindMaterialSetting(Anchor->GetMaterialTag());
		if (Material && !Material->bCanBurn)
		{
			continue;
		}

		float Exposure = 0.0f;
		for (const AFireActor* Fire : BurningFires)
		{
			const float Distance = FVector::Distance(Fire->GetActorLocation(), Anchor->GetActorLocation());
			if (Distance >= SpreadDistance || !HasClearHeatPath(*Fire, *Anchor))
			{
				continue;
			}

			const float DistanceFactor = 1.0f - Distance / SpreadDistance;
			Exposure += Fire->GetIntensityRatio() * DistanceFactor;
		}
		if (Anchor->IsAwaitingReignition() && Exposure < ReignitionMinExposure)
		{
			Exposure = 0.0f;
		}

		const float HeatReceiveMultiplier = Material
			? FMath::Max(0.0f, Material->HeatReceiveMultiplier) : 1.0f;
		const float HeatGain = Exposure * HeatPerSecond * HeatReceiveMultiplier * CheckInterval;
		const float HeatLoss = Exposure > 0.0f ? 0.0f : CoolingPerSecond * CheckInterval;
		Anchor->AddHeat(HeatGain - HeatLoss);

		const float IgnitionThreshold = Material
			? FMath::Max(UE_KINDA_SMALL_NUMBER, Material->IgnitionThreshold) : 1.0f;
		if (Anchor->GetAccumulatedHeat() < IgnitionThreshold)
		{
			continue;
		}

		const bool bIsReignition = Anchor->IsAwaitingReignition();
		bool bIgnitionSucceeded = false;
		if (TWeakObjectPtr<AFireActor>* TrackedFire = AnchorFires.Find(
			TWeakObjectPtr<AFireSpreadAnchor>(Anchor));
			bIsReignition && TrackedFire && TrackedFire->IsValid())
		{
			bIgnitionSucceeded = TrackedFire->Get()->ReigniteFromNearbyFire(NewFireIntensity);
		}
		else
		{
			bIgnitionSucceeded = IgniteLocation(Anchor->GetActorLocation(), NewFireIntensity,
				Anchor->GetMaterialTag(), Anchor);
		}

		if (bIgnitionSucceeded)
		{
			Anchor->MarkIgnited();
			if (bIsReignition)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("Nearby fire reignited anchor '%s' (Material=%s)."),
					*GetNameSafe(Anchor), *Anchor->GetMaterialTag().ToString());
			}
			else
			{
				++SpreadIgnitionCount;
				UE_LOG(LogTemp, Warning,
					TEXT("Fire spread ignited anchor '%s' (Material=%s, SpreadCount=%d)."),
					*GetNameSafe(Anchor), *Anchor->GetMaterialTag().ToString(), SpreadIgnitionCount);
				CheckEvacuationThreshold(*Anchor);
			}
		}
	}
}

bool UFireManager::HasClearHeatPath(const AFireActor& SourceFire,
	const AFireSpreadAnchor& TargetAnchor) const
{
	const FVector Start = SourceFire.GetActorLocation() + FVector(0.0f, 0.0f, 30.0f);
	const FVector End = TargetAnchor.GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FireSpreadOcclusion), false);
	QueryParams.AddIgnoredActor(&SourceFire);
	QueryParams.AddIgnoredActor(&TargetAnchor);
	return !GetWorldRef().LineTraceTestByChannel(Start, End, ECC_Visibility, QueryParams);
}

void UFireManager::CheckEvacuationThreshold(const AFireSpreadAnchor& IgnitedAnchor)
{
	if (!ActiveScenarioData || ScenarioState == EFireScenarioState::EvacuationRequired)
	{
		return;
	}

	const int32 FireCountLimit = FMath::Max(1, ActiveScenarioData->UncontrollableFireCount);
	const float DistanceLimit = FMath::Max(0.0f, ActiveScenarioData->EvacuationSpreadDistanceCm);
	const float SpreadDistance = FVector::Distance(
		InitialFireLocation, IgnitedAnchor.GetActorLocation());
	if (SpreadIgnitionCount >= FireCountLimit ||
		(DistanceLimit > 0.0f && SpreadDistance >= DistanceLimit))
	{
		RequireEvacuation();
	}
}

const FFireMaterialSetting* UFireManager::FindMaterialSetting(FName MaterialTag) const
{
	const UFireSimulationDataAsset* FireRules = ActiveScenarioData
		? ActiveScenarioData->FireRules.Get() : nullptr;
	if (!FireRules)
	{
		return nullptr;
	}

	const FName CanonicalMaterialTag = FireMaterialTags::Canonicalize(MaterialTag);
	return FireRules->MaterialSettings.FindByPredicate(
		[CanonicalMaterialTag](const FFireMaterialSetting& Setting)
		{
			return FireMaterialTags::Canonicalize(Setting.MaterialTag) == CanonicalMaterialTag;
		});
}

EFireClass UFireManager::ResolveFireClass(FName MaterialTag) const
{
	if (const FFireMaterialSetting* Material = FindMaterialSetting(MaterialTag))
	{
		return Material->FireClass;
	}

	const FName CanonicalMaterialTag = FireMaterialTags::Canonicalize(MaterialTag);
	if (CanonicalMaterialTag == FName(TEXT("FireMaterial.CookingOil")))
	{
		return EFireClass::K;
	}
	if (CanonicalMaterialTag == FName(TEXT("FireMaterial.ElectricalEnergized")))
	{
		return EFireClass::C;
	}
	if (CanonicalMaterialTag == FName(TEXT("FireMaterial.FlammableLiquid")))
	{
		return EFireClass::B;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Fire MaterialTag '%s' has no matching material setting. Using Class A."),
		MaterialTag.IsNone() ? TEXT("None") : *MaterialTag.ToString());
	return EFireClass::A;
}

AFireSpreadAnchor* UFireManager::FindClosestSpreadAnchor(
	const FVector& Location, float MaxDistanceCm, FName RequiredMaterialTag) const
{
	RequiredMaterialTag = FireMaterialTags::Canonicalize(RequiredMaterialTag);
	AFireSpreadAnchor* ClosestAnchor = nullptr;
	float ClosestDistanceSquared = FMath::Square(FMath::Max(0.0f, MaxDistanceCm));

	for (TActorIterator<AFireSpreadAnchor> Anchor(&GetWorldRef()); Anchor; ++Anchor)
	{
		if (!RequiredMaterialTag.IsNone() &&
			Anchor->GetMaterialTag() != RequiredMaterialTag)
		{
			continue;
		}
		const float DistanceSquared = FVector::DistSquared(Location, Anchor->GetActorLocation());
		if (DistanceSquared <= ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestAnchor = *Anchor;
		}
	}

	return ClosestAnchor;
}

void UFireManager::ResetSpreadSystem()
{
	GetWorldRef().GetTimerManager().ClearTimer(SpreadEvaluationTimer);
	SpreadAnchors.Reset();
	SpreadIgnitionCount = 0;
	InitialFireLocation = FVector::ZeroVector;
}

void UFireManager::RegisterFire(AFireActor& Fire)
{
	Fire.OnIntensityChanged.RemoveAll(this);
	Fire.OnIntensityChanged.AddUObject(this, &UFireManager::HandleTrackedFireIntensityChanged);

	// A spread fire can be registered while the five-second observation is running.
	HandleTrackedFireIntensityChanged(Fire.GetIntensityRatio());
}

void UFireManager::NotifyFireExtinguished(AFireActor& Fire)
{
	AFireSpreadAnchor* ExtinguishedAnchor = nullptr;
	for (auto It = AnchorFires.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || !It.Value().IsValid())
		{
			It.RemoveCurrent();
			continue;
		}
		if (It.Value().Get() == &Fire)
		{
			ExtinguishedAnchor = It.Key().Get();
			break;
		}
	}

	if (ExtinguishedAnchor && ActiveScenarioData)
	{
		const UFireSimulationDataAsset* FireRules = ActiveScenarioData->FireRules.Get();
		const float MinCooldown = FireRules
			? FMath::Max(0.0f, FireRules->SpreadReignitionCooldownMinSeconds) : 1.0f;
		const float MaxCooldown = FireRules
			? FMath::Max(MinCooldown, FireRules->SpreadReignitionCooldownMaxSeconds) : 2.0f;
		const float Cooldown = FMath::FRandRange(MinCooldown, MaxCooldown);
		ExtinguishedAnchor->MarkExtinguished(GetWorldRef().GetTimeSeconds(), Cooldown);
		UE_LOG(LogTemp, Log,
			TEXT("Anchor '%s' can receive reignition heat again in %.2f seconds."),
			*GetNameSafe(ExtinguishedAnchor), Cooldown);
	}

	if (!bTrainingWorld || bTrainingFinished ||
		ScenarioState == EFireScenarioState::EvacuationRequired ||
		IsEvacuationOnlyScenario())
	{
		return;
	}

	if (!HasAnyBurningFire())
	{
		BeginExtinguishmentMonitoring();
	}
}

void UFireManager::NotifyFireReignited(AFireActor& Fire)
{
	for (auto It = AnchorFires.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || !It.Value().IsValid())
		{
			It.RemoveCurrent();
			continue;
		}
		if (It.Value().Get() == &Fire)
		{
			It.Key().Get()->MarkIgnited();
			break;
		}
	}

	if (!bTrainingWorld || bTrainingFinished || !IsForcedReignition(Fire.GetFireClass()))
	{
		return;
	}

	OnScenarioFeedbackRequested.Broadcast(ETrainingFeedbackType::Reignited);
}

bool UFireManager::HasAnyBurningFire() const
{
	for (TActorIterator<AFireActor> Fire(&GetWorldRef()); Fire; ++Fire)
	{
		if (Fire->GetIntensityRatio() > 0.0f)
		{
			return true;
		}
	}
	return false;
}

void UFireManager::BeginExtinguishmentMonitoring()
{
	if (ScenarioState == EFireScenarioState::ExtinguishmentMonitoring ||
		ScenarioState == EFireScenarioState::Completed)
	{
		return;
	}

	ScenarioState = EFireScenarioState::ExtinguishmentMonitoring;
	OnScenarioFeedbackRequested.Broadcast(ETrainingFeedbackType::Monitoring);

	const float ObservationSeconds = ActiveScenarioData
		? FMath::Max(0.0f, ActiveScenarioData->ExtinguishmentObservationSeconds) : 5.0f;
	if (ObservationSeconds <= 0.0f)
	{
		CompleteExtinguishmentObservation();
		return;
	}

	GetWorldRef().GetTimerManager().SetTimer(ExtinguishmentObservationTimer, this,
		&UFireManager::CompleteExtinguishmentObservation, ObservationSeconds, false);
	UE_LOG(LogTemp, Log, TEXT("All fires are out. Monitoring for %.1f seconds."), ObservationSeconds);
}

void UFireManager::CompleteExtinguishmentObservation()
{
	if (bTrainingFinished || ScenarioState != EFireScenarioState::ExtinguishmentMonitoring)
	{
		return;
	}

	if (HasAnyBurningFire())
	{
		CancelExtinguishmentMonitoring(true);
		return;
	}

	FinishTraining(ETrainingOutcome::Extinguished);
}

void UFireManager::CancelExtinguishmentMonitoring(bool bShowReignitionFeedback)
{
	GetWorldRef().GetTimerManager().ClearTimer(ExtinguishmentObservationTimer);
	if (ScenarioState == EFireScenarioState::ExtinguishmentMonitoring)
	{
		ScenarioState = EFireScenarioState::Active;
		if (bShowReignitionFeedback)
		{
			OnScenarioFeedbackRequested.Broadcast(ETrainingFeedbackType::Reignited);
		}
	}
}

void UFireManager::HandleTrackedFireIntensityChanged(float IntensityRatio)
{
	if (IntensityRatio > 0.0f &&
		ScenarioState == EFireScenarioState::ExtinguishmentMonitoring)
	{
		CancelExtinguishmentMonitoring(true);
	}
}

void UFireManager::NotifyAgentConsumed(float UsedAgentKg)
{
	if (!bTrainingWorld || bTrainingFinished || UsedAgentKg <= 0.0f)
	{
		return;
	}

	TotalUsedAgentKg += UsedAgentKg;
}

void UFireManager::RequireEvacuation()
{
	if (!bTrainingWorld || bTrainingFinished ||
		ScenarioState == EFireScenarioState::EvacuationRequired ||
		ScenarioState == EFireScenarioState::Completed)
	{
		return;
	}

	GetWorldRef().GetTimerManager().ClearTimer(ExtinguishmentObservationTimer);
	ScenarioState = EFireScenarioState::EvacuationRequired;
	OnScenarioFeedbackRequested.Broadcast(ETrainingFeedbackType::EvacuateNow);

	const float GraceSeconds = ActiveScenarioData
		? FMath::Max(0.0f, ActiveScenarioData->EvacuationGraceSeconds) : 10.0f;
	if (GraceSeconds <= 0.0f)
	{
		HandleEvacuationTimeout();
		return;
	}

	GetWorldRef().GetTimerManager().SetTimer(EvacuationGraceTimer, this,
		&UFireManager::HandleEvacuationTimeout, GraceSeconds, false);
	UE_LOG(LogTemp, Warning, TEXT("Evacuation required. Player has %.1f seconds to reach an exit zone."),
		GraceSeconds);
}

void UFireManager::NotifyPlayerEvacuated()
{
	if (!bTrainingWorld || bTrainingFinished)
	{
		return;
	}

	const bool bEvacuationWasRequired =
		ScenarioState == EFireScenarioState::EvacuationRequired;
	const bool bChoseEarlyEvacuation = IsEvacuationOnlyScenario() &&
		(ScenarioState == EFireScenarioState::Active ||
		 ScenarioState == EFireScenarioState::ExtinguishmentMonitoring);
	if (!bEvacuationWasRequired && !bChoseEarlyEvacuation)
	{
		return;
	}

	FinishTraining(ETrainingOutcome::Evacuated);
}

void UFireManager::HandleEvacuationTimeout()
{
	if (!bTrainingFinished && ScenarioState == EFireScenarioState::EvacuationRequired)
	{
		FinishTraining(ETrainingOutcome::Failed, ETrainingFailureReason::IgnoredEvacuation);
	}
}

void UFireManager::HandleScenarioTimeout()
{
	if (bTrainingWorld && !bTrainingFinished)
	{
		FinishTraining(ETrainingOutcome::Failed, ETrainingFailureReason::ScenarioTimeout);
	}
}

void UFireManager::FinishTraining(ETrainingOutcome Outcome, ETrainingFailureReason FailureReason)
{
	if (!bTrainingWorld || bTrainingFinished)
	{
		return;
	}

	bTrainingFinished = true;
	ScenarioState = EFireScenarioState::Completed;
	GetWorldRef().GetTimerManager().ClearTimer(ExtinguishmentObservationTimer);
	GetWorldRef().GetTimerManager().ClearTimer(EvacuationGraceTimer);
	GetWorldRef().GetTimerManager().ClearTimer(ScenarioTimeoutTimer);
	GetWorldRef().GetTimerManager().ClearTimer(SpreadEvaluationTimer);
	FTrainingResult Result;
	Result.Outcome = Outcome;
	Result.FailureReason = FailureReason;
	Result.CompletionTime = static_cast<float>(FMath::Max(
		0.0, GetWorldRef().GetTimeSeconds() - ScenarioStartTimeSeconds));
	Result.UsedAgentKg = FMath::Max(0.0f, TotalUsedAgentKg);
	Result.ResultMessage = ResolveResultMessage(Outcome, FailureReason);

	UE_LOG(LogTemp, Log,
		TEXT("Training finished. Outcome=%d FailureReason=%d Time=%.1fs UsedAgent=%.2fkg"),
		static_cast<int32>(Outcome), static_cast<int32>(FailureReason),
		Result.CompletionTime, Result.UsedAgentKg);
	ATrainingResultGameMode::OpenResultLevel(&GetWorldRef(), Result);
}

void UFireManager::Deinitialize()
{
	GetWorldRef().GetTimerManager().ClearTimer(ExtinguishmentObservationTimer);
	GetWorldRef().GetTimerManager().ClearTimer(EvacuationGraceTimer);
	GetWorldRef().GetTimerManager().ClearTimer(ScenarioTimeoutTimer);
	ResetSpreadSystem();
	AnchorFires.Reset();
	for (TActorIterator<AFireActor> Fire(&GetWorldRef()); Fire; ++Fire)
	{
		Fire->OnIntensityChanged.RemoveAll(this);
	}
	OnScenarioFeedbackRequested.Clear();
	OnScenarioStarted.Clear();
	ActiveScenarioData = nullptr;
	bTrainingWorld = false;
	bTrainingFinished = false;
	bScenarioStarted = false;
	bTrashBinGuaranteedReignitionUsed = false;
	ScenarioStartTimeSeconds = 0.0;
	TotalUsedAgentKg = 0.0f;
	ScenarioState = EFireScenarioState::None;
	Super::Deinitialize();
}
