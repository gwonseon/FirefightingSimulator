#pragma once

#include "CoreMinimal.h"
#include "MapObjects/MapObject.h"
#include "BreakableGlass.generated.h"

class UStaticMeshComponent;

UCLASS()
class FIREFIGHTSUMUL_API ABreakableGlass : public AMapObject
{
	GENERATED_BODY()

public:
	ABreakableGlass();

	UFUNCTION(BlueprintCallable, Category = "Glass")
	bool BreakGlass();

private:
	UPROPERTY(VisibleAnywhere, Category = "Glass")
	TObjectPtr<UStaticMeshComponent> GlassMesh;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Glass", meta = (AllowPrivateAccess = "true"))
	bool bIsBroken = false;
};
