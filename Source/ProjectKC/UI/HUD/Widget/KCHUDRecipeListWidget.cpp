#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeListWidget.h"

#include "Components/ListView.h"
#include "Components/VerticalBox.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeEntryWidget.h"

void UKCHUDRecipeListWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (RecipeListView)
	{
		if (const TSubclassOf<UKCHUDRecipeEntryWidget> ResolvedEntryClass = ResolveEntryWidgetClass())
		{
			RecipeListView->SetEntryWidgetClass(ResolvedEntryClass);
		}
	}
}

void UKCHUDRecipeListWidget::SetRecipes(const TArray<FKCRecipeViewData>& Recipes)
{
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
		if (const TSubclassOf<UKCHUDRecipeEntryWidget> ResolvedEntryClass = ResolveEntryWidgetClass())
		{
			RecipeListView->SetEntryWidgetClass(ResolvedEntryClass);
		}
		RecipeListView->SetListItems(ListItems);
		return;
	}

	if (!RecipeEntryContainer)
	{
		return;
	}

	RecipeEntryContainer->ClearChildren();

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
		RecipeEntryContainer->AddChildToVerticalBox(EntryWidget);
	}
}

TSubclassOf<UKCHUDRecipeEntryWidget> UKCHUDRecipeListWidget::ResolveEntryWidgetClass() const
{
	if (EntryWidgetClass)
	{
		return EntryWidgetClass;
	}

	return LoadClass<UKCHUDRecipeEntryWidget>(
		nullptr,
		TEXT("/Game/KC/UI/HUD/Recipe/WBP_HUDRecipeEntry.WBP_HUDRecipeEntry_C"));
}
