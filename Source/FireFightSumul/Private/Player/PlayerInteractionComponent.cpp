#include "Player/PlayerInteractionComponent.h"

#include "AimTraceChannel.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Curves/CurveFloat.h"
#include "Data/ExtinguisherDataAsset.h"
#include "DoorInteractionLog.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "MapObject/Fire/FireActor.h"
#include "MapObject/Fire/FireManager.h"
#include "MapObjects/Interactable.h"
#include "MapObjects/LockableDoor.h"
#include "MapObjects/WorldExtinguisher.h"
#include "NiagaraSystem.h"
#include "Player/FireFighter.h"
#include "Player/FireFighterPlayerController.h"

namespace
{
	constexpr uint8 SweepLeftZone = 1 << 0;
	constexpr uint8 SweepCenterZone = 1 << 1;
	constexpr uint8 SweepRightZone = 1 << 2;
	constexpr uint8 SweepAllZones = SweepLeftZone | SweepCenterZone | SweepRightZone;

#if ENABLE_DRAW_DEBUG
	TAutoConsoleVariable<int32> CVarDrawSpray(
		TEXT("ffs.DebugSpray"), 0, TEXT("Draw the spray cone and fire collision spheres while spraying."));
#endif
}

bool UPlayerInteractionComponent::IsExtinguisherEquipped() const
{
	const AFireFighter* Player = Cast<AFireFighter>(GetOwner());
	return Player && Player->GetEquippedNozzle() && EquippedExtinguisher.IsValid();
}

const AWorldExtinguisher* UPlayerInteractionComponent::GetEquippedExtinguisher() const
{
	return EquippedExtinguisher.Get();
}

float UPlayerInteractionComponent::GetAgentRatio() const
{
	const AWorldExtinguisher* Extinguisher = EquippedExtinguisher.Get();
	return Extinguisher ? Extinguisher->GetAgentRatio() : 0.0f;
}

bool UPlayerInteractionComponent::EquipExtinguisher(AWorldExtinguisher& Extinguisher)
{
	AFireFighter* Player = Cast<AFireFighter>(GetOwner());
	if (!Player)
	{
		return false;
	}

	const bool bWasHoseEquipped = Player->GetEquippedNozzle() != nullptr;
	Player->SetHoseEquipped(true);
	if (!Player->GetEquippedNozzle())
	{
		return false;
	}
	AWorldExtinguisher* PreviousExtinguisher = EquippedExtinguisher.Get();
	if (PreviousExtinguisher && PreviousExtinguisher != &Extinguisher && !PreviousExtinguisher->TryDropNear(*Player))
	{
		Player->SetHoseEquipped(bWasHoseEquipped);
		return false;
	}

	Extinguisher.TryRemovePin();
	EquippedExtinguisher = &Extinguisher;
	Extinguisher.SetActorHiddenInGame(true);
	Extinguisher.SetActorEnableCollision(false);
	NotifyExtinguisherStatusChanged();
	return true;
}

void UPlayerInteractionComponent::NotifyExtinguisherStatusChanged()
{
	OnExtinguisherStatusChanged.Broadcast(IsExtinguisherEquipped(), GetAgentRatio());
}

void UPlayerInteractionComponent::Interact()
{
	AFireFighter* Player = Cast<AFireFighter>(GetOwner());
	if (!Player)
	{
		return;
	}
	if (AFireFighterPlayerController* Controller = Cast<AFireFighterPlayerController>(Player->GetController());
		Controller && Controller->IsInspectingExtinguisher())
	{
		Controller->EndExtinguisherInspection();
		return;
	}
	if (ALockableDoor* Door = Cast<ALockableDoor>(FindAimedActor()); Door && Door->CanToggleLock(*Player))
	{
		IInteractable::Execute_Interact(Door, Player);
		return;
	}
	if (AWorldExtinguisher* Extinguisher = EquippedExtinguisher.Get(); Extinguisher && IsExtinguisherEquipped())
	{
		if (Extinguisher->TryDropNear(*Player))
		{
			EquippedExtinguisher.Reset();
			Player->SetHoseEquipped(false);
		}
		return;
	}

	AActor* Target = FindFocusedActor(*Player, ECC_Visibility, TEXT("E"));
	if (IsValid(Target) && Target->Implements<UInteractable>())
	{
		IInteractable::Execute_Interact(Target, Player);
	}
}

void UPlayerInteractionComponent::PrimaryInteract()
{
	EndPrimaryInteract();
	AFireFighter* Player = Cast<AFireFighter>(GetOwner());
	if (!Player)
	{
		return;
	}
	AFireFighterPlayerController* Controller = Cast<AFireFighterPlayerController>(Player->GetController());
	if (Controller && Controller->IsInspectingExtinguisher())
	{
		Controller->BeginExtinguisherPinDrag();
		return;
	}

	AActor* Target = FindFocusedActor(*Player, AimTraceChannel, TEXT("LeftClick"));
	if (AWorldExtinguisher* Extinguisher = Cast<AWorldExtinguisher>(Target); Controller && Extinguisher)
	{
		if (Extinguisher->HasRemovedPin())
		{
			EquipExtinguisher(*Extinguisher);
		}
		else
		{
			Controller->BeginExtinguisherInspection(Extinguisher);
		}
		return;
	}
	const AWorldExtinguisher* Extinguisher = EquippedExtinguisher.Get();
	bSpraying = Player->GetEquippedNozzle() && Extinguisher && Extinguisher->GetRemainingAgentKg() > 0.0f;
	if (bSpraying)
	{
		const UExtinguisherDataAsset* Data = Extinguisher->GetExtinguisherData();
		UNiagaraSystem* SpraySystem = Data ? Data->SprayEffect.LoadSynchronous() : nullptr;
		Player->StartSprayEffect(SpraySystem, Extinguisher->GetRemainingAgentPerformanceFactor());
	}
	if (!bSpraying && Extinguisher && Extinguisher->GetRemainingAgentKg() <= 0.0f)
	{
		NotifyExtinguisherStatusChanged();
		ReportTrainingFeedback(ETrainingFeedbackType::ExtinguisherEmpty);
	}
}

void UPlayerInteractionComponent::InteractWithDoor()
{
	const AFireFighter* Player = Cast<AFireFighter>(GetOwner());
	const AFireFighterPlayerController* Controller = Player
		? Cast<AFireFighterPlayerController>(Player->GetController()) : nullptr;
	if (!Player || (Controller && Controller->IsInspectingExtinguisher()))
	{
		return;
	}

	if (ALockableDoor* Door = Cast<ALockableDoor>(FindFocusedActor(*Player, AimTraceChannel, TEXT("RightClick"))))
	{
		Door->TryToggleOpen();
	}
}

void UPlayerInteractionComponent::ContinuePrimaryInteract()
{
	AFireFighter* Player = Cast<AFireFighter>(GetOwner());
	if (AFireFighterPlayerController* Controller = Player ? Cast<AFireFighterPlayerController>(Player->GetController()) : nullptr;
		Controller && Controller->IsInspectingExtinguisher())
	{
		Controller->UpdateExtinguisherPinDrag();
		return;
	}
	if (!bSpraying)
	{
		return;
	}

	const USceneComponent* Nozzle = Player ? Player->GetEquippedNozzle() : nullptr;
	AWorldExtinguisher* Extinguisher = EquippedExtinguisher.Get();
	UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>();
	if (!Nozzle || !Extinguisher || Extinguisher->GetRemainingAgentKg() <= 0.0f ||
		(FireManager && FireManager->HasFinishedTraining()))
	{
		const bool bAgentDepleted = Extinguisher && Extinguisher->GetRemainingAgentKg() <= 0.0f;
		EndPrimaryInteract();
		if (bAgentDepleted)
		{
			ReportTrainingFeedback(ETrainingFeedbackType::ExtinguisherEmpty);
		}
		return;
	}

	const float DischargeRateKgPerSecond = Extinguisher->GetDischargeRateKgPerSecond();
	if (DischargeRateKgPerSecond <= UE_KINDA_SMALL_NUMBER)
	{
		EndPrimaryInteract();
		return;
	}

	const float PerformanceFactor = Extinguisher->GetRemainingAgentPerformanceFactor();
	Player->SetSprayEffectStrength(PerformanceFactor);
	const float RequestedAgentKg = DischargeRateKgPerSecond * GetWorld()->GetDeltaSeconds();
	const float UsedAgentKg = Extinguisher->ConsumeAgentKg(RequestedAgentKg);
	ETrainingFeedbackType FeedbackType = ETrainingFeedbackType::None;
	if (UsedAgentKg > 0.0f)
	{
		if (FireManager)
		{
			FireManager->NotifyAgentConsumed(UsedAgentKg);
		}
		const float ActualSpraySeconds = UsedAgentKg / DischargeRateKgPerSecond;
		const float SuppressionRatio = Extinguisher->GetBaseSuppressionRatioPerSecond()
			* ActualSpraySeconds * PerformanceFactor;
		FeedbackType = ApplySpray(*Nozzle, *Extinguisher, SuppressionRatio);
		NotifyExtinguisherStatusChanged();
	}
	if (Extinguisher->GetRemainingAgentKg() <= 0.0f)
	{
		EndPrimaryInteract();
		ReportTrainingFeedback(ETrainingFeedbackType::ExtinguisherEmpty);
	}
	else if (Extinguisher->GetAgentRatio() <= Extinguisher->GetLowAgentStartRatio())
	{
		ReportTrainingFeedback(ETrainingFeedbackType::ExtinguisherLow);
	}
	else
	{
		ReportTrainingFeedback(FeedbackType);
	}
}

void UPlayerInteractionComponent::EndPrimaryInteract()
{
	const bool bWasSpraying = bSpraying;
	bSpraying = false;
	SweepTrackingStates.Reset();
	if (bWasSpraying)
	{
		ReportTrainingFeedback(ETrainingFeedbackType::None);
	}
	AFireFighter* Player = Cast<AFireFighter>(GetOwner());
	if (Player)
	{
		Player->StopSprayEffect();
	}
	if (AFireFighterPlayerController* Controller = Player ? Cast<AFireFighterPlayerController>(Player->GetController()) : nullptr)
	{
		Controller->EndExtinguisherPinDrag();
	}
}

void UPlayerInteractionComponent::ReportTrainingFeedback(ETrainingFeedbackType Type)
{
	OnTrainingFeedbackSampled.Broadcast(Type);
}

ETrainingFeedbackType UPlayerInteractionComponent::ApplySpray(const USceneComponent& Nozzle,
	const AWorldExtinguisher& Extinguisher, float SuppressionRatio)
{
	const FVector Origin = Nozzle.GetComponentLocation();
	const FVector Forward = Nozzle.GetForwardVector();
	const FVector Right = Nozzle.GetRightVector();
	const UExtinguisherDataAsset* Data = Extinguisher.GetExtinguisherData();
	const double Range = FMath::Max(1.0f, Extinguisher.GetMaxEffectiveDistanceCm());
	const double MinSafeDistance = FMath::Clamp(
		static_cast<double>(Extinguisher.GetMinSafeDistanceCm()), 0.0, Range);
	const double HalfAngleDegrees = FMath::Clamp(SprayHalfAngle, 1.0f, 80.0f);
	const double HalfAngle = FMath::DegreesToRadians(HalfAngleDegrees);
	const double Slope = FMath::Tan(HalfAngle);
	const FVector ConeBase(Range, 0.0, 0.0);
	const FVector ConeRim(Range, Range * Slope, 0.0);
	const FCollisionQueryParams OcclusionParams(SCENE_QUERY_STAT(ExtinguisherSpray), false, GetOwner());

#if ENABLE_DRAW_DEBUG
	constexpr float DebugLifetime = 0.05f;
	const bool bDrawSpray = CVarDrawSpray.GetValueOnGameThread() != 0;
	if (bDrawSpray)
	{
		// DrawDebugCone measures along its side, while the Data Asset range measures along the axis.
		DrawDebugCone(GetWorld(), Origin, Forward, ConeRim.Size(), HalfAngle, HalfAngle, 12,
			FColor::Cyan, false, DebugLifetime, 0, 1.0f);
		DrawDebugDirectionalArrow(GetWorld(), Origin, Origin + Forward * Range, 15.0f,
			FColor::Yellow, false, DebugLifetime, 0, 1.5f);
	}
#endif

	struct FFeedbackCandidate
	{
		bool bValid = false;
		bool bIntersectsCone = false;
		bool bOccluded = false;
		double DistanceCm = 0.0;
		double Score = TNumericLimits<double>::Max();
		float AngleFraction = 0.0f;
		float AimHeight01 = 0.0f;
		bool bCompatible = true;
		bool bSweepComplete = false;
	};

	FFeedbackCandidate FeedbackCandidate;
	bool bHasBurningFire = false;
	const double GuidanceHalfAngle = FMath::Min(HalfAngle * 3.0, UE_HALF_PI);

	for (TActorIterator<AFireActor> It(GetWorld()); It; ++It)
	{
		AFireActor* Fire = *It;
		const USphereComponent* Sphere = Fire->GetAimSphere();
		if (Fire->GetIntensityRatio() <= 0.0f || !Sphere || !Sphere->IsQueryCollisionEnabled())
		{
			continue;
		}
		bHasBurningFire = true;

		// In a section through the cone axis, X is depth and Y is distance from the axis.
		const FVector SphereLocation = Sphere->GetComponentLocation();
		const float SphereRadius = Sphere->GetScaledSphereRadius();
		const FVector ToCenter = SphereLocation - Origin;
		const double Depth = FVector::DotProduct(ToCenter, Forward);
		const FVector Radial = ToCenter - Forward * Depth;
		const FVector Center(Depth, Radial.Size(), 0.0);
		FVector Contact = Center;
		bool bIntersectsCone = true;
		if (Depth < 0.0 || Depth > Range || Center.Y > Depth * Slope)
		{
			const FVector Side = FMath::ClosestPointOnSegment(Center, FVector::ZeroVector, ConeRim);
			const FVector Cap = FMath::ClosestPointOnSegment(Center, ConeBase, ConeRim);
			Contact = FVector::DistSquared(Center, Side) < FVector::DistSquared(Center, Cap) ? Side : Cap;
			bIntersectsCone = FVector::DistSquared(Center, Contact) <= FMath::Square(SphereRadius);
		}

		const FVector ContactPoint = Origin + Forward * Contact.X + Radial.GetSafeNormal() * Contact.Y;
		const bool bOccluded = bIntersectsCone &&
			GetWorld()->LineTraceTestByChannel(Origin, ContactPoint, ECC_Visibility, OcclusionParams);

		const double CenterDistance = ToCenter.Size();
		const double CenterAngle = Depth > 0.0
			? FMath::Atan2(Radial.Size(), Depth)
			: UE_PI;
		const FVector AxisPoint = Origin + Forward * Depth;
		const float SphereDiameter = FMath::Max(SphereRadius * 2.0f, 1.0f);
		const float AimHeight01 = FMath::Clamp(
			(AxisPoint.Z - (SphereLocation.Z - SphereRadius)) / SphereDiameter,
			0.0f, 1.0f);
		const float SignedLateralOffset01 = FMath::Clamp(
			FVector::DotProduct(AxisPoint - SphereLocation, Right) / FMath::Max(SphereRadius, 1.0f),
			-1.0f, 1.0f);
		const float CenterAngleFraction = static_cast<float>(CenterAngle / HalfAngle);
		const bool bCompatible = Extinguisher.SupportsFireClass(Fire->GetFireClass());
		const bool bValidSweepSample = bCompatible && bIntersectsCone && !bOccluded &&
			CenterDistance >= MinSafeDistance && CenterDistance <= Range &&
			AimHeight01 <= FMath::Clamp(SweepBaseAimMaxHeight, 0.05f, 1.0f) &&
			CenterAngleFraction <= 0.6f;
		const float SweepCoverage = bValidSweepSample
			? UpdateSweepCoverage(*Fire, SignedLateralOffset01) : 0.0f;
		const bool bSweepComplete = SweepCoverage >= 1.0f;
		if (Depth > 0.0 && CenterDistance <= Range * 2.0 && CenterAngle <= GuidanceHalfAngle)
		{
			const double CandidateScore = CenterAngle + CenterDistance / (Range * 1000.0);
			if (CandidateScore < FeedbackCandidate.Score)
			{
				FeedbackCandidate.bValid = true;
				FeedbackCandidate.bIntersectsCone = bIntersectsCone;
				FeedbackCandidate.bOccluded = bOccluded;
				FeedbackCandidate.DistanceCm = CenterDistance;
				FeedbackCandidate.Score = CandidateScore;
				FeedbackCandidate.AngleFraction = CenterAngleFraction;
				FeedbackCandidate.AimHeight01 = AimHeight01;
				FeedbackCandidate.bCompatible = bCompatible;
				FeedbackCandidate.bSweepComplete = bSweepComplete;
			}
		}

#if ENABLE_DRAW_DEBUG
		if (bDrawSpray)
		{
			const FColor SphereColor = !bIntersectsCone ? FColor::Red : (bOccluded ? FColor::Yellow : FColor::Green);
			DrawDebugSphere(GetWorld(), SphereLocation, SphereRadius, 16, SphereColor,
				false, DebugLifetime, SDPG_Foreground, 1.0f);
		}
#endif

		if (!bIntersectsCone || bOccluded)
		{
			continue;
		}

		const float AngleFraction = static_cast<float>(
			FMath::Clamp(FMath::Atan2(Contact.Y, Contact.X) / HalfAngle, 0.0, 1.0));
		const float DistanceFraction = static_cast<float>(FMath::Clamp(Contact.X / Range, 0.0, 1.0));
		const float AngleFactor = Data && Data->ConeCenterCurve
			? FMath::Clamp(Data->ConeCenterCurve->GetFloatValue(AngleFraction), 0.0f, 1.0f)
			: FMath::Lerp(1.0f, 0.15f, AngleFraction);

		float DistanceFactor = 1.0f;
		if (Data && Data->DistanceCurve)
		{
			DistanceFactor = FMath::Clamp(Data->DistanceCurve->GetFloatValue(DistanceFraction), 0.0f, 1.0f);
		}
		else if (Contact.X > MinSafeDistance)
		{
			const double FalloffLength = FMath::Max(Range - MinSafeDistance, 1.0);
			const float FalloffFraction = static_cast<float>(
				FMath::Clamp((Contact.X - MinSafeDistance) / FalloffLength, 0.0, 1.0));
			DistanceFactor = FMath::Lerp(1.0f, 0.2f, FalloffFraction);
		}

		if (bCompatible)
		{
			const float AimHeightFactor = Data && Data->AimHeightCurve
				? FMath::Clamp(Data->AimHeightCurve->GetFloatValue(AimHeight01), 0.0f, 1.0f)
				: FMath::Lerp(1.0f, 0.1f, AimHeight01);
			const float MinimumSweepFactor = FMath::Clamp(MinimumSweepSuppressionFactor, 0.0f, 1.0f);
			const float SweepFactor = FMath::Lerp(MinimumSweepFactor, 1.0f, SweepCoverage);
			Fire->ApplySuppressionRatio(
				SuppressionRatio * AngleFactor * DistanceFactor * AimHeightFactor * SweepFactor);
		}
	}

	if (!FeedbackCandidate.bValid)
	{
		return bHasBurningFire ? ETrainingFeedbackType::OffCenter : ETrainingFeedbackType::None;
	}
	if (FeedbackCandidate.bOccluded)
	{
		return ETrainingFeedbackType::Occluded;
	}
	if (FeedbackCandidate.DistanceCm < MinSafeDistance)
	{
		return ETrainingFeedbackType::TooClose;
	}
	if (FeedbackCandidate.DistanceCm > Range)
	{
		return ETrainingFeedbackType::TooFar;
	}
	if (!FeedbackCandidate.bIntersectsCone)
	{
		return ETrainingFeedbackType::OffCenter;
	}
	if (!FeedbackCandidate.bCompatible)
	{
		return ETrainingFeedbackType::WrongExtinguisher;
	}
	if (FeedbackCandidate.AimHeight01 > FMath::Clamp(SweepBaseAimMaxHeight, 0.05f, 1.0f))
	{
		return ETrainingFeedbackType::AimTooHigh;
	}
	if (FeedbackCandidate.AngleFraction > 0.6f)
	{
		return ETrainingFeedbackType::OffCenter;
	}
	if (!FeedbackCandidate.bSweepComplete)
	{
		return ETrainingFeedbackType::SweepRequired;
	}
	return ETrainingFeedbackType::GoodAim;
}

float UPlayerInteractionComponent::UpdateSweepCoverage(AFireActor& Fire, float SignedLateralOffset01)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	FSweepTrackingState& State = SweepTrackingStates.FindOrAdd(&Fire);
	const double Now = World->GetTimeSeconds();
	const double CompletionWindow = FMath::Max(0.1f, SweepCompletionWindowSeconds);
	if (State.SweepStartedTime >= 0.0 && State.CoveredZoneMask != SweepAllZones &&
		Now - State.SweepStartedTime > CompletionWindow)
	{
		State.CoveredZoneMask = 0;
		State.SweepStartedTime = -1.0;
	}
	if (State.SweepStartedTime < 0.0)
	{
		State.SweepStartedTime = Now;
	}

	const float SideThreshold = FMath::Clamp(SweepSideZoneThreshold, 0.05f, 0.9f);
	if (SignedLateralOffset01 <= -SideThreshold)
	{
		State.CoveredZoneMask |= SweepLeftZone;
	}
	else if (SignedLateralOffset01 >= SideThreshold)
	{
		State.CoveredZoneMask |= SweepRightZone;
	}
	else
	{
		State.CoveredZoneMask |= SweepCenterZone;
	}
	const int32 CoveredZoneCount =
		((State.CoveredZoneMask & SweepLeftZone) != 0 ? 1 : 0) +
		((State.CoveredZoneMask & SweepCenterZone) != 0 ? 1 : 0) +
		((State.CoveredZoneMask & SweepRightZone) != 0 ? 1 : 0);
	return State.CoveredZoneMask == SweepAllZones ? 1.0f : CoveredZoneCount / 3.0f;
}

AActor* UPlayerInteractionComponent::FindAimedActor() const
{
	const AFireFighter* Player = Cast<AFireFighter>(GetOwner());
	return Player ? FindFocusedActor(*Player, AimTraceChannel) : nullptr;
}

AActor* UPlayerInteractionComponent::FindFocusedActor(
	const AFireFighter& Player, ECollisionChannel Channel, const TCHAR* InputName) const
{
	const APlayerController* Controller = Cast<APlayerController>(Player.GetController());
	const UWorld* World = GetWorld();
	if (!Controller || !World)
	{
		if (InputName)
		{
			UE_LOG(LogDoorInteraction, Log, TEXT("InteractionTraceUnavailable: Input=%s Player=%s Controller=%s World=%s"),
				InputName, *Player.GetName(), *GetNameSafe(Controller), *GetNameSafe(World));
		}
		return nullptr;
	}

	FVector Start;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(Start, ViewRotation);
	const float TraceDistance = Channel == AimTraceChannel
		? FireAimDetectionDistance
		: InteractionDistance;
	const FVector End = Start + ViewRotation.Vector() * TraceDistance;
	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerInteraction), false, &Player);
	FHitResult Hit;
	World->LineTraceSingleByChannel(Hit, Start, End, Channel, QueryParams);
	AActor* Target = Hit.GetActor();
	if (Channel == AimTraceChannel && Target)
	{
		// Aim overlaps by default. Visibility still occludes targets behind walls.
		FHitResult Occluder;
		if (World->LineTraceSingleByChannel(Occluder, Start, Hit.ImpactPoint, ECC_Visibility, QueryParams)
			&& Occluder.GetActor() != Target)
		{
			Target = nullptr;
		}
	}

	if (InputName)
	{
		UE_LOG(LogDoorInteraction, Log, TEXT("InteractionTrace: Input=%s Player=%s HitActor=%s HitComponent=%s RangeCm=%.0f"),
			InputName, *Player.GetName(), *GetNameSafe(Target), *GetNameSafe(Hit.GetComponent()), TraceDistance);
	}
	return Target;
}
