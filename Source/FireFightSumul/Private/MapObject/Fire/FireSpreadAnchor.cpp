#include "MapObject/Fire/FireSpreadAnchor.h"

#include "Components/SphereComponent.h"

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

void AFireSpreadAnchor::ResetSpreadState()
{
	AccumulatedHeat = 0.0f;
	bIgnited = false;
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
	AccumulatedHeat = 0.0f;
}
