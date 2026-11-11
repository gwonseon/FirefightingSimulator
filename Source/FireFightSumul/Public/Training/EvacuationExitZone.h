#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EvacuationExitZone.generated.h"

class UBoxComponent;
class UPrimitiveComponent;

UCLASS()
class FIREFIGHTSUMUL_API AEvacuationExitZone : public AActor
{
	GENERATED_BODY()

public:
	AEvacuationExitZone();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleExitZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evacuation",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> ExitZone;
};
