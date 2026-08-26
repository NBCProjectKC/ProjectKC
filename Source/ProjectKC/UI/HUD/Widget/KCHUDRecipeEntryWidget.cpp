#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeEntryWidget.h"

#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeIngredientWidget.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeListWidget.h"

void UKCHUDRecipeEntryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IngredientListView)
	{
		if (const TSubclassOf<UKCHUDRecipeIngredientWidget> ResolvedEntryClass = ResolveIngredientWidgetClass())
		{
			IngredientListView->SetEntryWidgetClass(ResolvedEntryClass);
		}
	}
}

void UKCHUDRecipeEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	if (const UKCHUDRecipeListItem* RecipeItem = Cast<UKCHUDRecipeListItem>(ListItemObject))
	{
		SetRecipe(RecipeItem->Recipe);
	}
}

void UKCHUDRecipeEntryWidget::SetRecipe(const FKCRecipeViewData& Recipe)
{
	if (DifficultyText)
	{
		DifficultyText->SetText(BuildStarsText(Recipe.DifficultyStars));
	}

	if (FoodNameText)
	{
		FoodNameText->SetText(Recipe.DisplayName.IsEmpty() ? FText::FromName(Recipe.RecipeRowName) : Recipe.DisplayName);
	}

	IngredientItems.Reset();
	IngredientItems.Reserve(Recipe.Ingredients.Num());

	TArray<UObject*> ListItems;
	ListItems.Reserve(Recipe.Ingredients.Num());

	for (const FKCRecipeIngredientViewData& Ingredient : Recipe.Ingredients)
	{
		UKCHUDRecipeIngredientListItem* Item = NewObject<UKCHUDRecipeIngredientListItem>(this);
		Item->Ingredient = Ingredient;
		IngredientItems.Add(Item);
		ListItems.Add(Item);
	}

	if (IngredientListView)
	{
		if (const TSubclassOf<UKCHUDRecipeIngredientWidget> ResolvedEntryClass = ResolveIngredientWidgetClass())
		{
			IngredientListView->SetEntryWidgetClass(ResolvedEntryClass);
		}
		IngredientListView->SetListItems(ListItems);
		return;
	}

	if (!IngredientEntryContainer)
	{
		return;
	}

	IngredientEntryContainer->ClearChildren();

	const TSubclassOf<UKCHUDRecipeIngredientWidget> ResolvedEntryClass = ResolveIngredientWidgetClass();
	if (!ResolvedEntryClass)
	{
		return;
	}

	for (const FKCRecipeIngredientViewData& Ingredient : Recipe.Ingredients)
	{
		UKCHUDRecipeIngredientWidget* IngredientWidget = CreateWidget<UKCHUDRecipeIngredientWidget>(this, ResolvedEntryClass);
		if (!IngredientWidget)
		{
			continue;
		}

		IngredientWidget->SetIngredient(Ingredient);
		IngredientEntryContainer->AddChildToVerticalBox(IngredientWidget);
	}
}

FText UKCHUDRecipeEntryWidget::BuildStarsText(int32 DifficultyStars)
{
	const int32 StarCount = FMath::Clamp(DifficultyStars, 1, 5);
	FString Result;
	for (int32 Index = 0; Index < StarCount; ++Index)
	{
		if (!Result.IsEmpty())
		{
			Result += TEXT(" ");
		}
		Result += TEXT("*");
	}

	return FText::FromString(Result);
}

TSubclassOf<UKCHUDRecipeIngredientWidget> UKCHUDRecipeEntryWidget::ResolveIngredientWidgetClass() const
{
	if (IngredientWidgetClass)
	{
		return IngredientWidgetClass;
	}

	return LoadClass<UKCHUDRecipeIngredientWidget>(
		nullptr,
		TEXT("/Game/KC/UI/HUD/Recipe/WBP_HUDRecipeIngredient.WBP_HUDRecipeIngredient_C"));
}
