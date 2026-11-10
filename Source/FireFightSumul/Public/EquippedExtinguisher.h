#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EquippedExtinguisher.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class FIREFIGHTSUMUL_API AEquippedExtinguisher : public AActor
{
	GENERATED_BODY()

public:
	AEquippedExtinguisher();

private:
	UPROPERTY(VisibleAnywhere, Category = "Extinguisher")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Extinguisher")
	TObjectPtr<UStaticMeshComponent> HoseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extinguisher", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> NozzlePivot;

	UPROPERTY(VisibleAnywhere, Category = "Extinguisher")
	TObjectPtr<UStaticMeshComponent> NozzleMesh;
};
