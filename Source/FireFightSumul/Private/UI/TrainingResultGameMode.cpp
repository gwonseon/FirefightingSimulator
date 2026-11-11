#include "UI/TrainingResultGameMode.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ATrainingResultGameMode::ATrainingResultGameMode()
{
	DefaultPawnClass = nullptr;

	static const ConstructorHelpers::FClassFinder<UUserWidget> SuccessWidget(TEXT("/Game/_UI/WBP_EndSuccess"));
	static const ConstructorHelpers::FClassFinder<UUserWidget> FailureWidget(TEXT("/Game/_UI/WBP_EndFail"));
	SuccessWidgetClass = SuccessWidget.Class;
	FailureWidgetClass = FailureWidget.Class;
}

void ATrainingResultGameMode::OpenResultLevel(const UObject* WorldContext, bool bSuccess)
{
	const FString Options = FString::Printf(TEXT("game=%s?TrainingResult=%s"),
		*StaticClass()->GetPathName(), bSuccess ? TEXT("Success") : TEXT("Fail"));
	UGameplayStatics::OpenLevel(WorldContext, TEXT("/Game/_Map/end"), true, Options);
}

void ATrainingResultGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (!NewPlayer || !NewPlayer->GetLocalPlayer())
	{
		return;
	}

	const FString Result = UGameplayStatics::ParseOption(OptionsString, TEXT("TrainingResult"));
	const TSubclassOf<UUserWidget> WidgetClass = Result == TEXT("Success") ? SuccessWidgetClass : FailureWidgetClass;
	if (UUserWidget* Widget = CreateWidget<UUserWidget>(NewPlayer, WidgetClass))
	{
		Widget->AddToViewport();
		NewPlayer->SetInputMode(FInputModeUIOnly());
		NewPlayer->bShowMouseCursor = true;
	}
}
