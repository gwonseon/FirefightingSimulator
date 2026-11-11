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
DECLARE_MULTICAST_DELEGATE(FOnFireScenarioStarted);

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
	void NotifyFireExtinguished(AFireActor& Fire);
	void NotifyFireReignited(AFireActor& Fire);
	void NotifyPlayerEvacuated();
	void NotifyAgentConsumed(float UsedAgentKg);
	bool CanScheduleReignition(EFireClass FireClass, float DefaultChance) const;
	bool ShouldReignite(EFireClass FireClass, float DefaultChance);
	bool IsForcedReignition(EFireClass FireClass) const;
	float ResolveReignitionDelay(EFireClass FireClass, float DefaultDelay) const;
	float ResolveReignitionStartRatio(EFireClass FireClass, float DefaultRatio) const;

	UFUNCTION(BlueprintCallable, Category = "Fire Training")
	void RequireEvacuation();

	FOnScenarioFeedbackRequested OnScenarioFeedbackRequested;
	FOnFireScenarioStarted OnScenarioStarted;

private:
	void BeginExtinguishmentMonitoring();
	void CompleteExtinguishmentObservation();
	void CancelExtinguishmentMonitoring(bool bShowReignitionFeedback);
	void HandleTrackedFireIntensityChanged(float IntensityRatio);
	void HandleEvacuationTimeout();
	void HandleScenarioTimeout();
	void InitializeSpreadSystem(const FVector& InitialLocation, FName InitialMaterialTag);
	void EvaluateFireSpread();
	void ResetSpreadSystem();
	bool HasClearHeatPath(const AFireActor& SourceFire, const AFireSpreadAnchor& TargetAnchor) const;
	bool IgniteLocation(const FVector& Location, float InitialIntensityRatio, FName MaterialTag,
		AFireSpreadAnchor* SourceAnchor = nullptr);
	void CheckEvacuationThreshold(const AFireSpreadAnchor& IgnitedAnchor);
	const FFireMaterialSetting* FindMaterialSetting(FName MaterialTag) const;
	EFireClass ResolveFireClass(FName MaterialTag) const;
	AFireSpreadAnchor* FindClosestSpreadAnchor(const FVector& Location, float MaxDistanceCm,
		FName RequiredMaterialTag = NAME_None) const;
	bool HasAnyBurningFire() const;
	bool IsTrashBinScenario() const;
	bool IsEvacuationOnlyScenario() const;
	float ResolveGrowthToPeakSeconds() const;
	void FinishTraining(ETrainingOutcome Outcome,
		ETrainingFailureReason FailureReason = ETrainingFailureReason::None);
	bool IgnitePoint(const ATargetPoint& Point, float InitialIntensityRatio, FName& OutMaterialTag);
	ATargetPoint* SelectStartPoint(const UFireScenarioDataAsset& ScenarioData) const;

	UPROPERTY(Transient)
	TObjectPtr<UFireScenarioDataAsset> ActiveScenarioData;

	bool bFireCollisionEnabled = true;
	bool bTrainingWorld = false;
	bool bTrainingFinished = false;
	bool bScenarioStarted = false;
	bool bTrashBinGuaranteedReignitionUsed = false;
	double ScenarioStartTimeSeconds = 0.0;
	float TotalUsedAgentKg = 0.0f;
	EFireScenarioState ScenarioState = EFireScenarioState::None;
	FVector InitialFireLocation = FVector::ZeroVector;
	int32 SpreadIgnitionCount = 0;
	TArray<TWeakObjectPtr<AFireSpreadAnchor>> SpreadAnchors;
	TMap<TWeakObjectPtr<AFireSpreadAnchor>, TWeakObjectPtr<AFireActor>> AnchorFires;
	FTimerHandle ExtinguishmentObservationTimer;
	FTimerHandle EvacuationGraceTimer;
	FTimerHandle ScenarioTimeoutTimer;
	FTimerHandle SpreadEvaluationTimer;
};
