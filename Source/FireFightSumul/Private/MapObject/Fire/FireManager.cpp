#include "MapObject/Fire/FireManager.h"

#include "Data/FireScenarioDataAsset.h"
#include "Data/FireSimulationDataAsset.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "MapObject/Fire/FireActor.h"
#include "MapObject/Fire/FireSpreadAnchor.h"
#include "TimerManager.h"
#include "UI/TrainingResultGameMode.h"

namespace
{
	constexpr float InitialAnchorMatchDistanceCm = 100.0f;
	const FName TrashBinScenarioId(TEXT("TrashBin"));
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
	bTrainingWorld = true;
	bTrainingFinished = false;
	bScenarioStarted = true;
	bTrashBinGuaranteedReignitionUsed = false;
	ScenarioState = EFireScenarioState::Ready;

	const float InitialIntensity = FMath::Clamp(InScenarioData->InitialFireIntensityRatio, 0.01f, 1.0f);
	if (!IgnitePoint(*StartPoint, InitialIntensity))
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
	InitializeSpreadSystem(StartPoint->GetActorLocation());

	UE_LOG(LogTemp, Log, TEXT("Started fire scenario '%s' at TargetPoint '%s' (Tag=%s, Mode=%s)."),
		*InScenarioData->ScenarioId.ToString(), *GetNameSafe(StartPoint),
		*InScenarioData->StartPointTag.ToString(),
		InScenarioData->StartSelectionMode == EFireStartSelectionMode::Random
			? TEXT("Random") : TEXT("Fixed"));
	return true;
}

bool UFireManager::CanScheduleReignition(float DefaultChance) const
{
	if (IsTrashBinScenario())
	{
		return !bTrashBinGuaranteedReignitionUsed;
	}

	return DefaultChance > 0.0f;
}

bool UFireManager::ShouldReignite(float DefaultChance)
{
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

bool UFireManager::IsTrashBinScenario() const
{
	return ActiveScenarioData && ActiveScenarioData->ScenarioId == TrashBinScenarioId;
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

bool UFireManager::IgnitePoint(const ATargetPoint& Point, float InitialIntensityRatio)
{
	const AFireSpreadAnchor* InitialAnchor = FindClosestSpreadAnchor(
		Point.GetActorLocation(), InitialAnchorMatchDistanceCm);
	const FName MaterialTag = InitialAnchor ? InitialAnchor->GetMaterialTag() : NAME_None;

	if (!InitialAnchor)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Initial fire point '%s' has no FireSpreadAnchor within %.0fcm. Using Class A."),
			*GetNameSafe(&Point), InitialAnchorMatchDistanceCm);
	}

	return IgniteLocation(Point.GetActorLocation(), InitialIntensityRatio, MaterialTag);
}

bool UFireManager::IgniteLocation(const FVector& Location, float InitialIntensityRatio, FName MaterialTag)
{
	const FTransform Transform(FQuat::Identity, Location);
	AFireActor* Fire = GetWorldRef().SpawnActorDeferred<AFireActor>(AFireActor::StaticClass(), Transform);
	if (!Fire)
	{
		return false;
	}

	Fire->SetInitialIntensityRatio(InitialIntensityRatio);
	Fire->SetFireClass(ResolveFireClass(MaterialTag));
	Fire->FinishSpawning(Transform);
	RegisterFire(*Fire);
	return true;
}

void UFireManager::InitializeSpreadSystem(const FVector& InitialLocation)
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
		InitialLocation, InitialAnchorMatchDistanceCm))
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
	const float HeatPerSecond = FMath::Max(0.0f, FireRules->SpreadHeatPerSecond);
	const float CoolingPerSecond = FMath::Max(0.0f, FireRules->SpreadCoolingPerSecond);
	const float NewFireIntensity = FMath::Clamp(
		FireRules->SpreadFireInitialIntensityRatio, 0.01f, 1.0f);

	for (const TWeakObjectPtr<AFireSpreadAnchor>& WeakAnchor : SpreadAnchors)
	{
		AFireSpreadAnchor* Anchor = WeakAnchor.Get();
		if (!Anchor || Anchor->IsIgnited() || !Anchor->CanIgnite() ||
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

		if (IgniteLocation(Anchor->GetActorLocation(), NewFireIntensity, Anchor->GetMaterialTag()))
		{
			Anchor->MarkIgnited();
			++SpreadIgnitionCount;
			UE_LOG(LogTemp, Warning,
				TEXT("Fire spread ignited anchor '%s' (Material=%s, SpreadCount=%d)."),
				*GetNameSafe(Anchor), *Anchor->GetMaterialTag().ToString(), SpreadIgnitionCount);
			CheckEvacuationThreshold(*Anchor);
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

	return FireRules->MaterialSettings.FindByPredicate(
		[MaterialTag](const FFireMaterialSetting& Setting)
		{
			return Setting.MaterialTag == MaterialTag;
		});
}

EFireClass UFireManager::ResolveFireClass(FName MaterialTag) const
{
	if (const FFireMaterialSetting* Material = FindMaterialSetting(MaterialTag))
	{
		return Material->FireClass;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Fire MaterialTag '%s' has no matching material setting. Using Class A."),
		MaterialTag.IsNone() ? TEXT("None") : *MaterialTag.ToString());
	return EFireClass::A;
}

AFireSpreadAnchor* UFireManager::FindClosestSpreadAnchor(
	const FVector& Location, float MaxDistanceCm) const
{
	AFireSpreadAnchor* ClosestAnchor = nullptr;
	float ClosestDistanceSquared = FMath::Square(FMath::Max(0.0f, MaxDistanceCm));

	for (TActorIterator<AFireSpreadAnchor> Anchor(&GetWorldRef()); Anchor; ++Anchor)
	{
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

void UFireManager::NotifyFireExtinguished()
{
	if (!bTrainingWorld || bTrainingFinished ||
		ScenarioState == EFireScenarioState::EvacuationRequired)
	{
		return;
	}

	if (!HasAnyBurningFire())
	{
		BeginExtinguishmentMonitoring();
	}
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
	// Reaching the exterior before evacuation is required is not a scenario result.
	if (!bTrainingWorld || bTrainingFinished ||
		ScenarioState != EFireScenarioState::EvacuationRequired)
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
	GetWorldRef().GetTimerManager().ClearTimer(SpreadEvaluationTimer);
	const bool bSuccess = Outcome == ETrainingOutcome::Extinguished ||
		Outcome == ETrainingOutcome::Evacuated;
	UE_LOG(LogTemp, Log, TEXT("Training finished. Outcome=%d FailureReason=%d"),
		static_cast<int32>(Outcome), static_cast<int32>(FailureReason));
	ATrainingResultGameMode::OpenResultLevel(&GetWorldRef(), bSuccess);
}

void UFireManager::Deinitialize()
{
	GetWorldRef().GetTimerManager().ClearTimer(ExtinguishmentObservationTimer);
	GetWorldRef().GetTimerManager().ClearTimer(EvacuationGraceTimer);
	ResetSpreadSystem();
	for (TActorIterator<AFireActor> Fire(&GetWorldRef()); Fire; ++Fire)
	{
		Fire->OnIntensityChanged.RemoveAll(this);
	}
	OnScenarioFeedbackRequested.Clear();
	ActiveScenarioData = nullptr;
	bTrainingWorld = false;
	bTrainingFinished = false;
	bScenarioStarted = false;
	bTrashBinGuaranteedReignitionUsed = false;
	ScenarioState = EFireScenarioState::None;
	Super::Deinitialize();
}
