#pragma once

#include "CoreMinimal.h"
#include "MapObjects/Interactable.h"
#include "MapObjects/MapObject.h"
#include "WorldExtinguisher.generated.h"

class UBoxComponent;
class UCameraComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class FIREFIGHTSUMUL_API AWorldExtinguisher : public AMapObject, public IInteractable
{
	GENERATED_BODY()

public:
	AWorldExtinguisher();

	UFUNCTION(BlueprintCallable, Category = "Extinguisher")
	bool TryRemovePin();

	bool HasRemovedPin() const { return bIsPinRemoved; }
	bool TryDropNear(const AActor& Dropper);
	bool CanGrabPin(const FVector& Start, const FVector& End) const;
	void GetPinPullLocations(FVector& RestLocation, FVector& PulledLocation) const;
	void SetPinPullProgress(float Progress);
	float GetRemainingAgent() const { return RemainingAgent; }
	float GetAgentRatio() const;
	float ConsumeAgent(float RequestedAmount);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Extinguisher")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extinguisher", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> InspectCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extinguisher", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> PinPivot;

	UPROPERTY(VisibleAnywhere, Category = "Extinguisher")
	TObjectPtr<UStaticMeshComponent> PinMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extinguisher", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> PinHitBox;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Extinguisher", meta = (AllowPrivateAccess = "true"))
	bool bIsPinRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Extinguisher|Pin", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", Units = "cm"))
	float PinPullDistance = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Extinguisher|Agent", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AgentCapacity = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Extinguisher|Agent", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float InitialAgentAmount = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Extinguisher|Agent", meta = (AllowPrivateAccess = "true"))
	float RemainingAgent = 0.0f;

	FVector PinRestRelativeLocation = FVector::ZeroVector;
};
