#include "UI/TrainingMenuGameMode.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

ATrainingMenuGameModeBase::ATrainingMenuGameModeBase()
{
	DefaultPawnClass = nullptr;
}

void ATrainingMenuGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (!NewPlayer || !NewPlayer->GetLocalPlayer() || !MenuWidgetClass)
	{
		return;
	}

	if (UUserWidget* Widget = CreateWidget<UUserWidget>(NewPlayer, MenuWidgetClass))
	{
		Widget->AddToViewport();
		NewPlayer->SetInputMode(FInputModeUIOnly());
		NewPlayer->bShowMouseCursor = true;
	}
}

ATrainingStartGameMode::ATrainingStartGameMode()
{
	static const ConstructorHelpers::FClassFinder<UUserWidget> StartWidget(
		TEXT("/Game/_UI/WBP_TrainingStart"));
	MenuWidgetClass = StartWidget.Class;
}

AScenarioSelectGameMode::AScenarioSelectGameMode()
{
	static const ConstructorHelpers::FClassFinder<UUserWidget> SelectWidget(
		TEXT("/Game/_UI/WBP_ScenarioSelect"));
	MenuWidgetClass = SelectWidget.Class;
}
