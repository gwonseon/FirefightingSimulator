#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Training/FireTrainingTypes.h"
#include "TrainingResultWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;

UCLASS(Abstract, Blueprintable)
class FIREFIGHTSUMUL_API UTrainingResultWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> AccentLine;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ResultIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultDescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimeLabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimeValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AgentLabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AgentValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ScenarioSelectButton;

private:
	void ApplyTrainingResult(const FTrainingResult& Result);

	UFUNCTION()
	void HandleScenarioSelectClicked();
};
