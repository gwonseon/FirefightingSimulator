#include "Game/FireTrainingGameInstance.h"

#include "Data/FireScenarioDataAsset.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericPlatformProperties.h"
#include "HAL/PlatformMisc.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	class FEscapeToQuitInputProcessor final : public IInputProcessor
	{
	public:
		virtual void Tick(const float /*DeltaTime*/, FSlateApplication& /*SlateApplication*/,
			TSharedRef<ICursor> /*Cursor*/) override
		{
		}

		virtual bool HandleKeyDownEvent(FSlateApplication& /*SlateApplication*/,
			const FKeyEvent& KeyEvent) override
		{
			if (KeyEvent.GetKey() != EKeys::Escape)
			{
				return false;
			}

			FPlatformMisc::RequestExit(false);
			return true;
		}
	};
}

namespace FireTrainingLevels
{
	const FName Start(TEXT("/Game/_Map/start"));
	const FName Select(TEXT("/Game/_Map/select"));
	const FName Main(TEXT("/Game/_Map/Main"));
}

void UFireTrainingGameInstance::Init()
{
	Super::Init();

	// Cooked clients are packaged builds. Keep the editor's normal Escape behavior intact.
	if (FPlatformProperties::RequiresCookedData() && !IsRunningDedicatedServer()
		&& !IsRunningCommandlet() && FSlateApplication::IsInitialized())
	{
		EscapeToQuitInputProcessor = MakeShared<FEscapeToQuitInputProcessor>();
		FSlateApplication::Get().RegisterInputPreProcessor(EscapeToQuitInputProcessor);
	}
}

void UFireTrainingGameInstance::Shutdown()
{
	if (EscapeToQuitInputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(EscapeToQuitInputProcessor);
	}
	EscapeToQuitInputProcessor.Reset();

	Super::Shutdown();
}

bool UFireTrainingGameInstance::SelectScenario(UFireScenarioDataAsset* InScenario)
{
	if (!IsValid(InScenario))
	{
		UE_LOG(LogTemp, Warning, TEXT("SelectScenario was called without a valid scenario Data Asset."));
		return false;
	}

	SelectedScenario = InScenario;
	return true;
}

void UFireTrainingGameInstance::SetFireGrowthSpeedMode(EFireGrowthSpeedMode InMode)
{
	FireGrowthSpeedMode = InMode;
}

bool UFireTrainingGameInstance::StartSelectedScenario()
{
	if (!IsValid(SelectedScenario))
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot start training because no scenario is selected."));
		return false;
	}

	FName LevelName = FireTrainingLevels::Main;
	if (!SelectedScenario->ScenarioLevel.IsNull())
	{
		const FString PackageName = SelectedScenario->ScenarioLevel.ToSoftObjectPath().GetLongPackageName();
		if (!PackageName.IsEmpty())
		{
			LevelName = FName(*PackageName);
		}
	}

	ClearTrainingResult();
	UGameplayStatics::OpenLevel(this, LevelName);
	return true;
}

void UFireTrainingGameInstance::OpenScenarioSelect()
{
	ClearTrainingResult();
	UGameplayStatics::OpenLevel(this, FireTrainingLevels::Select);
}

void UFireTrainingGameInstance::OpenStartMenu()
{
	SelectedScenario = nullptr;
	ClearTrainingResult();
	UGameplayStatics::OpenLevel(this, FireTrainingLevels::Start);
}

void UFireTrainingGameInstance::SetTrainingResult(const FTrainingResult& InResult)
{
	TrainingResult = InResult;
	bHasTrainingResult = true;
}

void UFireTrainingGameInstance::ClearTrainingResult()
{
	TrainingResult = FTrainingResult();
	bHasTrainingResult = false;
}
