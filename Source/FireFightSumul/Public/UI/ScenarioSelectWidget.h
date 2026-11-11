#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScenarioSelectWidget.generated.h"

class UButton;
class UFireScenarioDataAsset;
class UImage;
class UTexture2D;

UCLASS(Abstract, Blueprintable)
class FIREFIGHTSUMUL_API UScenarioSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UScenarioSelectWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scenarios")
	TObjectPtr<UFireScenarioDataAsset> TrashBinScenario;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scenarios")
	TObjectPtr<UFireScenarioDataAsset> KitchenScenario;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scenarios")
	TObjectPtr<UFireScenarioDataAsset> RandomScenario;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection Visual")
	TObjectPtr<UTexture2D> SelectedFrameTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection Visual")
	TObjectPtr<UTexture2D> UnselectedFrameTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection Visual")
	FLinearColor SelectedThumbnailTint = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection Visual")
	FLinearColor UnselectedThumbnailTint = FLinearColor(FColor::FromHex(TEXT("737373FF")));

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TrashBinCardButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> KitchenCardButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RandomCardButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartTrainingButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TrashBinThumbnailImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> KitchenThumbnailImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> RandomThumbnailImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TrashBinFrameImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> KitchenFrameImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> RandomFrameImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TrashBinSelectedIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> KitchenSelectedIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> RandomSelectedIcon;

private:
	UPROPERTY(Transient)
	TObjectPtr<UFireScenarioDataAsset> CurrentScenario;

	void SetSelectedScenario(UFireScenarioDataAsset* Scenario);
	void UpdateSelectionVisuals();
	void ApplyCardVisuals(UFireScenarioDataAsset* Scenario, UImage* Thumbnail, UImage* Frame,
		UImage* SelectedIcon) const;

	UFUNCTION()
	void HandleTrashBinClicked();

	UFUNCTION()
	void HandleKitchenClicked();

	UFUNCTION()
	void HandleRandomClicked();

	UFUNCTION()
	void HandleStartTrainingClicked();
};
