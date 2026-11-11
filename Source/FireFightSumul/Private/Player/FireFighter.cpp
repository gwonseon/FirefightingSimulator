// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/FireFighter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DoorInteractionLog.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "HAL/IConsoleManager.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Player/PlayerInteractionComponent.h"

namespace
{
	const FName HoseComponentName(TEXT("Hose"));
}

AFireFighter::AFireFighter()
{
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0, 0.0, 64.0));
	FirstPersonCamera->bUsePawnControlRotation = true;
	NozzleTip = CreateDefaultSubobject<USceneComponent>(TEXT("NozzleTip"));
	NozzleTip->SetupAttachment(FirstPersonCamera);
	SprayEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SprayEffectComponent"));
	SprayEffectComponent->SetupAttachment(NozzleTip);
	SprayEffectComponent->SetAutoActivate(false);
	InteractionComponent = CreateDefaultSubobject<UPlayerInteractionComponent>(TEXT("InteractionComponent"));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 400.0f;
}

void AFireFighter::BeginPlay()
{
	Super::BeginPlay();
	SetCollisionDebugVisible(false);
	SprayEffectComponent->SetRelativeScale3D(
		FVector(1.0f, SprayVisualWidthScale, SprayVisualWidthScale));
	SetHoseEquipped(false);
}

const USceneComponent* AFireFighter::GetEquippedNozzle() const
{
	return EquippedHose.IsValid() ? NozzleTip.Get() : nullptr;
}

void AFireFighter::StartSprayEffect(UNiagaraSystem* System, float Strength)
{
	if (!SprayEffectComponent || !System)
	{
		return;
	}

	if (SprayEffectComponent->GetAsset() != System)
	{
		SprayEffectComponent->DeactivateImmediate();
		SprayEffectComponent->SetAsset(System);
	}
	SprayEffectComponent->SetRelativeScale3D(
		FVector(1.0f, SprayVisualWidthScale, SprayVisualWidthScale));

	SetSprayEffectStrength(Strength);
	if (!SprayEffectComponent->IsActive())
	{
		SprayEffectComponent->Activate(true);
	}
}

void AFireFighter::SetSprayEffectStrength(float Strength)
{
	if (SprayEffectComponent)
	{
		SprayEffectComponent->SetVariableFloat(
			TEXT("User.SprayStrength"), FMath::Clamp(Strength, 0.0f, 1.0f));
	}
}

void AFireFighter::StopSprayEffect()
{
	// Keep the Niagara system alive so already spawned powder can slow down and
	// fade naturally. A strength of zero must drive each emitter's spawn rate to
	// zero; pressing fire again can then resume spawning without a reactivation race.
	SetSprayEffectStrength(0.0f);
}

void AFireFighter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	USkeletalMeshComponent* Hose = EquippedHose.Get();
	if (!Hose)
	{
		return;
	}

	const float GroundSpeed = GetCharacterMovement()->IsMovingOnGround() ? GetVelocity().Size2D() : 0.0f;
	const float TargetWeight = GroundSpeed > 5.0f
		? FMath::Clamp(GroundSpeed / FMath::Max(SprintSpeed, 1.0f), 0.0f, 1.0f)
		: 0.0f;
	if (HoseSwayWeight == 0.0f && TargetWeight == 0.0f)
	{
		return;
	}

	HoseSwayWeight = FMath::FInterpTo(HoseSwayWeight, TargetWeight, DeltaSeconds, 8.0f);
	if (TargetWeight == 0.0f && HoseSwayWeight < 0.001f)
	{
		Hose->SetRelativeTransform(HoseRestTransform);
		HoseSwayPhase = 0.0f;
		HoseSwayWeight = 0.0f;
		return;
	}

	constexpr float FullTurn = 2.0f * PI;
	const float SwayRate = FMath::Lerp(0.8f, 1.6f, HoseSwayWeight);
	HoseSwayPhase = FMath::Fmod(HoseSwayPhase + FullTurn * SwayRate * DeltaSeconds, FullTurn);
	const float Amplitude = HoseSwayWeight * HoseSwayStrength;
	const float Side = FMath::Sin(HoseSwayPhase) * Amplitude;
	const float Height = -FMath::Cos(2.0f * HoseSwayPhase) * Amplitude;
	const FVector Offset(0.0, Side * 1.5f, Height * 2.0f);
	const FQuat Roll = FRotator(0.0, 0.0, Side * 2.0f).Quaternion();
	Hose->SetRelativeLocationAndRotation(HoseRestTransform.GetLocation() + Offset,
		Roll * HoseRestTransform.GetRotation());
}

void AFireFighter::SetHoseEquipped(bool bEquipped)
{
	static const FName ArmsName(TEXT("FirstPersonArms"));
	USkeletalMeshComponent* Hose = nullptr;
	USkeletalMeshComponent* Arms = nullptr;

	TInlineComponentArray<USkeletalMeshComponent*> Meshes(this);
	for (USkeletalMeshComponent* MeshComponent : Meshes)
	{
		if (MeshComponent->GetFName() == HoseComponentName)
		{
			Hose = MeshComponent;
		}
		else if (MeshComponent->GetFName() == ArmsName)
		{
			Arms = MeshComponent;
		}
	}

	if (Hose)
	{
		const bool bWasEquipped = EquippedHose.IsValid();
		if (bEquipped && EquippedHose.Get() != Hose)
		{
			HoseRestTransform = Hose->GetRelativeTransform();
			// The Blueprint authors the tip in camera space. Preserve that pose when attaching it.
			NozzleTip->AttachToComponent(Hose, FAttachmentTransformRules::KeepWorldTransform);
			EquippedHose = Hose;
		}
		else if (!bEquipped && EquippedHose.Get() == Hose)
		{
			Hose->SetRelativeTransform(HoseRestTransform);
			NozzleTip->AttachToComponent(FirstPersonCamera, FAttachmentTransformRules::KeepWorldTransform);
			InteractionComponent->EndPrimaryInteract();
			EquippedHose.Reset();
			HoseSwayPhase = 0.0f;
			HoseSwayWeight = 0.0f;
		}

		Hose->SetVisibility(bEquipped);
		if (Arms)
		{
			Arms->SetVisibility(!bEquipped);
		}

		if (bWasEquipped != EquippedHose.IsValid())
		{
			InteractionComponent->NotifyExtinguisherStatusChanged();
		}
	}
}

void AFireFighter::AddKey(FName KeyId)
{
	if (!KeyId.IsNone())
	{
		OwnedKeyIds.Add(KeyId);
	}
}

bool AFireFighter::HasKey(FName KeyId) const
{
	return OwnedKeyIds.Contains(KeyId);
}

void AFireFighter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!Input || !PlayerController)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	if (!Subsystem)
	{
		return;
	}

	InputMapping = NewObject<UInputMappingContext>(this);

	UInputAction* MoveAction = NewObject<UInputAction>(InputMapping);
	MoveAction->ValueType = EInputActionValueType::Axis2D;
	MoveAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;

	InputMapping->MapKey(MoveAction, EKeys::D);
	InputMapping->MapKey(MoveAction, EKeys::A).Modifiers.Add(NewObject<UInputModifierNegate>(InputMapping));
	InputMapping->MapKey(MoveAction, EKeys::W).Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(InputMapping));
	FEnhancedActionKeyMapping& Backward = InputMapping->MapKey(MoveAction, EKeys::S);
	Backward.Modifiers.Add(NewObject<UInputModifierNegate>(InputMapping));
	Backward.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(InputMapping));

	UInputAction* LookAction = NewObject<UInputAction>(InputMapping);
	LookAction->ValueType = EInputActionValueType::Axis2D;
	InputMapping->MapKey(LookAction, EKeys::Mouse2D);

	UInputAction* SprintAction = NewObject<UInputAction>(InputMapping);
	SprintAction->ValueType = EInputActionValueType::Boolean;
	InputMapping->MapKey(SprintAction, EKeys::LeftShift);

	UInputAction* JumpAction = NewObject<UInputAction>(InputMapping);
	JumpAction->ValueType = EInputActionValueType::Boolean;
	InputMapping->MapKey(JumpAction, EKeys::SpaceBar);

	UInputAction* InteractAction = NewObject<UInputAction>(InputMapping);
	InteractAction->ValueType = EInputActionValueType::Boolean;
	InputMapping->MapKey(InteractAction, EKeys::E);

	UInputAction* PrimaryInteractAction = NewObject<UInputAction>(InputMapping);
	PrimaryInteractAction->ValueType = EInputActionValueType::Boolean;
	InputMapping->MapKey(PrimaryInteractAction, EKeys::LeftMouseButton);

	UInputAction* DoorInteractAction = NewObject<UInputAction>(InputMapping);
	DoorInteractAction->ValueType = EInputActionValueType::Boolean;
	InputMapping->MapKey(DoorInteractAction, EKeys::RightMouseButton);

	UInputAction* ToggleCollisionDebugAction = NewObject<UInputAction>(InputMapping);
	ToggleCollisionDebugAction->ValueType = EInputActionValueType::Boolean;
	ToggleCollisionDebugAction->bConsumesActionAndAxisMappings = true;
	ToggleCollisionDebugAction->TriggerEventsThatConsumeLegacyKeys = static_cast<int32>(
		ETriggerEvent::Started | ETriggerEvent::Triggered | ETriggerEvent::Completed | ETriggerEvent::Canceled);
	InputMapping->MapKey(ToggleCollisionDebugAction, EKeys::M);

	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFireFighter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFireFighter::Look);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AFireFighter::StartSprint);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AFireFighter::StopSprint);
	Input->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AFireFighter::StopSprint);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AFireFighter::StartJump);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	Input->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ACharacter::StopJumping);
	Input->BindAction(InteractAction, ETriggerEvent::Started, InteractionComponent.Get(), &UPlayerInteractionComponent::Interact);
	Input->BindAction(DoorInteractAction, ETriggerEvent::Started, InteractionComponent.Get(), &UPlayerInteractionComponent::InteractWithDoor);
	Input->BindAction(ToggleCollisionDebugAction, ETriggerEvent::Started, this, &AFireFighter::ToggleCollisionDebug);
	Input->BindAction(PrimaryInteractAction, ETriggerEvent::Started, InteractionComponent.Get(), &UPlayerInteractionComponent::PrimaryInteract);
	Input->BindAction(PrimaryInteractAction, ETriggerEvent::Triggered, InteractionComponent.Get(), &UPlayerInteractionComponent::ContinuePrimaryInteract);
	Input->BindAction(PrimaryInteractAction, ETriggerEvent::Completed, InteractionComponent.Get(), &UPlayerInteractionComponent::EndPrimaryInteract);
	Input->BindAction(PrimaryInteractAction, ETriggerEvent::Canceled, InteractionComponent.Get(), &UPlayerInteractionComponent::EndPrimaryInteract);
	Subsystem->AddMappingContext(InputMapping, 0);
	InputSubsystem = Subsystem;
	UE_LOG(LogDoorInteraction, Log, TEXT("InteractionInputReady: Player=%s CapsuleOverlapEvents=%d"),
		*GetName(), GetCapsuleComponent()->GetGenerateOverlapEvents());
}

void AFireFighter::ToggleCollisionDebug()
{
	SetCollisionDebugVisible(!bCollisionDebugVisible);
}

void AFireFighter::SetCollisionDebugVisible(bool bVisible)
{
	bCollisionDebugVisible = bVisible;
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* Viewport = World->GetGameViewport())
		{
			Viewport->EngineShowFlags.SetCollision(bVisible);
		}
	}

#if ENABLE_DRAW_DEBUG
	if (IConsoleVariable* DrawSpray = IConsoleManager::Get().FindConsoleVariable(TEXT("ffs.DebugSpray")))
	{
		DrawSpray->Set(bVisible ? 1 : 0, ECVF_SetByCode);
	}
#endif

	UE_LOG(LogTemp, Log, TEXT("Collision debug drawing: %s"), bVisible ? TEXT("On") : TEXT("Off"));
}

void AFireFighter::DestroyPlayerInputComponent()
{
	StopSprint();
	StopJumping();
	InteractionComponent->EndPrimaryInteract();

	if (InputMapping && InputSubsystem.IsValid())
	{
		InputSubsystem->RemoveMappingContext(InputMapping);
	}
	InputMapping = nullptr;
	InputSubsystem.Reset();

	Super::DestroyPlayerInputComponent();
}

void AFireFighter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawOnly(0.0, GetControlRotation().Yaw, 0.0);

	AddMovementInput(YawOnly.Vector(), Axis.Y);
	AddMovementInput(YawOnly.RotateVector(FVector::RightVector), Axis.X);
}

void AFireFighter::Look(const FInputActionValue& Value)
{
	AController* PlayerController = GetController();
	if (!PlayerController || PlayerController->IsLookInputIgnored())
	{
		return;
	}

	const FVector2D MouseDelta = Value.Get<FVector2D>();
	FRotator View = PlayerController->GetControlRotation().GetNormalized();
	View.Yaw = FRotator::NormalizeAxis(View.Yaw + MouseDelta.X * MouseSensitivity);
	View.Pitch = FMath::Clamp(View.Pitch + MouseDelta.Y * MouseSensitivity, -85.0, 85.0);
	View.Roll = 0.0;
	PlayerController->SetControlRotation(View);
}

void AFireFighter::StartSprint()
{
	if (bIsSprinting || (GetController() && GetController()->IsMoveInputIgnored()))
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	WalkSpeedBeforeSprint = Movement->MaxWalkSpeed;
	Movement->MaxWalkSpeed = FMath::Max(WalkSpeedBeforeSprint, SprintSpeed);
	bIsSprinting = true;
}

void AFireFighter::StopSprint()
{
	if (!bIsSprinting)
	{
		return;
	}

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeedBeforeSprint;
	bIsSprinting = false;
}

void AFireFighter::StartJump()
{
	if (!GetController() || !GetController()->IsMoveInputIgnored())
	{
		Jump();
	}
}

