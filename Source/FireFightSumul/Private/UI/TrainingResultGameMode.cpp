#include "UI/TrainingResultGameMode.h"

#include "GameFramework/PlayerController.h"
#include "Game/FireTrainingGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "UI/TrainingResultWidget.h"
#include "UObject/ConstructorHelpers.h"

ATrainingResultGameMode::ATrainingResultGameMode()
{
	DefaultPawnClass = nullptr;

	static const ConstructorHelpers::FClassFinder<UTrainingResultWidget> ResultWidget(
		TEXT("/Game/_UI/WBP_TrainingResult"));
	ResultWidgetClass = ResultWidget.Class;
}

void ATrainingResultGameMode::OpenResultLevel(const UObject* WorldContext, const FTrainingResult& Result)
{
	if (!WorldContext)
	{
		return;
	}

	if (UFireTrainingGameInstance* TrainingGameInstance =
		Cast<UFireTrainingGameInstance>(UGameplayStatics::GetGameInstance(WorldContext)))
	{
		TrainingGameInstance->SetTrainingResult(Result);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot open the result level without FireTrainingGameInstance."));
		return;
	}

	const FString Options = FString::Printf(TEXT("game=%s"), *StaticClass()->GetPathName());
	UGameplayStatics::OpenLevel(WorldContext, TEXT("/Game/_Map/end"), true, Options);
}

void ATrainingResultGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (!NewPlayer || !NewPlayer->GetLocalPlayer())
	{
		return;
	}

	if (!ResultWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("WBP_TrainingResult could not be loaded."));
		return;
	}

	if (UTrainingResultWidget* Widget = CreateWidget<UTrainingResultWidget>(NewPlayer, ResultWidgetClass))
	{
		Widget->AddToViewport();
		NewPlayer->SetInputMode(FInputModeUIOnly());
		NewPlayer->bShowMouseCursor = true;
	}
}
