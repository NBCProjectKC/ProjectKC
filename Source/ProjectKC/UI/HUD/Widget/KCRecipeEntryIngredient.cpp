#include "UI/HUD/Widget/KCRecipeEntryIngredient.h"

#include "Components/Image.h"
#include "ProjectKC/UI/Common/Style/KCColorStyle.h"

void UKCRecipeIngredientListItem::SetIngredient(const FKCRecipeIngredientViewData& NewIngredient)
{
	Ingredient = NewIngredient;
	OnIngredientChangedNative.Broadcast(Ingredient);
}

void UKCRecipeEntryIngredient::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	if (UKCRecipeIngredientListItem* IngredientItem = Cast<UKCRecipeIngredientListItem>(ListItemObject))
	{
		BindIngredientListItem(IngredientItem);
		SetIngredient(IngredientItem->Ingredient);
	}
}

void UKCRecipeEntryIngredient::NativeOnEntryReleased()
{
	UnbindIngredientListItem();
}

void UKCRecipeEntryIngredient::SetIngredient(const FKCRecipeIngredientViewData& Ingredient)
{
	CurrentIngredient = Ingredient;

	if (!IngredientImage)
	{
		RefreshSubmittedTeamImages();
		return;
	}

	UTexture2D* IngredientIcon = Ingredient.Icon.Get();
	if (!IngredientIcon)
	{
		IngredientImage->SetVisibility(ESlateVisibility::Hidden);
		RefreshSubmittedTeamImages();
		return;
	}

	IngredientImage->SetBrushFromTexture(IngredientIcon, false);
	IngredientImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshSubmittedTeamImages();
}

void UKCRecipeEntryIngredient::BindIngredientListItem(UKCRecipeIngredientListItem* IngredientItem)
{
	if (BoundIngredientItem.Get() == IngredientItem)
	{
		return;
	}

	UnbindIngredientListItem();
	BoundIngredientItem = IngredientItem;

	if (IngredientItem)
	{
		IngredientChangedHandle = IngredientItem->OnIngredientChangedNative.AddUObject(
			this,
			&ThisClass::HandleIngredientListItemChanged);
	}
}

void UKCRecipeEntryIngredient::UnbindIngredientListItem()
{
	if (UKCRecipeIngredientListItem* IngredientItem = BoundIngredientItem.Get())
	{
		IngredientItem->OnIngredientChangedNative.Remove(IngredientChangedHandle);
	}

	BoundIngredientItem.Reset();
	IngredientChangedHandle.Reset();
}

void UKCRecipeEntryIngredient::HandleIngredientListItemChanged(const FKCRecipeIngredientViewData& Ingredient)
{
	SetIngredient(Ingredient);
}

void UKCRecipeEntryIngredient::NativeApplyColorStyle(const UKCColorStyle* InColorStyle)
{
	Super::NativeApplyColorStyle(InColorStyle);

	RefreshSubmittedTeamImages();
}

void UKCRecipeEntryIngredient::RefreshSubmittedTeamImages()
{
	const UKCColorStyle* ResolvedColorStyle = GetColorStyle();

	auto ApplyTeamImage = [ResolvedColorStyle](UImage* TeamImage, int32 TeamId, bool bSubmitted)
	{
		if (!TeamImage)
		{
			return;
		}

		if (!bSubmitted)
		{
			TeamImage->SetVisibility(ESlateVisibility::Hidden);
			return;
		}

		if (ResolvedColorStyle && ResolvedColorStyle->TeamColors.IsValidIndex(TeamId))
		{
			TeamImage->SetColorAndOpacity(ResolvedColorStyle->TeamColors[TeamId]);
		}

		TeamImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	};

	ApplyTeamImage(Team1InnerImage.Get(), 0, CurrentIngredient.bSubmittedByTeam0);
	ApplyTeamImage(Team2InnerImage.Get(), 1, CurrentIngredient.bSubmittedByTeam1);
}
