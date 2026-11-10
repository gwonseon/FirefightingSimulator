#include "Player/FireFighterPlayerController.h"

#include "Engine/World.h"
#include "GameFramework/PawnMovementComponent.h"
#include "MapObject/Fire/FireActor.h"
#include "MapObjects/LockableDoor.h"
#include "MapObjects/WorldExtinguisher.h"
#include "Player/FireFighter.h"
#include "Player/PlayerInteractionComponent.h"
#include "TimerManager.h"
#include "UI/HUDWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogExtinguisherInspection, Log, All);

void AFireFighterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController() || !HUDWidgetClass || HUDWidget)
	{
		return;
	}

	HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
	if (!HUDWidget || !HUDWidget->AddToPlayerScreen())
	{
		HUDWidget = nullptr;
		return;
	}

	HUDWidget->SetCrosshairVisible(true);
	HUDWidget->HideFireStatus();
	HUDWidget->HideActionHint();
	OnPossessedPawnChanged.AddDynamic(this, &AFireFighterPlayerController::HandleControlledPawnChanged);
	HandleControlledPawnChanged(nullptr, GetPawn());
	RefreshFocusedTarget();

	constexpr float AimFocusInterval = 0.05f;
	GetWorldTimerManager().SetTimer(AimFocusTimer, this,
		&AFireFighterPlayerController::RefreshFocusedTarget, AimFocusInterval, true);
}

void AFireFighterPlayerController::BeginExtinguisherInspection(AWorldExtinguisher* Extinguisher)
{
	AFireFighter* PlayerPawn = Cast<AFireFighter>(GetPawn());
	if (!IsLocalController() || !PlayerPawn || !IsValid(Extinguisher) || IsInspectingExtinguisher())
	{
		return;
	}

	PreviousViewTarget = GetViewTarget();
	bPreviousShowMouseCursor = bShowMouseCursor;
	InspectedExtinguisher = Extinguisher;
	Extinguisher->OnEndPlay.AddUniqueDynamic(this, &AFireFighterPlayerController::HandleInspectedExtinguisherEndPlay);
	bRestoreHoseAfterInspection = PlayerPawn->GetEquippedNozzle() != nullptr;
	if (bRestoreHoseAfterInspection)
	{
		PlayerPawn->SetHoseEquipped(false);
	}

	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	PlayerPawn->ConsumeMovementInputVector();
	PlayerPawn->StopJumping();
	if (UPawnMovementComponent* Movement = PlayerPawn->GetMovementComponent())
	{
		Movement->StopMovementImmediately();
	}

	constexpr float InspectionBlendTime = 0.2f;
	PinDragAvailableAt = GetWorld()->GetTimeSeconds() + InspectionBlendTime;
	SetViewTargetWithBlend(Extinguisher, InspectionBlendTime);
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	if (HUDWidget)
	{
		HUDWidget->SetCrosshairVisible(false);
		RefreshExtinguisherStatus();
		GetWorldTimerManager().ClearTimer(FireCompletionTimer);
		RefreshFocusedTarget();
		HUDWidget->HideFireStatus();
	}
	UE_LOG(LogExtinguisherInspection, Log, TEXT("InspectionStarted: Extinguisher=%s"), *GetNameSafe(Extinguisher));
}

void AFireFighterPlayerController::BeginExtinguisherPinDrag()
{
	if (!IsValid(InspectedExtinguisher) || GetWorld()->GetTimeSeconds() < PinDragAvailableAt)
	{
		return;
	}

	FVector RayOrigin, RayDirection;
	constexpr double PinTraceDistance = 1000.0;
	if (!DeprojectMousePositionToWorld(RayOrigin, RayDirection) ||
		!InspectedExtinguisher->CanGrabPin(RayOrigin, RayOrigin + RayDirection * PinTraceDistance))
	{
		return;
	}

	FVector RestLocation, PulledLocation;
	InspectedExtinguisher->GetPinPullLocations(RestLocation, PulledLocation);
	FVector2D RestScreen, PulledScreen;
	double MouseY;
	if (ProjectWorldLocationToScreen(RestLocation, RestScreen) &&
		ProjectWorldLocationToScreen(PulledLocation, PulledScreen) &&
		GetMousePosition(PinDragStartX, MouseY))
	{
		PinDragScreenDistance = FMath::Max(RestScreen.X - PulledScreen.X, 1.0);
	}
}

void AFireFighterPlayerController::UpdateExtinguisherPinDrag()
{
	if (PinDragScreenDistance <= 0.0 || !IsValid(InspectedExtinguisher))
	{
		return;
	}

	double MouseX, MouseY;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	const double Progress = FMath::Clamp((PinDragStartX - MouseX) / PinDragScreenDistance, 0.0, 1.0);
	InspectedExtinguisher->SetPinPullProgress(static_cast<float>(Progress));
	if (Progress < 1.0)
	{
		return;
	}

	APawn* PlayerPawn = GetPawn();
	UPlayerInteractionComponent* Interaction = PlayerPawn
		? PlayerPawn->FindComponentByClass<UPlayerInteractionComponent>() : nullptr;
	if (!Interaction || !Interaction->EquipExtinguisher(*InspectedExtinguisher))
	{
		EndExtinguisherPinDrag();
		return;
	}

	bRestoreHoseAfterInspection = false;
	UE_LOG(LogExtinguisherInspection, Log, TEXT("ExtinguisherEquipped: Extinguisher=%s RemainingAgent=%.1f"),
		*GetNameSafe(InspectedExtinguisher), InspectedExtinguisher->GetRemainingAgent());
	EndExtinguisherInspection();
}

void AFireFighterPlayerController::EndExtinguisherPinDrag()
{
	if (PinDragScreenDistance > 0.0 && IsValid(InspectedExtinguisher))
	{
		InspectedExtinguisher->SetPinPullProgress(0.0f);
	}
	PinDragScreenDistance = 0.0;
}

void AFireFighterPlayerController::EndExtinguisherInspection()
{
	if (!IsInspectingExtinguisher())
	{
		return;
	}

	EndExtinguisherPinDrag();
	InspectedExtinguisher->OnEndPlay.RemoveDynamic(this, &AFireFighterPlayerController::HandleInspectedExtinguisherEndPlay);
	InspectedExtinguisher = nullptr;
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	bShowMouseCursor = bPreviousShowMouseCursor;
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(false);
	SetInputMode(InputMode);

	AActor* ReturnTarget = PreviousViewTarget.Get();
	SetViewTarget(ReturnTarget ? ReturnTarget : GetPawn());
	PreviousViewTarget.Reset();
	if (HUDWidget)
	{
		HUDWidget->SetCrosshairVisible(true);
	}
	if (AFireFighter* PlayerPawn = Cast<AFireFighter>(GetPawn()); PlayerPawn && bRestoreHoseAfterInspection)
	{
		PlayerPawn->SetHoseEquipped(true);
	}
	bRestoreHoseAfterInspection = false;
	RefreshExtinguisherStatus();
	if (HUDWidget)
	{
		RefreshFocusedTarget();
	}
	UE_LOG(LogExtinguisherInspection, Log, TEXT("InspectionEnded: ViewTarget=%s"), *GetNameSafe(GetViewTarget()));
}

void AFireFighterPlayerController::HandleInspectedExtinguisherEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	EndExtinguisherInspection();
}

void AFireFighterPlayerController::HandleControlledPawnChanged(APawn* /*OldPawn*/, APawn* NewPawn)
{
	if (UPlayerInteractionComponent* PreviousSource = ExtinguisherStatusSource.Get())
	{
		PreviousSource->OnExtinguisherStatusChanged.RemoveAll(this);
	}

	UPlayerInteractionComponent* NewSource = NewPawn
		? NewPawn->FindComponentByClass<UPlayerInteractionComponent>() : nullptr;
	ExtinguisherStatusSource = NewSource;
	if (NewSource)
	{
		NewSource->OnExtinguisherStatusChanged.AddUObject(
			this, &AFireFighterPlayerController::HandleExtinguisherStatusChanged);
	}

	RefreshExtinguisherStatus();
}

void AFireFighterPlayerController::RefreshExtinguisherStatus()
{
	const UPlayerInteractionComponent* Source = ExtinguisherStatusSource.Get();
	HandleExtinguisherStatusChanged(Source && Source->IsExtinguisherEquipped(),
		Source ? Source->GetAgentRatio() : 0.0f);
}

void AFireFighterPlayerController::HandleExtinguisherStatusChanged(bool bEquipped, float AgentRatio)
{
	const UPlayerInteractionComponent* Source = ExtinguisherStatusSource.Get();
	const TWeakObjectPtr<const AWorldExtinguisher> EquippedExtinguisher(
		Source ? Source->GetEquippedExtinguisher() : nullptr);
	if (!LastGuidedExtinguisher.HasSameIndexAndSerialNumber(EquippedExtinguisher))
	{
		LastGuidedExtinguisher = EquippedExtinguisher;
		constexpr double SprayHintDuration = 3.0;
		SprayHintUntil = bEquipped && AgentRatio > 0.0f
			? GetWorld()->GetTimeSeconds() + SprayHintDuration : 0.0;
	}

	if (HUDWidget)
	{
		if (IsValid(InspectedExtinguisher))
		{
			HUDWidget->SetExtinguisherStatus(true, InspectedExtinguisher->GetAgentRatio());
		}
		else
		{
			HUDWidget->SetExtinguisherStatus(bEquipped, AgentRatio);
		}
	}
}

void AFireFighterPlayerController::UpdateDoorPrompt(ALockableDoor* Door, const AFireFighter* PlayerPawn)
{
	if (ALockableDoor* PreviousDoor = FocusedDoor.Get(); PreviousDoor && PreviousDoor != Door)
	{
		PreviousDoor->UpdateInteractionPrompt(nullptr);
	}
	FocusedDoor = Door;
	if (Door)
	{
		Door->UpdateInteractionPrompt(PlayerPawn);
	}
}

void AFireFighterPlayerController::RefreshTrainingGuidance(const AActor* Target)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (IsInspectingExtinguisher())
	{
		HUDWidget->SetTrainingObjective(NSLOCTEXT("TrainingHUD", "RemovePin", "소화기의 안전핀을 제거하세요."));
		SetActionHint(Now >= PinDragAvailableAt ? EActionHint::PullPin : EActionHint::None);
		return;
	}

	const UPlayerInteractionComponent* Source = ExtinguisherStatusSource.Get();
	const bool bEquipped = Source && Source->IsExtinguisherEquipped();
	const bool bCanSpray = bEquipped && Source->GetAgentRatio() > 0.0f;
	const AWorldExtinguisher* AimedExtinguisher = Cast<AWorldExtinguisher>(Target);
	const bool bPinRemoved = AimedExtinguisher && AimedExtinguisher->HasRemovedPin();

	FText Objective;
	if (bCanSpray)
	{
		Objective = bLookingForAnotherFire
			? NSLOCTEXT("TrainingHUD", "FindAnotherFire", "다른 화재가 있는지 확인하세요.")
			: NSLOCTEXT("TrainingHUD", "AimAndSpray", "불꽃의 밑부분을 향해 분사하세요.");
	}
	else if (bEquipped)
	{
		Objective = NSLOCTEXT("TrainingHUD", "FindAnotherExtinguisher", "다른 소화기를 찾으세요.");
	}
	else if (AimedExtinguisher)
	{
		Objective = bPinRemoved
			? NSLOCTEXT("TrainingHUD", "Equip", "소화기를 장착하세요.")
			: NSLOCTEXT("TrainingHUD", "RemovePin", "소화기의 안전핀을 제거하세요.");
	}
	else
	{
		Objective = NSLOCTEXT("TrainingHUD", "FindFireAndExtinguisher", "화재 위치를 확인하고 소화기를 찾으세요.");
	}
	HUDWidget->SetTrainingObjective(Objective);

	if (AimedExtinguisher)
	{
		SetActionHint(bPinRemoved ? EActionHint::EquipExtinguisher : EActionHint::InspectExtinguisher);
	}
	else
	{
		SetActionHint(bCanSpray && Now < SprayHintUntil ? EActionHint::Spray : EActionHint::None);
	}
}

void AFireFighterPlayerController::SetActionHint(EActionHint Hint)
{
	if (ActiveActionHint == Hint)
	{
		return;
	}
	ActiveActionHint = Hint;

	switch (Hint)
	{
	case EActionHint::InspectExtinguisher:
		HUDWidget->ShowActionHint(NSLOCTEXT("TrainingHUD", "Inspect", "소화기 검사"), false);
		break;
	case EActionHint::EquipExtinguisher:
		HUDWidget->ShowActionHint(NSLOCTEXT("TrainingHUD", "EquipHint", "소화기 장착"), false);
		break;
	case EActionHint::PullPin:
		HUDWidget->ShowActionHint(
			NSLOCTEXT("TrainingHUD", "PullPin", "← 핀을 좌클릭한 채 왼쪽으로 끝까지 드래그하세요."), true);
		break;
	case EActionHint::Spray:
		HUDWidget->ShowActionHint(NSLOCTEXT("TrainingHUD", "Spray", "좌클릭을 누르고 있으면 분사합니다."), false);
		break;
	case EActionHint::None:
		HUDWidget->HideActionHint();
		break;
	}
}

void AFireFighterPlayerController::RefreshFocusedTarget()
{
	const AFireFighter* PlayerPawn = IsInspectingExtinguisher() ? nullptr : Cast<AFireFighter>(GetPawn());
	const UPlayerInteractionComponent* Interaction = PlayerPawn
		? PlayerPawn->FindComponentByClass<UPlayerInteractionComponent>() : nullptr;
	AActor* Target = Interaction ? Interaction->FindAimedActor() : nullptr;
	UpdateDoorPrompt(Cast<ALockableDoor>(Target), PlayerPawn);

	AFireActor* NewFire = Cast<AFireActor>(Target);
	if (NewFire && (!NewFire->ActorHasTag(AFireActor::AimTag) || NewFire->GetIntensityRatio() <= 0.0f))
	{
		NewFire = nullptr;
	}
	if (NewFire)
	{
		bLookingForAnotherFire = false;
	}
	RefreshTrainingGuidance(Target);

	const TWeakObjectPtr<AFireActor> NewFocus(NewFire);
	if (FocusedFire.HasSameIndexAndSerialNumber(NewFocus))
	{
		return;
	}

	if (AFireActor* PreviousFire = FocusedFire.Get())
	{
		PreviousFire->OnIntensityChanged.RemoveAll(this);
	}
	FocusedFire = NewFocus;

	if (NewFire)
	{
		GetWorldTimerManager().ClearTimer(FireCompletionTimer);
		NewFire->OnIntensityChanged.AddUObject(
			this, &AFireFighterPlayerController::HandleFireIntensityChanged);
		HUDWidget->SetFireStatus(NewFire->GetIntensityRatio());
	}
	else if (!GetWorldTimerManager().IsTimerActive(FireCompletionTimer))
	{
		HUDWidget->HideFireStatus();
	}
}

void AFireFighterPlayerController::HandleFireIntensityChanged(float IntensityRatio)
{
	if (IntensityRatio > 0.0f)
	{
		HUDWidget->SetFireStatus(IntensityRatio);
		return;
	}

	bLookingForAnotherFire = true;
	HUDWidget->ShowFireExtinguished();
	constexpr float FireCompletionDuration = 2.0f;
	GetWorldTimerManager().SetTimer(FireCompletionTimer, HUDWidget.Get(),
		&UHUDWidget::HideFireStatus, FireCompletionDuration, false);
}

void AFireFighterPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnPossessedPawnChanged.RemoveDynamic(this, &AFireFighterPlayerController::HandleControlledPawnChanged);
	HandleControlledPawnChanged(GetPawn(), nullptr);
	EndExtinguisherInspection();
	GetWorldTimerManager().ClearTimer(AimFocusTimer);
	UpdateDoorPrompt(nullptr, nullptr);
	GetWorldTimerManager().ClearTimer(FireCompletionTimer);
	if (AFireActor* Fire = FocusedFire.Get())
	{
		Fire->OnIntensityChanged.RemoveAll(this);
	}
	FocusedFire.Reset();

	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
		HUDWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AFireFighterPlayerController::OnUnPossess()
{
	EndExtinguisherInspection();
	Super::OnUnPossess();
	UpdateDoorPrompt(nullptr, nullptr);
}
