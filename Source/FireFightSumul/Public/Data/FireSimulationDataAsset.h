#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Training/FireTrainingTypes.h"
#include "FireSimulationDataAsset.generated.h"

UCLASS(BlueprintType)
class FIREFIGHTSUMUL_API UFireSimulationDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Growth", meta = (ClampMin = "0.0"))
	float DefaultBaseGrowthPerSecond = 0.025f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.05", Units = "s"))
	float SpreadCheckInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.0", Units = "cm"))
	float SpreadDistanceCm = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.0"))
	float SpreadHeatPerSecond = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.0"))
	float SpreadCoolingPerSecond = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread",
		meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float SpreadFireInitialIntensityRatio = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ventilation", meta = (ClampMin = "0.0"))
	float SealedVentilationFactor = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ventilation", meta = (ClampMin = "0.0"))
	float OpenVentilationFactor = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	TArray<FFireMaterialSetting> MaterialSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Documentation")
	FSimulationSourceInfo SourceInfo;
};
