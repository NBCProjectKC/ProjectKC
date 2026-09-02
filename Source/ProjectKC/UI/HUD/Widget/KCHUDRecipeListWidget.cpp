#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeListWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/ListView.h"
#include "Components/VerticalBox.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDRecipeViewModel.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeEntryWidget.h"

void UKCHUDRecipeListWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UKCHUDRecipeListWidget::SetRecipes(const TArray<FKCRecipeViewData>& Recipes)
{
	BP_OnRecipesSet(Recipes);

	if (CanReuseRecipeItems(Recipes))
	{
		for (int32 RecipeIndex = 0; RecipeIndex < Recipes.Num(); ++RecipeIndex)
		{
			PreviewRecipeViewModels[RecipeIndex]->SetRecipe(Recipes[RecipeIndex]);
		}

		SetRecipeViewModels(PreviewRecipeViewModels);
		return;
	}

	PreviewRecipeViewModels.Reset();
	PreviewRecipeViewModels.Reserve(Recipes.Num());

	for (const FKCRecipeViewData& Recipe : Recipes)
	{
		UKCHUDRecipeViewModel* RecipeViewModel = NewObject<UKCHUDRecipeViewModel>(this);
		if (!RecipeViewModel)
		{
			continue;
		}

		RecipeViewModel->SetRecipe(Recipe);
		PreviewRecipeViewModels.Add(RecipeViewModel);
	}

	SetRecipeViewModels(PreviewRecipeViewModels);
}

void UKCHUDRecipeListWidget::PlayListShake()
{
	if (ListShake)
	{
		PlayAnimation(ListShake, 0.0f, 2);
	}
}

void UKCHUDRecipeListWidget::SetRecipeViewModels(const TArray<TObjectPtr<UKCHUDRecipeViewModel>>& RecipeViewModels)
{
	TArray<UObject*> ListItems;
	ListItems.Reserve(RecipeViewModels.Num());

	for (UKCHUDRecipeViewModel* RecipeViewModel : RecipeViewModels)
	{
		if (RecipeViewModel)
		{
			ListItems.Add(RecipeViewModel);
		}
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

	for (UKCHUDRecipeViewModel* RecipeViewModel : RecipeViewModels)
	{
		UKCHUDRecipeEntryWidget* EntryWidget = CreateWidget<UKCHUDRecipeEntryWidget>(this, EntryWidgetClass);
		if (!EntryWidget || !RecipeViewModel)
		{
			continue;
		}

		EntryWidget->SetRecipeViewModel(RecipeViewModel);
		EntryContainer->AddChildToVerticalBox(EntryWidget);
	}
}

bool UKCHUDRecipeListWidget::CanReuseRecipeItems(const TArray<FKCRecipeViewData>& Recipes) const
{
	if (PreviewRecipeViewModels.Num() != Recipes.Num())
	{
		return false;
	}

	for (int32 RecipeIndex = 0; RecipeIndex < Recipes.Num(); ++RecipeIndex)
	{
		const UKCHUDRecipeViewModel* RecipeViewModel = PreviewRecipeViewModels[RecipeIndex];
		if (!RecipeViewModel || RecipeViewModel->GetRecipe().RecipeRowName != Recipes[RecipeIndex].RecipeRowName)
		{
			return false;
		}
	}

	return true;
}
