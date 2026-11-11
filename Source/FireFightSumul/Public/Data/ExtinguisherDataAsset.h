#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Training/FireTrainingTypes.h"
#include "ExtinguisherDataAsset.generated.h"

class UNiagaraSystem;
class USoundBase;
class UStaticMesh;
class UCurveFloat;

UCLASS(BlueprintType)
class FIREFIGHTSUMUL_API UExtinguisherDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", AssetRegistrySearchable)
	FName ExtinguisherId = TEXT("ABC_3_3kg");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rating", meta = (ClampMin = "0.0", Units = "kg"))
	float AgentMassKg = 3.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rating", meta = (ClampMin = "0"))
	int32 ARating = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rating", meta = (ClampMin = "0"))
	int32 BRating = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rating")
	bool bSupportsClassC = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rating")
	bool bSupportsClassK = false;

	UFUNCTION(BlueprintPure, Category = "Extinguisher|Rating")
	bool SupportsFireClass(EFireClass FireClass) const
	{
		switch (FireClass)
		{
		case EFireClass::A:
			return ARating > 0;
		case EFireClass::B:
			return BRating > 0;
		case EFireClass::C:
			return bSupportsClassC;
		case EFireClass::K:
			return bSupportsClassK;
		default:
			return false;
		}
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance", meta = (ClampMin = "0.1", Units = "s"))
	float DischargeDurationSeconds = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance", meta = (ClampMin = "0.0", Units = "cm"))
	float MinSafeDistanceCm = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxEffectiveDistanceCm = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "Fraction of a full fire intensity removed per second under ideal conditions."))
	float BaseSuppressionPerSecond = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance|Class K",
		meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "Temporary suppression multiplier applied to Class K fires when this extinguisher is not Class K rated. Set to zero for no effect."))
	float UnsupportedClassKTemporarySuppressionMultiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowAgentStartRatio = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Curves")
	TObjectPtr<UCurveFloat> DistanceCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Curves")
	TObjectPtr<UCurveFloat> ConeCenterCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Curves")
	TObjectPtr<UCurveFloat> AimHeightCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Curves")
	TObjectPtr<UCurveFloat> RemainingAgentCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<UStaticMesh> ExtinguisherMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<UNiagaraSystem> SprayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<USoundBase> SpraySound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Documentation")
	FSimulationSourceInfo SourceInfo;
};
