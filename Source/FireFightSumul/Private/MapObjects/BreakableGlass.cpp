#include "MapObjects/BreakableGlass.h"

#include "Components/StaticMeshComponent.h"

ABreakableGlass::ABreakableGlass()
{
	GlassMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GlassMesh"));
	GlassMesh->SetupAttachment(GetRootComponent());
}

bool ABreakableGlass::BreakGlass()
{
	if (bIsBroken)
	{
		return false;
	}

	bIsBroken = true;
	GlassMesh->SetVisibility(false);
	GlassMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return true;
}
