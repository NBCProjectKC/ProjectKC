#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "KCHUDRecipeListWidget.generated.h"

class UListView;
class UVerticalBox;
class UKCHUDRecipeEntryWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FKCRecipeListItemChangedNativeDelegate, const FKCRecipeViewData&);

UCLASS(BlueprintType)
class PROJECTKC_API UKCHUDRecipeListItem : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetRecipe(const FKCRecipeViewData& NewRecipe);

	UPROPERTY(BlueprintReadOnly, Category = "KC|UI")
	FKCRecipeViewData Recipe;

	FKCRecipeListItemChangedNativeDelegate OnRecipeChangedNative;
};

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCHUDRecipeListWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetRecipes(const TArray<FKCRecipeViewData>& Recipes);

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

private:
	bool CanReuseRecipeItems(const TArray<FKCRecipeViewData>& Recipes) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKCHUDRecipeListItem>> RecipeItems;
};
