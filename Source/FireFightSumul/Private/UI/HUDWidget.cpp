#include "UI/HUDWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"

void UHUDWidget::SetCrosshairVisible(bool bVisible)
{
	CrosshairImage->SetVisibility(
		bVisible ? ESlateVisibility::HitTestInvisible
		         : ESlateVisibility::Collapsed);
}

void UHUDWidget::SetFireStatus(float IntensityRatio)
{
	if (bFireUncontrollable)
	{
		return;
	}

	const float Ratio = FMath::Clamp(IntensityRatio, 0.0f, 1.0f);
	FNumberFormattingOptions PercentFormat;
	PercentFormat.SetMaximumFractionalDigits(0);
	FirePercentText->SetText(FText::AsPercent(Ratio, &PercentFormat));
	FireIntensityBar->SetPercent(Ratio);
	FireIntensityBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	FireStatusPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHUDWidget::ShowFireExtinguished()
{
	if (bFireUncontrollable)
	{
		return;
	}

	FirePercentText->SetText(NSLOCTEXT("FireHUD", "Extinguished", "진압 완료"));
	FireIntensityBar->SetVisibility(ESlateVisibility::Collapsed);
	FireStatusPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHUDWidget::HideFireStatus()
{
	if (bFireUncontrollable)
	{
		return;
	}

	FireStatusPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UHUDWidget::ShowFireUncontrollable()
{
	bFireUncontrollable = true;
	FireStatusPanel->SetVisibility(ESlateVisibility::Collapsed);
	FireUncontrollablePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHUDWidget::ResetFireUncontrollable()
{
	bFireUncontrollable = false;
	FireStatusPanel->SetVisibility(ESlateVisibility::Collapsed);
	FireUncontrollablePanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UHUDWidget::SetExtinguisherStatus(bool bVisible, float AgentRatio)
{
	if (!bVisible)
	{
		ExtinguisherStatusPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const float Ratio = FMath::Clamp(AgentRatio, 0.0f, 1.0f);
	const bool bEmpty = Ratio <= KINDA_SMALL_NUMBER;
	const ESlateVisibility GaugeVisibility = bEmpty
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible;
	ExtinguisherBodyLeftSpacer->SetVisibility(GaugeVisibility);
	ExtinguisherGaugeBox->SetVisibility(GaugeVisibility);
	ExtinguisherGaugePercentSpacer->SetVisibility(GaugeVisibility);

	const FLinearColor Green = FLinearColor::FromSRGBColor(FColor(53, 200, 90));
	const FLinearColor Orange = FLinearColor::FromSRGBColor(FColor(255, 152, 31));
	const FLinearColor Red = FLinearColor::FromSRGBColor(FColor(240, 68, 68));
	constexpr float MidCharge = 0.5f;
	constexpr float LowCharge = 0.15f;
	const FLinearColor ChargeColor = Ratio >= MidCharge
		? FMath::Lerp(Orange, Green, (Ratio - MidCharge) / (1.0f - MidCharge))
		: FMath::Lerp(Red, Orange, FMath::Clamp((Ratio - LowCharge) / (MidCharge - LowCharge), 0.0f, 1.0f));

	FNumberFormattingOptions PercentFormat;
	PercentFormat.SetMaximumFractionalDigits(0);
	ExtinguisherPercentText->SetText(!bEmpty
		? FText::AsPercent(Ratio, &PercentFormat)
		: NSLOCTEXT("ExtinguisherHUD", "Empty", "약제 없음"));
	ExtinguisherChargeBar->SetPercent(Ratio);
	ExtinguisherChargeBar->SetFillColorAndOpacity(ChargeColor);
	ExtinguisherStatusPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHUDWidget::SetTrainingObjective(const FText& Objective)
{
	if (!ObjectiveText->GetText().EqualTo(Objective))
	{
		ObjectiveText->SetText(Objective);
	}
	if (TrainingObjectivePanel->GetVisibility() != ESlateVisibility::HitTestInvisible)
	{
		TrainingObjectivePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UHUDWidget::ShowActionHint(const FText& Hint, bool bShowCancel)
{
	ActionHintText->SetText(Hint);
	CancelHintText->SetVisibility(bShowCancel ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	ActionHintPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHUDWidget::HideActionHint()
{
	ActionHintPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UHUDWidget::ShowTrainingFeedback(const FText& Message, bool bPositive)
{
	if (!TrainingFeedbackPanel || !TrainingFeedbackText)
	{
		return;
	}

	static_cast<void>(bPositive);
	TrainingFeedbackText->SetText(Message);
	TrainingFeedbackText->SetColorAndOpacity(FLinearColor::White);
	TrainingFeedbackPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHUDWidget::HideTrainingFeedback()
{
	if (TrainingFeedbackPanel)
	{
		TrainingFeedbackPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}
