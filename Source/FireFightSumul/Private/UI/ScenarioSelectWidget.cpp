#include "UI/ScenarioSelectWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Data/FireScenarioDataAsset.h"
#include "Engine/Texture2D.h"
#include "Game/FireTrainingGameInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const TCHAR* SelectedFrameTexturePath =
		TEXT("/Game/_Image/Frame_Scenario_Selected-trimmed_local.Frame_Scenario_Selected-trimmed_local");
	const TCHAR* UnselectedFrameTexturePath =
		TEXT("/Game/_Image/Frame_Scenario_Unselected-trimmed_local.Frame_Scenario_Unselected-trimmed_local");
}

UScenarioSelectWidget::UScenarioSelectWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static const ConstructorHelpers::FObjectFinder<UTexture2D> SelectedFrameAsset(SelectedFrameTexturePath);
	static const ConstructorHelpers::FObjectFinder<UTexture2D> UnselectedFrameAsset(UnselectedFrameTexturePath);

	SelectedFrameTexture = SelectedFrameAsset.Object;
	UnselectedFrameTexture = UnselectedFrameAsset.Object;
}

void UScenarioSelectWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Blueprint class defaults can retain an older texture assignment. Reapply the
	// project defaults so every scenario card uses the tightly cropped frames.
	if (UTexture2D* SelectedFrameAsset = LoadObject<UTexture2D>(nullptr, SelectedFrameTexturePath))
	{
		SelectedFrameTexture = SelectedFrameAsset;
	}
	if (UTexture2D* UnselectedFrameAsset = LoadObject<UTexture2D>(nullptr, UnselectedFrameTexturePath))
	{
		UnselectedFrameTexture = UnselectedFrameAsset;
	}

	TrashBinCardButton->OnClicked.AddUniqueDynamic(this, &UScenarioSelectWidget::HandleTrashBinClicked);
	KitchenCardButton->OnClicked.AddUniqueDynamic(this, &UScenarioSelectWidget::HandleKitchenClicked);
	RandomCardButton->OnClicked.AddUniqueDynamic(this, &UScenarioSelectWidget::HandleRandomClicked);
	StartTrainingButton->OnClicked.AddUniqueDynamic(this, &UScenarioSelectWidget::HandleStartTrainingClicked);
}

void UScenarioSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetSelectedScenario(TrashBinScenario);
}

void UScenarioSelectWidget::SetSelectedScenario(UFireScenarioDataAsset* Scenario)
{
	if (!IsValid(Scenario))
	{
		UE_LOG(LogTemp, Warning, TEXT("The selected scenario card has no Data Asset assigned."));
		return;
	}

	CurrentScenario = Scenario;
	if (UFireTrainingGameInstance* TrainingGameInstance = GetGameInstance<UFireTrainingGameInstance>())
	{
		TrainingGameInstance->SelectScenario(Scenario);
	}
	UpdateSelectionVisuals();
}

void UScenarioSelectWidget::UpdateSelectionVisuals()
{
	ApplyCardVisuals(TrashBinScenario, TrashBinThumbnailImage, TrashBinFrameImage, TrashBinSelectedIcon);
	ApplyCardVisuals(KitchenScenario, KitchenThumbnailImage, KitchenFrameImage, KitchenSelectedIcon);
	ApplyCardVisuals(RandomScenario, RandomThumbnailImage, RandomFrameImage, RandomSelectedIcon);
}

void UScenarioSelectWidget::ApplyCardVisuals(UFireScenarioDataAsset* Scenario, UImage* Thumbnail,
	UImage* Frame, UImage* SelectedIcon) const
{
	const bool bSelected = IsValid(Scenario) && Scenario == CurrentScenario;
	if (Thumbnail)
	{
		Thumbnail->SetColorAndOpacity(bSelected ? SelectedThumbnailTint : UnselectedThumbnailTint);
	}
	if (Frame)
	{
		if (UTexture2D* FrameTexture = bSelected ? SelectedFrameTexture : UnselectedFrameTexture)
		{
			Frame->SetBrushFromTexture(FrameTexture, true);
		}
	}
	if (SelectedIcon)
	{
		SelectedIcon->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UScenarioSelectWidget::HandleTrashBinClicked()
{
	SetSelectedScenario(TrashBinScenario);
}

void UScenarioSelectWidget::HandleKitchenClicked()
{
	SetSelectedScenario(KitchenScenario);
}

void UScenarioSelectWidget::HandleRandomClicked()
{
	SetSelectedScenario(RandomScenario);
}

void UScenarioSelectWidget::HandleStartTrainingClicked()
{
	if (UFireTrainingGameInstance* TrainingGameInstance = GetGameInstance<UFireTrainingGameInstance>())
	{
		TrainingGameInstance->SelectScenario(CurrentScenario);
		TrainingGameInstance->StartSelectedScenario();
	}
}
