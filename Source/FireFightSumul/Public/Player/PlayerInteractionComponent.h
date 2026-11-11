#pragma once

#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "Training/FireTrainingTypes.h"
#include "PlayerInteractionComponent.generated.h"

class AFireActor;
class AFireFighter;
class AWorldExtinguisher;
class USceneComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnExtinguisherStatusChanged, bool, float);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTrainingFeedbackSampled, ETrainingFeedbackType);

UCLASS(ClassGroup = (Player))
class FIREFIGHTSUMUL_API UPlayerInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void Interact();
	void PrimaryInteract();
	void InteractWithDoor();
	void ContinuePrimaryInteract();
	void EndPrimaryInteract();
	bool EquipExtinguisher(AWorldExtinguisher& Extinguisher);

	AActor* FindAimedActor() const;

	bool IsExtinguisherEquipped() const;
	const AWorldExtinguisher* GetEquippedExtinguisher() const;
	float GetAgentRatio() const;
	void NotifyExtinguisherStatusChanged();

	FOnExtinguisherStatusChanged OnExtinguisherStatusChanged;
	FOnTrainingFeedbackSampled OnTrainingFeedbackSampled;

private:
	struct FSweepTrackingState
	{
		uint8 CoveredZoneMask = 0;
		double SweepStartedTime = -1.0;
	};

	AActor* FindFocusedActor(const AFireFighter& Player, ECollisionChannel Channel, const TCHAR* InputName = nullptr) const;
	ETrainingFeedbackType ApplySpray(const USceneComponent& Nozzle, const AWorldExtinguisher& Extinguisher,
		float SuppressionRatio);
	float UpdateSweepCoverage(AFireActor& Fire, float SignedLateralOffset01);
	void ReportTrainingFeedback(ETrainingFeedbackType Type);

	TWeakObjectPtr<AWorldExtinguisher> EquippedExtinguisher;
	TMap<TWeakObjectPtr<AFireActor>, FSweepTrackingState> SweepTrackingStates;
	bool bSpraying = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extinguisher",
		meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "80.0", Units = "deg"))
	float SprayHalfAngle = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extinguisher|Sweep",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.05", ClampMax = "0.9"))
	float SweepSideZoneThreshold = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extinguisher|Sweep",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.1", Units = "s"))
	float SweepCompletionWindowSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extinguisher|Sweep",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float MinimumSweepSuppressionFactor = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extinguisher|Sweep",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.05", ClampMax = "1.0"))
	float SweepBaseAimMaxHeight = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Interaction", meta = (ClampMin = "0.0", Units = "cm"))
	float InteractionDistance = 300.0f;
};
