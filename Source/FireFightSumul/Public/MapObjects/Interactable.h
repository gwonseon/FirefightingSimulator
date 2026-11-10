#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class AFireFighter;

UINTERFACE(BlueprintType, Blueprintable)
class FIREFIGHTSUMUL_API UInteractable : public UInterface
{
	GENERATED_BODY()
};

class FIREFIGHTSUMUL_API IInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void Interact(AFireFighter* Player);
};
