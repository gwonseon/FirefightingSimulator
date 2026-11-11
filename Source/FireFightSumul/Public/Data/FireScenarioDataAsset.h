#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Training/FireTrainingTypes.h"
#include "FireScenarioDataAsset.generated.h"

class UExtinguisherDataAsset;
class UFireSimulationDataAsset;
class UWorld;

UCLASS(BlueprintType)
class FIREFIGHTSUMUL_API UFireScenarioDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", AssetRegistrySearchable)
	FName ScenarioId = TEXT("TrashBin");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText ScenarioName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TSoftObjectPtr<UWorld> ScenarioLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Start")
	FName StartPointTag = TEXT("ScenarioStart.TrashBin");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Start")
	EFireStartSelectionMode StartSelectionMode = EFireStartSelectionMode::Fixed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Start",
		meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float InitialFireIntensityRatio = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "References")
	TObjectPtr<UExtinguisherDataAsset> DefaultExtinguisher;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "References")
	TObjectPtr<UFireSimulationDataAsset> FireRules;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	TArray<FTrainingFeedbackSetting> FeedbackSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float ExtinguishmentObservationSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float EvacuationGraceSeconds = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0", Units = "s"))
	float ScenarioTimeoutSeconds = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limits", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UncontrollableFireSize = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limits", meta = (ClampMin = "1"))
	int32 UncontrollableFireCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limits", meta = (ClampMin = "0.0", Units = "cm"))
	float EvacuationSpreadDistanceCm = 400.0f;
};
