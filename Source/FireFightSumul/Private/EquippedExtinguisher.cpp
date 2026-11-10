#include "EquippedExtinguisher.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AEquippedExtinguisher::AEquippedExtinguisher()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	HoseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HoseMesh"));
	HoseMesh->SetupAttachment(SceneRoot);
	HoseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	NozzlePivot = CreateDefaultSubobject<USceneComponent>(TEXT("NozzlePivot"));
	NozzlePivot->SetupAttachment(SceneRoot);

	NozzleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NozzleMesh"));
	NozzleMesh->SetupAttachment(NozzlePivot);
	NozzleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
