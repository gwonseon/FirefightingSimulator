#include "MapObjects/WorldExtinguisher.h"

#include "AimTraceChannel.h"
#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AWorldExtinguisher::AWorldExtinguisher()
{
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetRootComponent());

	InspectCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("InspectCamera"));
	InspectCamera->SetupAttachment(GetRootComponent());
	InspectCamera->SetRelativeLocation(FVector(-70.0, 0.0, 25.0));

	PinPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PinPivot"));
	PinPivot->SetupAttachment(GetRootComponent());

	PinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PinMesh"));
	PinMesh->SetupAttachment(PinPivot);
	PinMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PinHitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("PinHitBox"));
	PinHitBox->SetupAttachment(PinPivot);
	PinHitBox->InitBoxExtent(FVector(3.0));
	PinHitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PinHitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	PinHitBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AWorldExtinguisher::BeginPlay()
{
	PinRestRelativeLocation = PinPivot->GetRelativeLocation();
	RemainingAgent = FMath::Clamp(InitialAgentAmount, 0.0f, FMath::Max(AgentCapacity, 0.0f));

	TInlineComponentArray<UBoxComponent*> Boxes(this);
	bool bHasSelectionBox = false;
	for (UBoxComponent* Box : Boxes)
	{
		if (Box != PinHitBox && Box->IsQueryCollisionEnabled())
		{
			Box->SetCollisionResponseToChannel(AimTraceChannel, ECR_Block);
			bHasSelectionBox = true;
		}
	}
	if (!bHasSelectionBox)
	{
		BodyMesh->SetCollisionResponseToChannel(AimTraceChannel, ECR_Block);
	}

	Super::BeginPlay();
}

bool AWorldExtinguisher::TryDropNear(const AActor& Dropper)
{
	UWorld* World = GetWorld();
	const FVector DropperLocation = Dropper.GetActorLocation();
	const FVector BodyExtent = BodyMesh->Bounds.BoxExtent;
	const FVector BodyCenterOffset = BodyMesh->Bounds.Origin - GetActorLocation();
	const double DropDistance = Dropper.GetSimpleCollisionRadius() + BodyExtent.Size2D() + 12.0;
	const double FloorTraceDistance = Dropper.GetSimpleCollisionHalfHeight() + 100.0;
	const FVector Forward = Dropper.GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	const FVector Directions[] = { Forward, Right, -Right, -Forward };
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ExtinguisherDrop), false, &Dropper);
	QueryParams.AddIgnoredActor(this);
	const FCollisionShape BodyShape = FCollisionShape::MakeBox(BodyExtent);

	for (const FVector& Direction : Directions)
	{
		const FVector FloorStart = DropperLocation + Direction * DropDistance;
		const FVector FloorEnd = FloorStart - FVector(0.0, 0.0, FloorTraceDistance);
		FHitResult FloorHit;
		if (!World->LineTraceSingleByChannel(FloorHit, FloorStart, FloorEnd, ECC_Visibility, QueryParams)
			|| FloorHit.ImpactNormal.Z < 0.7)
		{
			continue;
		}

		const FVector BodyCenter = FloorHit.ImpactPoint + FVector(0.0, 0.0, BodyExtent.Z + 2.0);
		if (World->LineTraceTestByChannel(DropperLocation, BodyCenter, ECC_Visibility, QueryParams)
			|| World->OverlapBlockingTestByChannel(BodyCenter, FQuat::Identity, ECC_Visibility, BodyShape, QueryParams))
		{
			continue;
		}

		if (SetActorLocation(BodyCenter - BodyCenterOffset, false, nullptr, ETeleportType::TeleportPhysics))
		{
			SetActorHiddenInGame(false);
			SetActorEnableCollision(true);
			return true;
		}
	}
	return false;
}

bool AWorldExtinguisher::CanGrabPin(const FVector& Start, const FVector& End) const
{
	FHitResult Hit;
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(ExtinguisherPin), false);
	return !bIsPinRemoved && PinHitBox->IsQueryCollisionEnabled() && PinHitBox->LineTraceComponent(Hit, Start, End, Params);
}

void AWorldExtinguisher::GetPinPullLocations(FVector& RestLocation, FVector& PulledLocation) const
{
	RestLocation = PinPivot->GetAttachParent()->GetComponentTransform().TransformPosition(PinRestRelativeLocation);
	PulledLocation = RestLocation - InspectCamera->GetRightVector() * FMath::Max(PinPullDistance, 0.1f);
}

void AWorldExtinguisher::SetPinPullProgress(float Progress)
{
	if (!bIsPinRemoved)
	{
		FVector RestLocation, PulledLocation;
		GetPinPullLocations(RestLocation, PulledLocation);
		PinPivot->SetWorldLocation(FMath::Lerp(RestLocation, PulledLocation, Progress));
	}
}

float AWorldExtinguisher::GetAgentRatio() const
{
	return AgentCapacity > 0.0f ? RemainingAgent / AgentCapacity : 0.0f;
}

float AWorldExtinguisher::ConsumeAgent(float RequestedAmount)
{
	const float UsedAmount = FMath::Clamp(RequestedAmount, 0.0f, RemainingAgent);
	RemainingAgent -= UsedAmount;
	return UsedAmount;
}

bool AWorldExtinguisher::TryRemovePin()
{
	if (bIsPinRemoved)
	{
		return false;
	}

	bIsPinRemoved = true;
	PinMesh->SetVisibility(false);
	PinHitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return true;
}
