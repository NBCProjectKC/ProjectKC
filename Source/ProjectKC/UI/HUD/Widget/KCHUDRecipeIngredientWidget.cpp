#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeIngredientWidget.h"

#include "Components/TextBlock.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeEntryWidget.h"

void UKCHUDRecipeIngredientWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	if (const UKCHUDRecipeIngredientListItem* IngredientItem = Cast<UKCHUDRecipeIngredientListItem>(ListItemObject))
	{
		SetIngredient(IngredientItem->Ingredient);
	}
}

void UKCHUDRecipeIngredientWidget::SetIngredient(const FKCRecipeIngredientViewData& Ingredient)
{
	if (IngredientText)
	{
		const FText IngredientDisplayName = Ingredient.DisplayName.IsEmpty()
			? FText::FromString(Ingredient.IngredientId.ToString())
			: Ingredient.DisplayName;
		IngredientText->SetText(IngredientDisplayName);
	}

	if (CheckText)
	{
		CheckText->SetText(Ingredient.bSubmitted ? FText::FromString(TEXT("v")) : FText::GetEmpty());
	}
}
