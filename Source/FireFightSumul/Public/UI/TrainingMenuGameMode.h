#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TrainingMenuGameMode.generated.h"

class UUserWidget;

UCLASS(Abstract)
class FIREFIGHTSUMUL_API ATrainingMenuGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATrainingMenuGameModeBase();
	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	UPROPERTY()
	TSubclassOf<UUserWidget> MenuWidgetClass;
};

UCLASS()
class FIREFIGHTSUMUL_API ATrainingStartGameMode : public ATrainingMenuGameModeBase
{
	GENERATED_BODY()

public:
	ATrainingStartGameMode();
};

UCLASS()
class FIREFIGHTSUMUL_API AScenarioSelectGameMode : public ATrainingMenuGameModeBase
{
	GENERATED_BODY()

public:
	AScenarioSelectGameMode();
};
