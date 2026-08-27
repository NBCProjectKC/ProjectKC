#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeIngredientWidget.h"

#include "Components/TextBlock.h"
#include "ProjectKC/UI/Common/Style/KCColorStyle.h"
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
	BP_OnIngredientSet(Ingredient);

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

void UKCHUDRecipeIngredientWidget::NativeApplyColorStyle(const UKCColorStyle* InColorStyle)
{
	if (!InColorStyle)
	{
		return;
	}

	if (IngredientText)
	{
		IngredientText->SetColorAndOpacity(FSlateColor(InColorStyle->RecipeText));
	}

	if (CheckText)
	{
		CheckText->SetColorAndOpacity(FSlateColor(InColorStyle->RecipeCheck));
	}
}
