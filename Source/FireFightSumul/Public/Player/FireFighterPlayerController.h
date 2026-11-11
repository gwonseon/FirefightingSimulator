#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/PlayerController.h"
#include "Training/FireTrainingTypes.h"
#include "FireFighterPlayerController.generated.h"

class AFireActor;
class ALockableDoor;
class AFireFighter;
class AWorldExtinguisher;
class UHUDWidget;
class UPlayerInteractionComponent;

UCLASS()
class FIREFIGHTSUMUL_API AFireFighterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	bool IsInspectingExtinguisher() const { return InspectedExtinguisher != nullptr; }
	void BeginExtinguisherInspection(AWorldExtinguisher* Extinguisher);
	void EndExtinguisherInspection();
	void BeginExtinguisherPinDrag();
	void UpdateExtinguisherPinDrag();
	void EndExtinguisherPinDrag();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUnPossess() override;

private:
	enum class EActionHint : uint8
	{
		None,
		InspectExtinguisher,
		EquipExtinguisher,
		PullPin,
		Spray
	};

	void RefreshFocusedTarget();
	void RefreshTrainingGuidance(const AActor* Target);
	void SetActionHint(EActionHint Hint);
	void UpdateDoorPrompt(ALockableDoor* Door, const AFireFighter* PlayerPawn);
	void HandleFireIntensityChanged(float IntensityRatio);
	void RefreshExtinguisherStatus();
	void HandleExtinguisherStatusChanged(bool bEquipped, float AgentRatio);
	void HandleTrainingFeedbackSampled(ETrainingFeedbackType Type);
	void HandleScenarioFeedbackRequested(ETrainingFeedbackType Type);
	void ResetTrainingFeedbackState();

	UFUNCTION()
	void HandleControlledPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleInspectedExtinguisherEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	UPROPERTY(Transient)
	TObjectPtr<AWorldExtinguisher> InspectedExtinguisher;

	TWeakObjectPtr<AActor> PreviousViewTarget;
	bool bPreviousShowMouseCursor = false;
	bool bRestoreHoseAfterInspection = false;
	double PinDragAvailableAt = 0.0;
	double PinDragStartX = 0.0;
	double PinDragScreenDistance = 0.0;

	EActionHint ActiveActionHint = EActionHint::None;
	ETrainingFeedbackType PendingFeedbackType = ETrainingFeedbackType::None;
	bool bLookingForAnotherFire = false;
	double SprayHintUntil = 0.0;
	double PendingFeedbackSince = 0.0;
	TMap<ETrainingFeedbackType, double> FeedbackCooldownUntil;
	TWeakObjectPtr<const AWorldExtinguisher> LastGuidedExtinguisher;
	TWeakObjectPtr<UPlayerInteractionComponent> ExtinguisherStatusSource;
	TWeakObjectPtr<AFireActor> FocusedFire;
	TWeakObjectPtr<ALockableDoor> FocusedDoor;
	FTimerHandle AimFocusTimer;
	FTimerHandle FireCompletionTimer;
	FTimerHandle FeedbackHideTimer;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UHUDWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UHUDWidget> HUDWidget;
};
