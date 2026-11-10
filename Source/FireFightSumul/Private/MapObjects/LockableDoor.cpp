#include "MapObjects/LockableDoor.h"

#include "AimTraceChannel.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "DoorInteractionLog.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Player/FireFighter.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogDoorInteraction);

const FName ALockableDoor::AimTag(TEXT("Door"));

ALockableDoor::ALockableDoor()
{
	FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
	FrameMesh->SetupAttachment(GetRootComponent());

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(GetRootComponent());

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(Hinge);

	static const ConstructorHelpers::FObjectFinder<UTexture2D> LockedIcon(
		TEXT("/Game/_Image/T_UI_DoorLocked.T_UI_DoorLocked"));
	static const ConstructorHelpers::FObjectFinder<UTexture2D> UnlockIcon(
		TEXT("/Game/_Image/T_UI_KeyE.T_UI_KeyE"));
	static const ConstructorHelpers::FObjectFinder<UTexture2D> OpenCloseIcon(
		TEXT("/Game/_Image/T_UI_MouseRightClick.T_UI_MouseRightClick"));
	LockedPromptTexture = LockedIcon.Object;
	UnlockPromptTexture = UnlockIcon.Object;
	OpenClosePromptTexture = OpenCloseIcon.Object;
}

void ALockableDoor::BeginPlay()
{
	Tags.AddUnique(AimTag);
	FrameMesh->SetCollisionResponseToChannel(AimTraceChannel, ECR_Block);
	DoorMesh->SetCollisionResponseToChannel(AimTraceChannel, ECR_Block);

	ClosedRotation = Hinge->GetRelativeRotation().Quaternion();
	bIsOpen = bStartsOpen;
	bIsLocked = bStartsLocked && !bIsOpen;
	CurrentAngle = bIsOpen ? OpenAngle : 0.0f;
	UpdateHingeRotation();

	UE_LOG(LogDoorInteraction, Log, TEXT("DoorReady: Door=%s RequiredKeyId=%s Locked=%d Open=%d Angle=%.1f"),
		*GetName(), *RequiredKeyId.ToString(), bIsLocked, bIsOpen, CurrentAngle);
	if (bIsLocked && RequiredKeyId.IsNone())
	{
		UE_LOG(LogDoorInteraction, Error, TEXT("%s starts locked but has no RequiredKeyId."), *GetPathName());
	}

	Super::BeginPlay();

	static const FName PromptComponentName(TEXT("DoorPromptWidget"));
	TInlineComponentArray<UWidgetComponent*> WidgetComponents(this);
	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		if (WidgetComponent->GetFName() == PromptComponentName)
		{
			PromptComponent = WidgetComponent;
			WidgetComponent->SetVisibility(false);
			break;
		}
	}
}

void ALockableDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(MovementTimer);
	Super::EndPlay(EndPlayReason);
}

void ALockableDoor::Interact_Implementation(AFireFighter* Player)
{
	const bool bMoving = MovementTimer.IsValid();
	const bool bHasRequiredKey = IsValid(Player) && !RequiredKeyId.IsNone() && Player->HasKey(RequiredKeyId);
	const bool bCanToggleLock = IsValid(Player) && CanToggleLock(*Player);
	if (bCanToggleLock)
	{
		bIsLocked = !bIsLocked;
	}

	UE_LOG(LogDoorInteraction, Log,
		TEXT("%s: Door=%s Player=%s RequiredKeyId=%s HasRequiredKey=%d Locked=%d Open=%d Moving=%d"),
		bCanToggleLock ? TEXT("DoorLockToggled") : TEXT("DoorLockRejected"),
		*GetName(), *GetNameSafe(Player), *RequiredKeyId.ToString(), bHasRequiredKey, bIsLocked, bIsOpen, bMoving);
}

bool ALockableDoor::CanToggleLock(const AFireFighter& Player) const
{
	return !bIsOpen && !MovementTimer.IsValid() && !RequiredKeyId.IsNone() && Player.HasKey(RequiredKeyId);
}

void ALockableDoor::UpdateInteractionPrompt(const AFireFighter* Player)
{
	UWidgetComponent* Prompt = PromptComponent.Get();
	if (!Prompt)
	{
		return;
	}

	UTexture2D* Texture = nullptr;
	if (Player && !MovementTimer.IsValid())
	{
		if (bIsLocked)
		{
			Texture = CanToggleLock(*Player) ? UnlockPromptTexture.Get() : LockedPromptTexture.Get();
		}
		else
		{
			Texture = OpenClosePromptTexture.Get();
		}
	}

	if (Texture == DisplayedPromptTexture)
	{
		return;
	}

	if (!Texture)
	{
		Prompt->SetVisibility(false);
		DisplayedPromptTexture = nullptr;
		return;
	}

	Prompt->InitWidget();
	UUserWidget* Widget = Prompt->GetUserWidgetObject();
	UImage* StateIcon = Widget ? Cast<UImage>(Widget->GetWidgetFromName(TEXT("StateIcon"))) : nullptr;
	if (!StateIcon)
	{
		return;
	}

	if (UWidget* KeyIconBox = Widget->GetWidgetFromName(TEXT("KeyIconBox")))
	{
		KeyIconBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	StateIcon->SetBrushFromTexture(Texture);
	Prompt->SetVisibility(true);
	DisplayedPromptTexture = Texture;
}

bool ALockableDoor::TryToggleOpen()
{
	if (bIsLocked || MovementTimer.IsValid())
	{
		UE_LOG(LogDoorInteraction, Log, TEXT("DoorMoveRejected: Door=%s Locked=%d Open=%d Moving=%d"),
			*GetName(), bIsLocked, bIsOpen, MovementTimer.IsValid());
		return false;
	}

	LastMovementUpdateTime = GetWorld()->GetTimeSeconds();
	FTimerManagerTimerParameters TimerParameters;
	TimerParameters.bLoop = true;
	TimerParameters.bMaxOncePerFrame = true;
	GetWorldTimerManager().SetTimer(MovementTimer, this, &ALockableDoor::AdvanceDoorMovement, 1.0f / 120.0f, TimerParameters);
	UE_LOG(LogDoorInteraction, Log, TEXT("DoorMoveStarted: Door=%s Action=%s Angle=%.1f TargetAngle=%.1f OpenSpeed=%.1f"),
		*GetName(), bIsOpen ? TEXT("Closing") : TEXT("Opening"), CurrentAngle, bIsOpen ? 0.0f : OpenAngle, OpenSpeed);
	return true;
}

void ALockableDoor::AdvanceDoorMovement()
{
	const double Now = GetWorld()->GetTimeSeconds();
	const float DeltaTime = static_cast<float>(Now - LastMovementUpdateTime);
	LastMovementUpdateTime = Now;

	const float TargetAngle = bIsOpen ? 0.0f : OpenAngle;
	CurrentAngle = FMath::FInterpConstantTo(CurrentAngle, TargetAngle, DeltaTime, FMath::Max(OpenSpeed, 1.0f));
	if (FMath::IsNearlyEqual(CurrentAngle, TargetAngle))
	{
		CurrentAngle = TargetAngle;
		bIsOpen = !bIsOpen;
		GetWorldTimerManager().ClearTimer(MovementTimer);
		UE_LOG(LogDoorInteraction, Log, TEXT("DoorMoveFinished: Door=%s Locked=%d Open=%d Angle=%.1f"),
			*GetName(), bIsLocked, bIsOpen, CurrentAngle);
	}

	UpdateHingeRotation();
}

void ALockableDoor::UpdateHingeRotation()
{
	const FQuat Rotation(FVector::UpVector, FMath::DegreesToRadians(CurrentAngle));
	Hinge->SetRelativeRotation(Rotation * ClosedRotation);
}
