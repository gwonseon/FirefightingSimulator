#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TrainingResultWidget.generated.h"

class UButton;

UCLASS(Abstract, Blueprintable)
class FIREFIGHTSUMUL_API UTrainingResultWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ScenarioSelectButton;

private:
	UFUNCTION()
	void HandleScenarioSelectClicked();
};
