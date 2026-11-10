#include "MapObject/Fire/FireManager.h"

#include "CollisionQueryParams.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "MapObject/Fire/FireActor.h"
#include "TimerManager.h"
#include "UI/TrainingResultGameMode.h"

namespace
{
	const FName SpreadTag(TEXT("spread"));
	constexpr float SpreadIntensityThreshold = 0.5f;
	constexpr double SpreadDelay = 5.0;
	constexpr double FullIntensitySpreadDelay = 3.5;
	constexpr double SpreadDistance = 250.0;
	constexpr double SpreadHeightTolerance = 25.0;
	constexpr float SpreadInitialIntensity = 0.3f;
}

void UFireManager::ToggleFireCollision()
{
	bFireCollisionEnabled = !bFireCollisionEnabled;
	for (TActorIterator<AFireActor> Fire(&GetWorldRef()); Fire; ++Fire)
	{
		Fire->SetAimCollisionEnabled(bFireCollisionEnabled);
	}
	UE_LOG(LogTemp, Log, TEXT("Fire colliders: %s"), bFireCollisionEnabled ? TEXT("On") : TEXT("Off"));
}

void UFireManager::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld() || UGameplayStatics::GetCurrentLevelName(&InWorld) != TEXT("Main"))
	{
		return;
	}
	bTrainingWorld = true;

	for (TActorIterator<ATargetPoint> Point(&InWorld); Point; ++Point)
	{
		if (Point->ActorHasTag(SpreadTag))
		{
			UnusedSpreadPoints.Add(*Point);
		}
		else
		{
			UnusedIgnitionPoints.Add(*Point);
		}
	}

	IgniteRandomPoints(1);
	if (!UnusedIgnitionPoints.IsEmpty())
	{
		constexpr float IgnitionInterval = 10.0f;
		InWorld.GetTimerManager().SetTimer(IgnitionTimer,
			FTimerDelegate::CreateUObject(this, &UFireManager::IgniteRandomPoints, 2),
			IgnitionInterval, true);
	}
	if (!UnusedSpreadPoints.IsEmpty())
	{
		constexpr float SpreadCheckInterval = 0.25f;
		InWorld.GetTimerManager().SetTimer(SpreadTimer, this, &UFireManager::SpreadFires,
			SpreadCheckInterval, true);
	}
}

void UFireManager::NotifyFireExtinguished()
{
	if (!bTrainingWorld || bTrainingFinished)
	{
		return;
	}

	for (TActorIterator<AFireActor> Fire(&GetWorldRef()); Fire; ++Fire)
	{
		if (Fire->GetIntensityRatio() > 0.0f)
		{
			return;
		}
	}

	FinishTraining(true);
}

void UFireManager::NotifyExtinguisherDepleted()
{
	FinishTraining(false);
}

void UFireManager::FinishTraining(bool bSuccess)
{
	if (!bTrainingWorld || bTrainingFinished)
	{
		return;
	}

	bTrainingFinished = true;
	FTimerManager& Timers = GetWorldRef().GetTimerManager();
	Timers.ClearTimer(IgnitionTimer);
	Timers.ClearTimer(SpreadTimer);
	ATrainingResultGameMode::OpenResultLevel(&GetWorldRef(), bSuccess);
}

void UFireManager::IgniteRandomPoints(int32 Count)
{
	while (Count > 0 && !UnusedIgnitionPoints.IsEmpty())
	{
		const int32 Index = FMath::RandHelper(UnusedIgnitionPoints.Num());
		ATargetPoint* Point = UnusedIgnitionPoints[Index].Get();
		UnusedIgnitionPoints.RemoveAtSwap(Index, 1, EAllowShrinking::No);

		if (Point && IgnitePoint(*Point, 1.0f))
		{
			--Count;
		}
	}

	if (UnusedIgnitionPoints.IsEmpty())
	{
		GetWorldRef().GetTimerManager().ClearTimer(IgnitionTimer);
	}
}

bool UFireManager::IgnitePoint(const ATargetPoint& Point, float InitialIntensityRatio)
{
	const FTransform Transform(FQuat::Identity, Point.GetActorLocation());
	AFireActor* Fire = GetWorldRef().SpawnActorDeferred<AFireActor>(AFireActor::StaticClass(), Transform);
	if (!Fire)
	{
		return false;
	}

	Fire->SetInitialIntensityRatio(InitialIntensityRatio);
	if (!UnusedSpreadPoints.IsEmpty())
	{
		Fire->OnIntensityChanged.AddUObject(this, &UFireManager::UpdateSpreadReadiness,
			TWeakObjectPtr<AFireActor>(Fire));
	}
	Fire->FinishSpawning(Transform);
	return true;
}

void UFireManager::UpdateSpreadReadiness(float IntensityRatio, TWeakObjectPtr<AFireActor> Source)
{
	if (IntensityRatio <= SpreadIntensityThreshold)
	{
		SpreadWaitStartTimes.Remove(Source);
	}
	else if (!UnusedSpreadPoints.IsEmpty() && !SpreadWaitStartTimes.Contains(Source))
	{
		SpreadWaitStartTimes.Add(Source, GetWorldRef().GetTimeSeconds());
	}
}

void UFireManager::SpreadFires()
{
	const double Now = GetWorldRef().GetTimeSeconds();
	TArray<TWeakObjectPtr<AFireActor>> ReadySources;
	for (auto It = SpreadWaitStartTimes.CreateIterator(); It; ++It)
	{
		const AFireActor* Fire = It.Key().Get();
		if (!Fire)
		{
			It.RemoveCurrent();
			continue;
		}

		const double RequiredWait = Fire->GetIntensityRatio() >= 1.0f ? FullIntensitySpreadDelay : SpreadDelay;
		const double WaitStartTime = FMath::Max(It.Value(), Fire->GetLastSuppressionTime());
		if (Now - WaitStartTime >= RequiredWait)
		{
			ReadySources.Add(It.Key());
		}
	}

	for (const TWeakObjectPtr<AFireActor>& Source : ReadySources)
	{
		if (AFireActor* Fire = Source.Get(); Fire && TrySpreadFrom(*Fire))
		{
			SpreadWaitStartTimes.Add(Source, Now);
		}
	}

	if (UnusedSpreadPoints.IsEmpty())
	{
		GetWorldRef().GetTimerManager().ClearTimer(SpreadTimer);
		SpreadWaitStartTimes.Reset();
	}
}

bool UFireManager::TrySpreadFrom(AFireActor& Source)
{
	const FVector SourceLocation = Source.GetActorLocation();
	const FVector TraceOffset(0.0, 0.0, 50.0);
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FireSpread), false, &Source);
	int32 BestIndex = INDEX_NONE;
	double BestDistanceSquared = FMath::Square(SpreadDistance);

	for (int32 Index = 0; Index < UnusedSpreadPoints.Num(); ++Index)
	{
		const ATargetPoint* Point = UnusedSpreadPoints[Index].Get();
		if (!Point)
		{
			continue;
		}

		const FVector Location = Point->GetActorLocation();
		const FVector Offset = Location - SourceLocation;
		const double DistanceSquared = Offset.SizeSquared2D();
		if (FMath::Abs(Offset.Z) > SpreadHeightTolerance || DistanceSquared <= 1.0 ||
			DistanceSquared > BestDistanceSquared)
		{
			continue;
		}

		if (!GetWorldRef().LineTraceTestByChannel(SourceLocation + TraceOffset, Location + TraceOffset,
			ECC_Visibility, QueryParams))
		{
			BestIndex = Index;
			BestDistanceSquared = DistanceSquared;
		}
	}

	if (BestIndex != INDEX_NONE && IgnitePoint(*UnusedSpreadPoints[BestIndex].Get(), SpreadInitialIntensity))
	{
		UnusedSpreadPoints.RemoveAtSwap(BestIndex, 1, EAllowShrinking::No);
		return true;
	}

	return false;
}

void UFireManager::Deinitialize()
{
	GetWorldRef().GetTimerManager().ClearAllTimersForObject(this);
	UnusedIgnitionPoints.Reset();
	UnusedSpreadPoints.Reset();
	SpreadWaitStartTimes.Reset();
	Super::Deinitialize();
}
