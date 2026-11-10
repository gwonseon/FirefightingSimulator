#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TrainingResultGameMode.generated.h"

class UUserWidget;

UCLASS()
class ATrainingResultGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATrainingResultGameMode();
	static void OpenResultLevel(const UObject* WorldContext, bool bSuccess);
	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	UPROPERTY()
	TSubclassOf<UUserWidget> SuccessWidgetClass;

	UPROPERTY()
	TSubclassOf<UUserWidget> FailureWidgetClass;
};
