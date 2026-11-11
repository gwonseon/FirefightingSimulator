#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "Training/FireTrainingTypes.h"
#include "FireManager.generated.h"

class AFireActor;
class AFireSpreadAnchor;
class ATargetPoint;
class UFireScenarioDataAsset;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnScenarioFeedbackRequested, ETrainingFeedbackType);

UCLASS()
class FIREFIGHTSUMUL_API UFireManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	void ToggleFireCollision();
	bool IsFireCollisionEnabled() const { return bFireCollisionEnabled; }
	bool HasFinishedTraining() const { return bTrainingFinished; }
	bool StartScenario(UFireScenarioDataAsset* InScenarioData);
	const UFireScenarioDataAsset* GetActiveScenarioData() const { return ActiveScenarioData; }
	EFireScenarioState GetScenarioState() const { return ScenarioState; }
	void RegisterFire(AFireActor& Fire);
	void NotifyFireExtinguished();
	void NotifyPlayerEvacuated();
	bool CanScheduleReignition(float DefaultChance) const;
	bool ShouldReignite(float DefaultChance);

	UFUNCTION(BlueprintCallable, Category = "Fire Training")
	void RequireEvacuation();

	FOnScenarioFeedbackRequested OnScenarioFeedbackRequested;

private:
	void BeginExtinguishmentMonitoring();
	void CompleteExtinguishmentObservation();
	void CancelExtinguishmentMonitoring(bool bShowReignitionFeedback);
	void HandleTrackedFireIntensityChanged(float IntensityRatio);
	void HandleEvacuationTimeout();
	void InitializeSpreadSystem(const FVector& InitialLocation);
	void EvaluateFireSpread();
	void ResetSpreadSystem();
	bool HasClearHeatPath(const AFireActor& SourceFire, const AFireSpreadAnchor& TargetAnchor) const;
	bool IgniteLocation(const FVector& Location, float InitialIntensityRatio, FName MaterialTag);
	void CheckEvacuationThreshold(const AFireSpreadAnchor& IgnitedAnchor);
	const FFireMaterialSetting* FindMaterialSetting(FName MaterialTag) const;
	EFireClass ResolveFireClass(FName MaterialTag) const;
	AFireSpreadAnchor* FindClosestSpreadAnchor(const FVector& Location, float MaxDistanceCm) const;
	bool HasAnyBurningFire() const;
	bool IsTrashBinScenario() const;
	void FinishTraining(ETrainingOutcome Outcome,
		ETrainingFailureReason FailureReason = ETrainingFailureReason::None);
	bool IgnitePoint(const ATargetPoint& Point, float InitialIntensityRatio);
	ATargetPoint* SelectStartPoint(const UFireScenarioDataAsset& ScenarioData) const;

	UPROPERTY(Transient)
	TObjectPtr<UFireScenarioDataAsset> ActiveScenarioData;

	bool bFireCollisionEnabled = true;
	bool bTrainingWorld = false;
	bool bTrainingFinished = false;
	bool bScenarioStarted = false;
	bool bTrashBinGuaranteedReignitionUsed = false;
	EFireScenarioState ScenarioState = EFireScenarioState::None;
	FVector InitialFireLocation = FVector::ZeroVector;
	int32 SpreadIgnitionCount = 0;
	TArray<TWeakObjectPtr<AFireSpreadAnchor>> SpreadAnchors;
	FTimerHandle ExtinguishmentObservationTimer;
	FTimerHandle EvacuationGraceTimer;
	FTimerHandle SpreadEvaluationTimer;
};
