#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDRecipeTypes.h"
#include "KCHUDRecipeListWidget.generated.h"

class UListView;
class UVerticalBox;
class UWidgetAnimation;
class UKCHUDRecipeEntryWidget;
class UKCHUDRecipeViewModel;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCHUDRecipeListWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetRecipes(const TArray<FKCRecipeViewData>& Recipes);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void PlayListShake();

	void SetRecipeViewModels(const TArray<TObjectPtr<UKCHUDRecipeViewModel>>& RecipeViewModels);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI", meta = (DisplayName = "On Recipes Set"))
	void BP_OnRecipesSet(const TArray<FKCRecipeViewData>& Recipes);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI")
	TSubclassOf<UKCHUDRecipeEntryWidget> EntryWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UListView> RecipeListView;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UVerticalBox> RecipeEntryContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UVerticalBox> VerticalBox;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ListShake;

private:
	bool CanReuseRecipeItems(const TArray<FKCRecipeViewData>& Recipes) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKCHUDRecipeViewModel>> PreviewRecipeViewModels;
};
