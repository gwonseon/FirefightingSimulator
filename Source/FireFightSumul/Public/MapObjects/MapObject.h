#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapObject.generated.h"

class USceneComponent;

UCLASS(Abstract, Blueprintable)
class FIREFIGHTSUMUL_API AMapObject : public AActor
{
	GENERATED_BODY()

public:
	AMapObject();

private:
	UPROPERTY(VisibleAnywhere, Category = "Map Object")
	TObjectPtr<USceneComponent> SceneRoot;
};
