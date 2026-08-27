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

	UVerticalBox* EntryContainer = GetRecipeEntryContainer();
	if (!EntryContainer)
	{
		return;
	}

	EntryContainer->ClearChildren();

	const TSubclassOf<UKCHUDRecipeEntryWidget> ResolvedEntryClass = ResolveEntryWidgetClass();
	if (!ResolvedEntryClass)
	{
		return;
	}

	for (const FKCRecipeViewData& Recipe : Recipes)
	{
		UKCHUDRecipeEntryWidget* EntryWidget = CreateWidget<UKCHUDRecipeEntryWidget>(this, ResolvedEntryClass);
		if (!EntryWidget)
		{
			continue;
		}

		EntryWidget->SetRecipe(Recipe);
		EntryContainer->AddChildToVerticalBox(EntryWidget);
	}
}

TSubclassOf<UKCHUDRecipeEntryWidget> UKCHUDRecipeListWidget::ResolveEntryWidgetClass() const
{
	return EntryWidgetClass;
}

UVerticalBox* UKCHUDRecipeListWidget::GetRecipeEntryContainer() const
{
	return RecipeEntryContainer ? RecipeEntryContainer.Get() : VerticalBox.Get();
}
