#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "MapObjects/Interactable.h"
#include "MapObjects/MapObject.h"
#include "LockableDoor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTexture2D;
class UWidgetComponent;

UCLASS()
class FIREFIGHTSUMUL_API ALockableDoor : public AMapObject, public IInteractable
{
	GENERATED_BODY()

public:
	ALockableDoor();
	static const FName AimTag;

	virtual void Interact_Implementation(AFireFighter* Player) override;

	UFUNCTION(BlueprintCallable, Category = "Door")
	bool TryToggleOpen();

	bool CanToggleLock(const AFireFighter& Player) const;
	void UpdateInteractionPrompt(const AFireFighter* Player);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void AdvanceDoorMovement();
	void UpdateHingeRotation();

	UPROPERTY(VisibleAnywhere, Category = "Door")
	TObjectPtr<UStaticMeshComponent> FrameMesh;

	UPROPERTY(VisibleAnywhere, Category = "Door")
	TObjectPtr<USceneComponent> Hinge;

	UPROPERTY(VisibleAnywhere, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Initial State", meta = (AllowPrivateAccess = "true", ToolTip = "Required when starting locked. Leave as None for a door with no lock."))
	FName RequiredKeyId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Initial State", meta = (AllowPrivateAccess = "true", ToolTip = "Start open and unlocked. The hinge transform in the editor defines the closed pose."))
	bool bStartsOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Initial State", meta = (AllowPrivateAccess = "true", EditCondition = "!bStartsOpen", ToolTip = "Start locked. Requires a matching key ID."))
	bool bStartsLocked = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (AllowPrivateAccess = "true", Units = "deg"))
	float OpenAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ToolTip = "Opening and closing speed in degrees per second."))
	float OpenSpeed = 120.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door", meta = (AllowPrivateAccess = "true"))
	bool bIsLocked = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door", meta = (AllowPrivateAccess = "true"))
	bool bIsOpen = false;

	UPROPERTY()
	TObjectPtr<UTexture2D> LockedPromptTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> UnlockPromptTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> OpenClosePromptTexture;

	TWeakObjectPtr<UWidgetComponent> PromptComponent;
	const UTexture2D* DisplayedPromptTexture = nullptr;

	FQuat ClosedRotation = FQuat::Identity;
	float CurrentAngle = 0.0f;
	double LastMovementUpdateTime = 0.0;
	FTimerHandle MovementTimer;
};
