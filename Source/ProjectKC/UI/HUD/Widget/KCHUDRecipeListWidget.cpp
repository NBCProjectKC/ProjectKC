#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeListWidget.h"

#include "Components/ListView.h"
#include "Components/VerticalBox.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeEntryWidget.h"

void UKCHUDRecipeListItem::SetRecipe(const FKCRecipeViewData& NewRecipe)
{
	Recipe = NewRecipe;
	OnRecipeChangedNative.Broadcast(Recipe);
}

void UKCHUDRecipeListWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UKCHUDRecipeListWidget::SetRecipes(const TArray<FKCRecipeViewData>& Recipes)
{
	BP_OnRecipesSet(Recipes);

	if (RecipeListView && CanReuseRecipeItems(Recipes))
	{
		for (int32 RecipeIndex = 0; RecipeIndex < Recipes.Num(); ++RecipeIndex)
		{
			RecipeItems[RecipeIndex]->SetRecipe(Recipes[RecipeIndex]);
		}

		return;
	}

	RecipeItems.Reset();
	RecipeItems.Reserve(Recipes.Num());

	TArray<UObject*> ListItems;
	ListItems.Reserve(Recipes.Num());

	for (const FKCRecipeViewData& Recipe : Recipes)
	{
		UKCHUDRecipeListItem* Item = NewObject<UKCHUDRecipeListItem>(this);
		Item->SetRecipe(Recipe);
		RecipeItems.Add(Item);
		ListItems.Add(Item);
	}

	if (RecipeListView)
	{
		RecipeListView->SetListItems(ListItems);
		return;
	}

	UVerticalBox* EntryContainer = RecipeEntryContainer ? RecipeEntryContainer.Get() : VerticalBox.Get();
	if (!EntryContainer)
	{
		return;
	}

	EntryContainer->ClearChildren();
	
	if (!EntryWidgetClass)
	{
		return;
	}

	for (const FKCRecipeViewData& Recipe : Recipes)
	{
		UKCHUDRecipeEntryWidget* EntryWidget = CreateWidget<UKCHUDRecipeEntryWidget>(this, EntryWidgetClass);
		if (!EntryWidget)
		{
			continue;
		}

		EntryWidget->SetRecipe(Recipe);
		EntryContainer->AddChildToVerticalBox(EntryWidget);
	}
}

bool UKCHUDRecipeListWidget::CanReuseRecipeItems(const TArray<FKCRecipeViewData>& Recipes) const
{
	if (RecipeItems.Num() != Recipes.Num())
	{
		return false;
	}

	for (int32 RecipeIndex = 0; RecipeIndex < Recipes.Num(); ++RecipeIndex)
	{
		const UKCHUDRecipeListItem* RecipeItem = RecipeItems[RecipeIndex];
		if (!RecipeItem || RecipeItem->Recipe.RecipeRowName != Recipes[RecipeIndex].RecipeRowName)
		{
			return false;
		}
	}

	return true;
}
