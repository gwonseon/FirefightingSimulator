#pragma once

#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "PlayerInteractionComponent.generated.h"

class AFireActor;
class AFireFighter;
class AWorldExtinguisher;
class USceneComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnExtinguisherStatusChanged, bool, float);

UCLASS(ClassGroup = (Player))
class FIREFIGHTSUMUL_API UPlayerInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void Interact();
	void PrimaryInteract();
	void InteractWithDoor();
	void ContinuePrimaryInteract();
	void EndPrimaryInteract();
	bool EquipExtinguisher(AWorldExtinguisher& Extinguisher);

	AActor* FindAimedActor() const;

	bool IsExtinguisherEquipped() const;
	const AWorldExtinguisher* GetEquippedExtinguisher() const;
	float GetAgentRatio() const;
	void NotifyExtinguisherStatusChanged();

	FOnExtinguisherStatusChanged OnExtinguisherStatusChanged;

private:
	AActor* FindFocusedActor(const AFireFighter& Player, ECollisionChannel Channel, const TCHAR* InputName = nullptr) const;
	void ApplySpray(const USceneComponent& Nozzle, float SuppressionAmount) const;

	TWeakObjectPtr<AWorldExtinguisher> EquippedExtinguisher;
	bool bSpraying = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extinguisher",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.01", ToolTip = "Agent used per second of continuous spraying. The default empties a full 100-unit ABC extinguisher in 12 seconds."))
	float AgentPerSecond = 100.0f / 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extinguisher",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ToolTip = "Fire intensity removed per unit of agent before charge, distance, and angle falloff."))
	float SuppressionPerUnit = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extinguisher",
		meta = (AllowPrivateAccess = "true", ClampMin = "1.0", Units = "cm"))
	float SprayRange = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extinguisher",
		meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "80.0", Units = "deg"))
	float SprayHalfAngle = 15.0f;

	UPROPERTY(EditAnywhere, Category = "Interaction", meta = (ClampMin = "0.0", Units = "cm"))
	float InteractionDistance = 300.0f;
};
