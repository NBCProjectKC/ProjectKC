#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeListWidget.h"

#include "Components/ListView.h"
#include "Components/VerticalBox.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeEntryWidget.h"

void UKCHUDRecipeListWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UKCHUDRecipeListWidget::SetRecipes(const TArray<FKCRecipeViewData>& Recipes)
{
	BP_OnRecipesSet(Recipes);

	RecipeItems.Reset();
	RecipeItems.Reserve(Recipes.Num());

	TArray<UObject*> ListItems;
	ListItems.Reserve(Recipes.Num());

	for (const FKCRecipeViewData& Recipe : Recipes)
	{
		UKCHUDRecipeListItem* Item = NewObject<UKCHUDRecipeListItem>(this);
		Item->Recipe = Recipe;
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
