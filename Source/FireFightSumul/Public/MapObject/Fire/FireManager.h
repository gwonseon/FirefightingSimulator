#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "FireManager.generated.h"

class AFireActor;
class ATargetPoint;

UCLASS()
class FIREFIGHTSUMUL_API UFireManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	void ToggleFireCollision();
	bool IsFireCollisionEnabled() const { return bFireCollisionEnabled; }
	bool HasFinishedTraining() const { return bTrainingFinished; }
	void NotifyFireExtinguished();
	void NotifyExtinguisherDepleted();

private:
	void FinishTraining(bool bSuccess);
	void IgniteRandomPoints(int32 Count);
	bool IgnitePoint(const ATargetPoint& Point, float InitialIntensityRatio);
	void UpdateSpreadReadiness(float IntensityRatio, TWeakObjectPtr<AFireActor> Source);
	void SpreadFires();
	bool TrySpreadFrom(AFireActor& Source);

	TArray<TWeakObjectPtr<ATargetPoint>> UnusedIgnitionPoints;
	TArray<TWeakObjectPtr<ATargetPoint>> UnusedSpreadPoints;
	TMap<TWeakObjectPtr<AFireActor>, double> SpreadWaitStartTimes;
	FTimerHandle IgnitionTimer;
	FTimerHandle SpreadTimer;
	bool bFireCollisionEnabled = true;
	bool bTrainingWorld = false;
	bool bTrainingFinished = false;
};
