#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TrainingStartWidget.generated.h"

class UButton;

UCLASS(Abstract, Blueprintable)
class FIREFIGHTSUMUL_API UTrainingStartWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartButton;

private:
	UFUNCTION()
	void HandleStartClicked();
};
