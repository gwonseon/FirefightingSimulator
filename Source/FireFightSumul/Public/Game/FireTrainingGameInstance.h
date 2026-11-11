#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "FireTrainingGameInstance.generated.h"

class UFireScenarioDataAsset;

UCLASS()
class FIREFIGHTSUMUL_API UFireTrainingGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Fire Training|Navigation")
	bool SelectScenario(UFireScenarioDataAsset* InScenario);

	UFUNCTION(BlueprintPure, Category = "Fire Training|Navigation")
	UFireScenarioDataAsset* GetSelectedScenario() const { return SelectedScenario; }

	UFUNCTION(BlueprintCallable, Category = "Fire Training|Navigation")
	bool StartSelectedScenario();

	UFUNCTION(BlueprintCallable, Category = "Fire Training|Navigation")
	void OpenScenarioSelect();

	UFUNCTION(BlueprintCallable, Category = "Fire Training|Navigation")
	void OpenStartMenu();

private:
	UPROPERTY(Transient)
	TObjectPtr<UFireScenarioDataAsset> SelectedScenario;
};
