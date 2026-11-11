#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Training/FireTrainingTypes.h"
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

	UFUNCTION(BlueprintCallable, Category = "Fire Training|Result")
	void SetTrainingResult(const FTrainingResult& InResult);

	UFUNCTION(BlueprintPure, Category = "Fire Training|Result")
	FTrainingResult GetTrainingResult() const { return TrainingResult; }

	UFUNCTION(BlueprintPure, Category = "Fire Training|Result")
	bool HasTrainingResult() const { return bHasTrainingResult; }

	UFUNCTION(BlueprintCallable, Category = "Fire Training|Result")
	void ClearTrainingResult();

private:
	UPROPERTY(Transient)
	TObjectPtr<UFireScenarioDataAsset> SelectedScenario;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Fire Training|Result",
		meta = (AllowPrivateAccess = "true"))
	FTrainingResult TrainingResult;

	UPROPERTY(Transient)
	bool bHasTrainingResult = false;
};
