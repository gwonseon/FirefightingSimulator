#include "UI/TrainingResultWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Game/FireTrainingGameInstance.h"

namespace
{
	const FName TrainingStringTableId(TEXT("/Game/_Data/ST_FireTraining.ST_FireTraining"));

	FText GetTrainingText(const TCHAR* Key)
	{
		return FText::FromStringTable(TrainingStringTableId, Key);
	}

	FLinearColor ColorFromHex(const TCHAR* Hex)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(FString(Hex)));
	}

	FText ResolveTitle(const FTrainingResult& Result)
	{
		switch (Result.Outcome)
		{
		case ETrainingOutcome::Extinguished:
			return GetTrainingText(TEXT("Result.Extinguished"));
		case ETrainingOutcome::Evacuated:
			return GetTrainingText(TEXT("Result.Evacuated"));
		case ETrainingOutcome::Failed:
			switch (Result.FailureReason)
			{
			case ETrainingFailureReason::EnteredDangerZone:
				return GetTrainingText(TEXT("Result.Title.DangerZone"));
			case ETrainingFailureReason::IgnoredEvacuation:
				return GetTrainingText(TEXT("Result.Title.IgnoredEvacuation"));
			case ETrainingFailureReason::ScenarioTimeout:
			default:
				return GetTrainingText(TEXT("Result.Title.Timeout"));
			}
		default:
			return FText::GetEmpty();
		}
	}

	FText ResolveFallbackDescription(const FTrainingResult& Result)
	{
		if (Result.Outcome == ETrainingOutcome::Extinguished)
		{
			return GetTrainingText(TEXT("Result.Description.Extinguished"));
		}
		if (Result.Outcome == ETrainingOutcome::Evacuated)
		{
			return GetTrainingText(TEXT("Result.Description.Evacuated"));
		}
		if (Result.FailureReason == ETrainingFailureReason::EnteredDangerZone)
		{
			return GetTrainingText(TEXT("Result.FailedDangerZone"));
		}
		if (Result.FailureReason == ETrainingFailureReason::IgnoredEvacuation)
		{
			return GetTrainingText(TEXT("Result.FailedEvacuation"));
		}
		return GetTrainingText(TEXT("Result.FailedTimeout"));
	}

	const TCHAR* ResolveIconPath(const FTrainingResult& Result)
	{
		if (Result.Outcome == ETrainingOutcome::Extinguished)
		{
			return TEXT("/Game/_Image/Icon_Result_Extinguished_local.Icon_Result_Extinguished_local");
		}
		if (Result.Outcome == ETrainingOutcome::Evacuated)
		{
			return TEXT("/Game/_Image/Icon_Result_Evacuated_local.Icon_Result_Evacuated_local");
		}

		switch (Result.FailureReason)
		{
		case ETrainingFailureReason::EnteredDangerZone:
			return TEXT("/Game/_Image/Icon_Result_DangerZone_local.Icon_Result_DangerZone_local");
		case ETrainingFailureReason::IgnoredEvacuation:
			return TEXT("/Game/_Image/Icon_Result_IgnoredEvacuation_local.Icon_Result_IgnoredEvacuation_local");
		case ETrainingFailureReason::ScenarioTimeout:
		default:
			return TEXT("/Game/_Image/Icon_Result_Timeout_local.Icon_Result_Timeout_local");
		}
	}
}

void UTrainingResultWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (ScenarioSelectButton)
	{
		ScenarioSelectButton->OnClicked.AddUniqueDynamic(
			this, &UTrainingResultWidget::HandleScenarioSelectClicked);
	}
}

void UTrainingResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (const UFireTrainingGameInstance* TrainingGameInstance =
		GetGameInstance<UFireTrainingGameInstance>();
		TrainingGameInstance && TrainingGameInstance->HasTrainingResult())
	{
		ApplyTrainingResult(TrainingGameInstance->GetTrainingResult());
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("WBP_TrainingResult opened without a stored training result. Keeping designer preview values."));
	}
}

void UTrainingResultWidget::ApplyTrainingResult(const FTrainingResult& Result)
{
	const FLinearColor AccentColor = Result.Outcome == ETrainingOutcome::Evacuated
		? ColorFromHex(TEXT("46C96B"))
		: Result.Outcome == ETrainingOutcome::Failed
			? ColorFromHex(TEXT("E85643"))
			: ColorFromHex(TEXT("FFC247"));

	if (HeaderText)
	{
		HeaderText->SetText(GetTrainingText(TEXT("Result.Header")));
	}
	if (AccentLine)
	{
		AccentLine->SetBrushColor(AccentColor);
	}
	if (ResultIcon)
	{
		if (UTexture2D* IconTexture = LoadObject<UTexture2D>(nullptr, ResolveIconPath(Result)))
		{
			ResultIcon->SetBrushFromTexture(IconTexture, true);
		}
		ResultIcon->SetColorAndOpacity(AccentColor);
	}
	if (ResultTitleText)
	{
		ResultTitleText->SetText(ResolveTitle(Result));
		ResultTitleText->SetColorAndOpacity(FSlateColor(AccentColor));
	}
	if (ResultDescriptionText)
	{
		ResultDescriptionText->SetText(Result.ResultMessage.IsEmpty()
			? ResolveFallbackDescription(Result) : Result.ResultMessage);
	}

	const bool bFailed = Result.Outcome == ETrainingOutcome::Failed;
	if (TimeLabelText)
	{
		TimeLabelText->SetText(GetTrainingText(bFailed
			? TEXT("Result.Label.FailureTime") : TEXT("Result.Label.CompletionTime")));
	}
	if (TimeValueText)
	{
		const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(Result.CompletionTime));
		const int32 Minutes = TotalSeconds / 60;
		const int32 Seconds = TotalSeconds % 60;
		TimeValueText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
	}
	if (AgentLabelText)
	{
		AgentLabelText->SetText(GetTrainingText(TEXT("Result.Label.AgentUsed")));
	}
	if (AgentValueText)
	{
		FNumberFormattingOptions NumberOptions;
		NumberOptions.MinimumFractionalDigits = 1;
		NumberOptions.MaximumFractionalDigits = 1;
		const FText AgentAmount = FText::AsNumber(FMath::Max(0.0f, Result.UsedAgentKg), &NumberOptions);
		AgentValueText->SetText(FText::Format(
			GetTrainingText(TEXT("Result.Value.AgentKg")), AgentAmount));
	}
}

void UTrainingResultWidget::HandleScenarioSelectClicked()
{
	if (UFireTrainingGameInstance* TrainingGameInstance = GetGameInstance<UFireTrainingGameInstance>())
	{
		TrainingGameInstance->OpenScenarioSelect();
	}
}
