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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Growth",
		meta = (DeprecatedProperty, DeprecationMessage = "Use the time-to-peak settings instead."))
	float DefaultBaseGrowthPerSecond = 0.025f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Growth", meta = (ClampMin = "0.1", Units = "s"))
	float TrainingGrowthToPeakSeconds = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Growth", meta = (ClampMin = "0.1", Units = "s"))
	float RealisticGrowthToPeakSeconds = 484.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.05", Units = "s"))
	float SpreadCheckInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.0", Units = "cm"))
	float SpreadDistanceCm = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread",
		meta = (DeprecatedProperty, DeprecationMessage = "Use SpreadIgnitionExposureSeconds instead."))
	float SpreadHeatPerSecond = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.1", Units = "s"))
	float SpreadIgnitionExposureSeconds = 12.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.0"))
	float SpreadCoolingPerSecond = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Reignition",
		meta = (ClampMin = "0.0", Units = "s"))
	float SpreadReignitionCooldownMinSeconds = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Reignition",
		meta = (ClampMin = "0.0", Units = "s"))
	float SpreadReignitionCooldownMaxSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Reignition",
		meta = (ClampMin = "0.0"))
	float SpreadReignitionMinExposure = 0.35f;

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
