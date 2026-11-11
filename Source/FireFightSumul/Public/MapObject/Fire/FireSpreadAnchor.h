#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireSpreadAnchor.generated.h"

class USphereComponent;

UCLASS()
class FIREFIGHTSUMUL_API AFireSpreadAnchor : public AActor
{
	GENERATED_BODY()

public:
	AFireSpreadAnchor();

	FName GetMaterialTag() const { return MaterialTag; }
	bool CanIgnite() const { return bCanIgnite; }
	bool IsIgnited() const { return bIgnited; }
	float GetAccumulatedHeat() const { return AccumulatedHeat; }

	void ResetSpreadState();
	void AddHeat(float Amount);
	void MarkIgnited();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fire Spread",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> EditorMarker;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Fire Spread",
		meta = (AllowPrivateAccess = "true"))
	FName MaterialTag = TEXT("Wood");

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Fire Spread",
		meta = (AllowPrivateAccess = "true"))
	bool bCanIgnite = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Fire Spread|Runtime",
		meta = (AllowPrivateAccess = "true"))
	float AccumulatedHeat = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Fire Spread|Runtime",
		meta = (AllowPrivateAccess = "true"))
	bool bIgnited = false;
};
