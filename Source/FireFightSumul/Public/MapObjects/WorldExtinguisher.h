#pragma once

#include "CoreMinimal.h"
#include "MapObjects/Interactable.h"
#include "MapObjects/MapObject.h"
#include "WorldExtinguisher.generated.h"

class UBoxComponent;
class UCameraComponent;
class UExtinguisherDataAsset;
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
	const UExtinguisherDataAsset* GetExtinguisherData() const { return ExtinguisherData; }
	float GetAgentCapacityKg() const;
	float GetRemainingAgentKg() const { return RemainingAgentKg; }
	float GetAgentRatio() const;
	float GetDischargeRateKgPerSecond() const;
	float GetBaseSuppressionRatioPerSecond() const;
	float GetMinSafeDistanceCm() const;
	float GetMaxEffectiveDistanceCm() const;
	float GetLowAgentStartRatio() const;
	float GetRemainingAgentPerformanceFactor() const;
	float ConsumeAgentKg(float RequestedAmountKg);

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Extinguisher|Data", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UExtinguisherDataAsset> ExtinguisherData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Extinguisher|Pin", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", Units = "cm"))
	float PinPullDistance = 8.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Extinguisher|Agent", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float InitialAgentRatio = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Extinguisher|Agent", meta = (AllowPrivateAccess = "true", Units = "kg"))
	float RemainingAgentKg = 0.0f;

	FVector PinRestRelativeLocation = FVector::ZeroVector;
};
