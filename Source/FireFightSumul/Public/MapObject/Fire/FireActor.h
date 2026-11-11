#pragma once

#include "CoreMinimal.h"
#include "MapObjects/MapObject.h"
#include "Training/FireTrainingTypes.h"
#include "FireActor.generated.h"

class UNiagaraComponent;
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
	void SetFireClass(EFireClass InFireClass) { FireClass = InFireClass; }
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

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void SetIntensity(float NewIntensity);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fire", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> FireEffect;

	UPROPERTY(VisibleAnywhere, Category = "Fire")
	TObjectPtr<USphereComponent> AimSphere;

	UPROPERTY(EditDefaultsOnly, Category = "Fire", meta = (ClampMin = "1.0"))
	float MaxIntensity = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fire|Classification",
		meta = (AllowPrivateAccess = "true"))
	EFireClass FireClass = EFireClass::A;

	UPROPERTY(EditDefaultsOnly, Category = "Fire|Recovery", meta = (ClampMin = "0.0", Units = "s"))
	float RecoveryDelay = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire|Recovery", meta = (ClampMin = "0.0"))
	float RecoveryRate = 10.0f;

	UPROPERTY(VisibleInstanceOnly, Category = "Fire")
	float CurrentIntensity = 0.0f;

	float InitialIntensityRatio = 1.0f;
	double LastSuppressionTime = 0.0;
	double RecoveryStartTime = 0.0;
	FVector InitialEffectScale = FVector::OneVector;
};
