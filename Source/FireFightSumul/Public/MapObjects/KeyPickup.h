#pragma once

#include "CoreMinimal.h"
#include "MapObjects/MapObject.h"
#include "KeyPickup.generated.h"

class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class FIREFIGHTSUMUL_API AKeyPickup : public AMapObject
{
	GENERATED_BODY()

public:
	AKeyPickup();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "Key")
	TObjectPtr<USphereComponent> PickupCollision;

	UPROPERTY(VisibleAnywhere, Category = "Key")
	TObjectPtr<UStaticMeshComponent> KeyMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Key", meta = (AllowPrivateAccess = "true"))
	FName KeyId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Key", meta = (AllowPrivateAccess = "true", ToolTip = "Spin speed around the key actor's local Z axis, in degrees per second."))
	float RotationSpeed = 90.0f;
};
