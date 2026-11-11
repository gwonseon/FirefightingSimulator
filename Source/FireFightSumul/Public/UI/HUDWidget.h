#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class UBorder;
class UImage;
class UProgressBar;
class USizeBox;
class USpacer;
class UTextBlock;

UCLASS(Abstract, Blueprintable)
class FIREFIGHTSUMUL_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetCrosshairVisible(bool bVisible);

	void SetFireStatus(float IntensityRatio);
	void ShowFireExtinguished();
	void HideFireStatus();
	void ShowFireUncontrollable();
	void ResetFireUncontrollable();
	void SetExtinguisherStatus(bool bVisible, float AgentRatio);
	void SetTrainingObjective(const FText& Objective);
	void ShowActionHint(const FText& Hint, bool bShowCancel);
	void HideActionHint();
	void ShowTrainingFeedback(const FText& Message, bool bPositive);
	void HideTrainingFeedback();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CrosshairImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> FireStatusPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> FireIntensityBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FirePercentText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> FireUncontrollablePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> ExtinguisherStatusPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ExtinguisherChargeBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpacer> ExtinguisherBodyLeftSpacer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> ExtinguisherGaugeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpacer> ExtinguisherGaugePercentSpacer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ExtinguisherPercentText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> TrainingObjectivePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> ActionHintPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActionHintText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CancelHintText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> TrainingFeedbackPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TrainingFeedbackText;

	bool bFireUncontrollable = false;
};
