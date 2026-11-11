#include "UI/TrainingStartWidget.h"

#include "Components/Button.h"
#include "Game/FireTrainingGameInstance.h"

void UTrainingStartWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (StartButton)
	{
		StartButton->OnClicked.AddUniqueDynamic(this, &UTrainingStartWidget::HandleStartClicked);
	}
}

void UTrainingStartWidget::HandleStartClicked()
{
	if (UFireTrainingGameInstance* TrainingGameInstance = GetGameInstance<UFireTrainingGameInstance>())
	{
		TrainingGameInstance->OpenScenarioSelect();
	}
}
