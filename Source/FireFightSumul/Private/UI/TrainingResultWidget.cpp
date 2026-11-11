#include "UI/TrainingResultWidget.h"

#include "Components/Button.h"
#include "Game/FireTrainingGameInstance.h"

void UTrainingResultWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (ScenarioSelectButton)
	{
		ScenarioSelectButton->OnClicked.AddUniqueDynamic(
			this, &UTrainingResultWidget::HandleScenarioSelectClicked);
	}
}

void UTrainingResultWidget::HandleScenarioSelectClicked()
{
	if (UFireTrainingGameInstance* TrainingGameInstance = GetGameInstance<UFireTrainingGameInstance>())
	{
		TrainingGameInstance->OpenScenarioSelect();
	}
}
