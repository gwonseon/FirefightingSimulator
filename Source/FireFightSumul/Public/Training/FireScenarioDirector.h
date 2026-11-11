#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireScenarioDirector.generated.h"

class UFireScenarioDataAsset;
class USceneComponent;

UCLASS(Blueprintable)
class FIREFIGHTSUMUL_API AFireScenarioDirector : public AActor
{
	GENERATED_BODY()

public:
	AFireScenarioDirector();

	const UFireScenarioDataAsset* GetScenarioData() const { return ScenarioData; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Scenario")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Scenario",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFireScenarioDataAsset> ScenarioData;
};
