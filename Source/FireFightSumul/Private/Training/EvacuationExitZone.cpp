#include "Training/EvacuationExitZone.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "MapObject/Fire/FireManager.h"

AEvacuationExitZone::AEvacuationExitZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ExitZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ExitZone"));
	SetRootComponent(ExitZone);
	ExitZone->InitBoxExtent(FVector(100.0f, 150.0f, 120.0f));
	ExitZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ExitZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	ExitZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ExitZone->SetGenerateOverlapEvents(true);
}

void AEvacuationExitZone::BeginPlay()
{
	Super::BeginPlay();
	ExitZone->OnComponentBeginOverlap.AddDynamic(
		this, &AEvacuationExitZone::HandleExitZoneBeginOverlap);
}

void AEvacuationExitZone::HandleExitZoneBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor, UPrimitiveComponent* /*OtherComponent*/, int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	if (UFireManager* FireManager = GetWorld()->GetSubsystem<UFireManager>())
	{
		FireManager->NotifyPlayerEvacuated();
	}
}
