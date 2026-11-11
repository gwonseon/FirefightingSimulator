// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FireFighter.generated.h"

class UCameraComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputMappingContext;
class UNiagaraComponent;
class UNiagaraSystem;
class UPlayerInteractionComponent;
class USceneComponent;
class USkeletalMeshComponent;
struct FInputActionValue;

UCLASS()
class FIREFIGHTSUMUL_API AFireFighter : public ACharacter
{
	GENERATED_BODY()

public:
	AFireFighter();

	void AddKey(FName KeyId);

	UFUNCTION(BlueprintPure, Category = "Keys")
	bool HasKey(FName KeyId) const;

	UFUNCTION(BlueprintCallable, Category = "Extinguisher")
	void SetHoseEquipped(bool bEquipped);

	const USceneComponent* GetEquippedNozzle() const;
	void StartSprayEffect(UNiagaraSystem* System, float Strength);
	void SetSprayEffectStrength(float Strength);
	void StopSprayEffect();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void DestroyPlayerInputComponent() override;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void StartJump();
	void ToggleFireCollision();

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extinguisher", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> NozzleTip;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extinguisher", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> SprayEffectComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Extinguisher|Effects",
		meta = (ClampMin = "1.0", ClampMax = "3.0"))
	float SprayVisualWidthScale = 1.65f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPlayerInteractionComponent> InteractionComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Extinguisher", meta = (ClampMin = "0.0"))
	float HoseSwayStrength = 1.0f;

	TWeakObjectPtr<USkeletalMeshComponent> EquippedHose;
	FTransform HoseRestTransform = FTransform::Identity;
	float HoseSwayPhase = 0.0f;
	float HoseSwayWeight = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (ClampMin = "0.0"))
	float MouseSensitivity = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float SprintSpeed = 650.0f;

	float WalkSpeedBeforeSprint = 0.0f;
	bool bIsSprinting = false;
	TSet<FName> OwnedKeyIds;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> InputMapping;

	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSubsystem;
};
