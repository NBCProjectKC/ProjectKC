#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "KCHUDWidget.generated.h"

class UTextBlock;
class UKCColorStyle;
class UKCHUDRecipeListWidget;
class UKCHUDViewModel;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCHUDWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UKCHUDViewModel* GetHUDViewModel() const { return HUDViewModel; }

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UKCHUDRecipeListWidget* GetRecipeListWidget() const { return WBP_HUDRecipeList; }

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UKCHUDRecipeListWidget> WBP_HUDRecipeList;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "KC|UI")
	TObjectPtr<UKCHUDViewModel> HUDViewModel;

	virtual void NativeApplyColorStyle(const UKCColorStyle* InColorStyle) override;

private:
	void HandleTeamScoresChanged(const TArray<int32>& TeamScores);
	void HandleRecipesChanged();
	void RefreshHUD();
	void RefreshScore();
	void RefreshRecipes();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Team1ScoreText;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Team2ScoreText;
};
