#include "Player/PlayerInteractionComponent.h"

#include "AimTraceChannel.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
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
#include "Player/FireFighter.h"
#include "Player/FireFighterPlayerController.h"

namespace
{
#if ENABLE_DRAW_DEBUG
	TAutoConsoleVariable<int32> CVarDrawSpray(
		TEXT("ffs.DebugSpray"), 1, TEXT("Draw the spray cone and fire collision spheres while spraying."));
#endif

	float GetChargeSuppressionMultiplier(float AgentRatio)
	{
		// Relative MAP extinguishment rates from Table 3 (PMC11818437), normalized to the 0.3 MPa test.
		if (AgentRatio >= 0.75f)
		{
			return 1.0f;
		}
		if (AgentRatio >= 0.35f)
		{
			return 2.1f / 4.7f;
		}
		return 2.1f / 22.8f;
	}
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
	const AFireFighter* Player = Cast<AFireFighter>(GetOwner());
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
	bSpraying = Player->GetEquippedNozzle() && Extinguisher && Extinguisher->GetRemainingAgent() > 0.0f;
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
	const AFireFighter* Player = Cast<AFireFighter>(GetOwner());
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
	/*if (!Nozzle || !Extinguisher || Extinguisher->GetRemainingAgent() <= 0.0f ||
		(FireManager && FireManager->HasFinishedTraining()))
	{
		EndPrimaryInteract();
		return;
	}*/

	const float AgentRatio = Extinguisher->GetAgentRatio();
	const float AgentUsed = Extinguisher->ConsumeAgent(FMath::Max(AgentPerSecond, 0.01f) * GetWorld()->GetDeltaSeconds());
	if (AgentUsed > 0.0f)
	{
		NotifyExtinguisherStatusChanged();
		const float SuppressionAmount = AgentUsed * SuppressionPerUnit * GetChargeSuppressionMultiplier(AgentRatio);
		ApplySpray(*Nozzle, SuppressionAmount);
	}
	if (Extinguisher->GetRemainingAgent() <= 0.0f)
	{
	/*	EndPrimaryInteract();
		if (FireManager)
		{
			FireManager->NotifyExtinguisherDepleted();
		}
		*/
	}
}

void UPlayerInteractionComponent::EndPrimaryInteract()
{
	bSpraying = false;
	const AFireFighter* Player = Cast<AFireFighter>(GetOwner());
	if (AFireFighterPlayerController* Controller = Player ? Cast<AFireFighterPlayerController>(Player->GetController()) : nullptr)
	{
		Controller->EndExtinguisherPinDrag();
	}
}

void UPlayerInteractionComponent::ApplySpray(const USceneComponent& Nozzle, float SuppressionAmount) const
{
	const FVector Origin = Nozzle.GetComponentLocation();
	const FVector Forward = Nozzle.GetForwardVector();
	const double Range = FMath::Max(1.0f, SprayRange);
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
		// DrawDebugCone measures along its side, while SprayRange measures along the axis.
		DrawDebugCone(GetWorld(), Origin, Forward, ConeRim.Size(), HalfAngle, HalfAngle, 12,
			FColor::Cyan, false, DebugLifetime, 0, 1.0f);
		DrawDebugDirectionalArrow(GetWorld(), Origin, Origin + Forward * Range, 15.0f,
			FColor::Yellow, false, DebugLifetime, 0, 1.5f);
	}
#endif

	for (TActorIterator<AFireActor> It(GetWorld()); It; ++It)
	{
		AFireActor* Fire = *It;
		const USphereComponent* Sphere = Fire->GetAimSphere();
		if (Fire->GetIntensityRatio() <= 0.0f || !Sphere || !Sphere->IsQueryCollisionEnabled())
		{
			continue;
		}

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

		const double AngleFraction = FMath::Clamp(FMath::Atan2(Contact.Y, Contact.X) / HalfAngle, 0.0, 1.0);
		const double DistanceFraction = Contact.X / Range;
		const double AngleFactor = FMath::Lerp(1.0, 0.15, AngleFraction);
		const double DistanceFactor = FMath::Lerp(1.0, 0.2, DistanceFraction);
		Fire->ApplySuppression(SuppressionAmount * AngleFactor * DistanceFactor);
	}
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
	const FVector End = Start + ViewRotation.Vector() * InteractionDistance;
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
			InputName, *Player.GetName(), *GetNameSafe(Target), *GetNameSafe(Hit.GetComponent()), InteractionDistance);
	}
	return Target;
}
