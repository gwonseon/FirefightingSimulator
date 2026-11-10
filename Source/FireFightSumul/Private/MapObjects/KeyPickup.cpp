#include "MapObjects/KeyPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DoorInteractionLog.h"
#include "Player/FireFighter.h"

AKeyPickup::AKeyPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
	PickupCollision->SetupAttachment(GetRootComponent());
	PickupCollision->InitSphereRadius(32.0f);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupCollision->SetGenerateOverlapEvents(true);
	PickupCollision->OnComponentBeginOverlap.AddDynamic(this, &AKeyPickup::OnPickupOverlap);

	KeyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KeyMesh"));
	KeyMesh->SetupAttachment(GetRootComponent());
	KeyMesh->SetMobility(EComponentMobility::Movable);
	KeyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AKeyPickup::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogDoorInteraction, Log, TEXT("KeyReady: Key=%s KeyId=%s OverlapEvents=%d"),
		*GetName(), *KeyId.ToString(), PickupCollision->GetGenerateOverlapEvents());
}

void AKeyPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (RotationSpeed != 0.0f)
	{
		KeyMesh->AddRelativeRotation(FRotator(0.0, RotationSpeed * DeltaSeconds, 0.0));
	}
}

void AKeyPickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AFireFighter* Player = Cast<AFireFighter>(OtherActor);
	if (!IsValid(Player) || KeyId.IsNone())
	{
		UE_LOG(LogDoorInteraction, Log, TEXT("KeyPickupRejected: Key=%s KeyId=%s OverlapActor=%s IsFireFighter=%d"),
			*GetName(), *KeyId.ToString(), *GetNameSafe(OtherActor), IsValid(Player));
		return;
	}

	Player->AddKey(KeyId);
	UE_LOG(LogDoorInteraction, Log, TEXT("KeyPickedUp: Key=%s KeyId=%s Player=%s"),
		*GetName(), *KeyId.ToString(), *Player->GetName());
	Destroy();
}
