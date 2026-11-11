#pragma once

#include "CoreMinimal.h"
#include "MapObjects/MapObject.h"
#include "FireAlarmActor.generated.h"

class UAudioComponent;
class UFireScenarioDataAsset;
class URectLightComponent;
class UStaticMeshComponent;

/**
 * World-space fire alarm that plays once when a fire scenario starts.
 * Place a Blueprint subclass in the level and assign its sound/attenuation assets.
 */
UCLASS(Blueprintable)
class FIREFIGHTSUMUL_API AFireAlarmActor : public AMapObject
{
	GENERATED_BODY()

public:
	AFireAlarmActor();

	UFUNCTION(BlueprintCallable, Category = "Fire Alarm")
	void PlayAlarmOnce();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleScenarioStarted();
	bool MatchesScenario(const UFireScenarioDataAsset& ScenarioData) const;
	void StartLightFlashing();
	void StopLightFlashing();
	void ToggleAlarmLight();
	void HandleAlarmFinished(UAudioComponent* FinishedComponent);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fire Alarm",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> AlarmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fire Alarm",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAudioComponent> AlarmAudio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fire Alarm",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URectLightComponent> AlarmLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire Alarm|Light",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.05", Units = "s"))
	float LightFlashIntervalSeconds = 0.25f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Fire Alarm",
		meta = (AllowPrivateAccess = "true"))
	bool bHasPlayed = false;

	bool bAlarmLightOn = false;
	FTimerHandle LightFlashTimer;
};
