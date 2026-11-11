#include "MapObject/Fire/FireSpreadAnchor.h"

#include "Components/SphereComponent.h"
#include "MapObject/Fire/FireMaterialTags.h"

AFireSpreadAnchor::AFireSpreadAnchor()
{
	PrimaryActorTick.bCanEverTick = false;

	EditorMarker = CreateDefaultSubobject<USphereComponent>(TEXT("EditorMarker"));
	SetRootComponent(EditorMarker);
	EditorMarker->InitSphereRadius(15.0f);
	EditorMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EditorMarker->SetGenerateOverlapEvents(false);
	EditorMarker->SetHiddenInGame(true);
	EditorMarker->ShapeColor = FColor(255, 128, 0);
}

FName AFireSpreadAnchor::GetMaterialTag() const
{
	FName ActorMaterialTag;
	const EFireMaterialTagResolveResult Result =
		FireMaterialTags::ResolveActorMaterialTag(*this, ActorMaterialTag);
	if (Result == EFireMaterialTagResolveResult::Found)
	{
		return ActorMaterialTag;
	}
	if (Result == EFireMaterialTagResolveResult::Ambiguous)
	{
		UE_LOG(LogTemp, Error,
			TEXT("FireSpreadAnchor '%s' has more than one FireMaterial tag. The anchor is disabled."),
			*GetName());
		return NAME_None;
	}

	return FireMaterialTags::Canonicalize(MaterialTag);
}

void AFireSpreadAnchor::ResetSpreadState()
{
	AccumulatedHeat = 0.0f;
	bIgnited = false;
	bAwaitingReignition = false;
	ReignitionAllowedTimeSeconds = 0.0;
}

void AFireSpreadAnchor::AddHeat(float Amount)
{
	if (!FMath::IsFinite(Amount))
	{
		return;
	}

	AccumulatedHeat = FMath::Max(0.0f, AccumulatedHeat + Amount);
}

void AFireSpreadAnchor::MarkIgnited()
{
	bIgnited = true;
	bAwaitingReignition = false;
	ReignitionAllowedTimeSeconds = 0.0;
	AccumulatedHeat = 0.0f;
}

void AFireSpreadAnchor::MarkExtinguished(double WorldTimeSeconds, float CooldownSeconds)
{
	bIgnited = false;
	bAwaitingReignition = true;
	ReignitionAllowedTimeSeconds = WorldTimeSeconds + FMath::Max(0.0f, CooldownSeconds);
	AccumulatedHeat = 0.0f;
}

bool AFireSpreadAnchor::IsReignitionReady(double WorldTimeSeconds) const
{
	return !bAwaitingReignition || WorldTimeSeconds >= ReignitionAllowedTimeSeconds;
}
