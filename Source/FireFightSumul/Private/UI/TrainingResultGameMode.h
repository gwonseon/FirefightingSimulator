#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Training/FireTrainingTypes.h"
#include "TrainingResultGameMode.generated.h"

class UTrainingResultWidget;

UCLASS()
class ATrainingResultGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATrainingResultGameMode();
	static void OpenResultLevel(const UObject* WorldContext, const FTrainingResult& Result);
	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	UPROPERTY()
	TSubclassOf<UTrainingResultWidget> ResultWidgetClass;
};
