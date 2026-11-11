#pragma once

#include "CoreMinimal.h"
#include "MapObjects/MapObject.h"
#include "Training/FireTrainingTypes.h"
#include "FireActor.generated.h"

class UNiagaraComponent;
class UPrimitiveComponent;
class USphereComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFireIntensityChanged, float);

UCLASS()
class FIREFIGHTSUMUL_API AFireActor : public AMapObject
{
	GENERATED_BODY()

public:
	AFireActor();
	static const FName AimTag;

	void SetInitialIntensityRatio(float Ratio);
	void ConfigureMaterial(FName InMaterialTag, EFireClass InFireClass,
		float InGrowthMultiplier, float InSuppressionMultiplier);
	void SetGrowthToPeakSeconds(float Seconds);
	FName GetMaterialTag() const { return MaterialTag; }
	EFireClass GetFireClass() const { return FireClass; }
	float GetIntensityRatio() const;
	double GetLastSuppressionTime() const { return LastSuppressionTime; }
	void SetAimCollisionEnabled(bool bEnabled);
	const USphereComponent* GetAimSphere() const { return AimSphere.Get(); }
	FOnFireIntensityChanged OnIntensityChanged;

	UFUNCTION(BlueprintCallable, Category = "Fire")
	void ApplySuppression(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Fire")
	void ApplySuppressionRatio(float RatioAmount);

	bool ReigniteFromNearbyFire(float StartIntensityRatio);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UFUNCTION()
	void HandleHeatOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleHeatOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void RefreshHeatedWindows();
	void ApplyHeatDamage();
	void SetHeatCollisionEnabled(bool bEnabled);
	bool IsWindowHeatOverlap(const AActor* OtherActor, const UPrimitiveComponent* OtherComp) const;
	void SetIntensity(float NewIntensity);
	void BeginGrowth(double StartTime);
	void ScheduleReignition();
	void TryReignite();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fire", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> FireEffect;

	UPROPERTY(VisibleAnywhere, Category = "Fire")
	TObjectPtr<USphereComponent> AimSphere;

	UPROPERTY(VisibleAnywhere, Category = "Fire|Heat")
	TObjectPtr<USphereComponent> HeatCollision;

	UPROPERTY(EditDefaultsOnly, Category = "Fire|Heat",
		meta = (ClampMin = "0.02", Units = "s"))
	float HeatDamageInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire|Heat",
		meta = (ClampMin = "0.0"))
	float DefaultWindowFireDamagePerSecond = 40.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire", meta = (ClampMin = "1.0"))
	float MaxIntensity = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire|Classification",
		meta = (AllowPrivateAccess = "true"))
	EFireClass FireClass = EFireClass::A;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Fire|Classification",
		meta = (AllowPrivateAccess = "true"))
	FName MaterialTag = NAME_None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Fire|Classification",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float GrowthMultiplier = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Fire|Classification",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SuppressionMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire|Recovery", meta = (ClampMin = "0.0", Units = "s"))
	float RecoveryDelay = 3.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Fire|Growth")
	float GrowthToPeakSeconds = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire|Reignition", meta = (ClampMin = "0.1", Units = "s"))
	float ReignitionDelay = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire|Reignition",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReignitionChance = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire|Reignition",
		meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float ReignitionStartRatio = 0.01f;

	UPROPERTY(VisibleInstanceOnly, Category = "Fire")
	float CurrentIntensity = 0.0f;

	float InitialIntensityRatio = 1.0f;
	double LastSuppressionTime = 0.0;
	double RecoveryStartTime = 0.0;
	float GrowthStartIntensityRatio = 1.0f;
	FVector InitialEffectScale = FVector::OneVector;
	TSet<TWeakObjectPtr<AActor>> HeatedWindows;
	FTimerHandle HeatDamageTimer;
	FTimerHandle ReignitionTimer;
};
