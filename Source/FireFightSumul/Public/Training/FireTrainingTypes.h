#pragma once

#include "CoreMinimal.h"
#include "FireTrainingTypes.generated.h"

UENUM(BlueprintType)
enum class EFireStartSelectionMode : uint8
{
	Fixed = 0 UMETA(DisplayName = "Fixed"),
	Random = 1 UMETA(DisplayName = "Random")
};

UENUM(BlueprintType)
enum class EFireClass : uint8
{
	A = 0 UMETA(DisplayName = "Class A - Ordinary Combustibles"),
	B = 1 UMETA(DisplayName = "Class B - Flammable Liquids"),
	C = 2 UMETA(DisplayName = "Class C - Energized Electrical"),
	K = 3 UMETA(DisplayName = "Class K - Cooking Oils and Fats")
};

UENUM(BlueprintType)
enum class EFireScenarioState : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Ready = 1 UMETA(DisplayName = "Ready"),
	Active = 2 UMETA(DisplayName = "Active"),
	ExtinguishmentMonitoring = 3 UMETA(DisplayName = "Extinguishment Monitoring"),
	EvacuationRequired = 4 UMETA(DisplayName = "Evacuation Required"),
	Completed = 5 UMETA(DisplayName = "Completed")
};

UENUM(BlueprintType)
enum class ETrainingOutcome : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Extinguished = 1 UMETA(DisplayName = "Extinguished"),
	Evacuated = 2 UMETA(DisplayName = "Evacuated"),
	Failed = 3 UMETA(DisplayName = "Failed")
};

UENUM(BlueprintType)
enum class ETrainingFailureReason : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	EnteredDangerZone = 1 UMETA(DisplayName = "Entered Danger Zone"),
	IgnoredEvacuation = 2 UMETA(DisplayName = "Ignored Evacuation"),
	ScenarioTimeout = 3 UMETA(DisplayName = "Scenario Timeout"),
	TriggeredBackdraft = 4 UMETA(DisplayName = "Triggered Backdraft"),
	Aborted = 5 UMETA(DisplayName = "Aborted")
};

UENUM(BlueprintType)
enum class EExtinguisherState : uint8
{
	Unknown = 0 UMETA(DisplayName = "Unknown"),
	Normal = 1 UMETA(DisplayName = "Normal"),
	Low = 2 UMETA(DisplayName = "Low"),
	Empty = 3 UMETA(DisplayName = "Empty")
};

UENUM(BlueprintType)
enum class EFireState : uint8
{
	Inactive = 0 UMETA(DisplayName = "Inactive"),
	Burning = 1 UMETA(DisplayName = "Burning"),
	ExtinguishmentMonitoring = 2 UMETA(DisplayName = "Extinguishment Monitoring"),
	Extinguished = 3 UMETA(DisplayName = "Extinguished")
};

UENUM(BlueprintType)
enum class ETrainingFeedbackType : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	TooClose = 1 UMETA(DisplayName = "Too Close"),
	TooFar = 2 UMETA(DisplayName = "Too Far"),
	AimTooHigh = 3 UMETA(DisplayName = "Aim Too High"),
	OffCenter = 4 UMETA(DisplayName = "Off Center"),
	Occluded = 5 UMETA(DisplayName = "Occluded"),
	ExtinguisherLow = 6 UMETA(DisplayName = "Extinguisher Low"),
	ExtinguisherEmpty = 7 UMETA(DisplayName = "Extinguisher Empty"),
	EvacuateNow = 8 UMETA(DisplayName = "Evacuate Now"),
	GoodAim = 9 UMETA(DisplayName = "Good Aim"),
	Reignited = 10 UMETA(DisplayName = "Reignited"),
	Monitoring = 11 UMETA(DisplayName = "Monitoring"),
	WrongExtinguisher = 12 UMETA(DisplayName = "Wrong Extinguisher"),
	SweepRequired = 13 UMETA(DisplayName = "Sweep Required")
};

USTRUCT(BlueprintType)
struct FIREFIGHTSUMUL_API FSimulationSourceInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	FText SourceTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	FString SourceUrl;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source")
	FText SourceSection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Source", meta = (MultiLine = true))
	FText CalibrationNote;
};

USTRUCT(BlueprintType)
struct FIREFIGHTSUMUL_API FSuppressionSample
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression", meta = (ClampMin = "0.0", Units = "s"))
	float SampleDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression", meta = (ClampMin = "0.0", Units = "kg"))
	float UsedAgentKg = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression", meta = (ClampMin = "0.0", Units = "cm"))
	float DistanceCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DistanceFactor = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ConeCenterFactor = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AimHeight01 = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AimHeightFactor = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExtinguisherStateFactor = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression")
	bool bOccluded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suppression", meta = (ClampMin = "0.0"))
	float SuppressionAmount = 0.0f;
};

USTRUCT(BlueprintType)
struct FIREFIGHTSUMUL_API FTrainingFeedbackSetting
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	ETrainingFeedbackType Type = ETrainingFeedbackType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	FText Message;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback", meta = (ClampMin = "0.0", Units = "s"))
	float TriggerHoldSeconds = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback", meta = (ClampMin = "0.0", Units = "s"))
	float DisplaySeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback", meta = (ClampMin = "0.0", Units = "s"))
	float CooldownSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback")
	bool bCausesImmediateFailure = false;
};

USTRUCT(BlueprintType)
struct FIREFIGHTSUMUL_API FTrainingResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Result")
	ETrainingOutcome Outcome = ETrainingOutcome::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Result")
	ETrainingFailureReason FailureReason = ETrainingFailureReason::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Result", meta = (ClampMin = "0.0", Units = "s"))
	float CompletionTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Result", meta = (ClampMin = "0.0", Units = "kg"))
	float UsedAgentKg = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Result")
	FText ResultMessage;
};

USTRUCT(BlueprintType)
struct FIREFIGHTSUMUL_API FFireMaterialSetting
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material")
	FName MaterialTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material")
	EFireClass FireClass = EFireClass::A;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material")
	bool bCanBurn = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material", meta = (ClampMin = "0.0"))
	float IgnitionThreshold = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material", meta = (ClampMin = "0.0"))
	float HeatReceiveMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material", meta = (ClampMin = "0.0"))
	float GrowthMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material", meta = (ClampMin = "0.0"))
	float SuppressionMultiplier = 1.0f;
};
