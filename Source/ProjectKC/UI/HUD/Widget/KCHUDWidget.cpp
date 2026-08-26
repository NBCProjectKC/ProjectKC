#include "ProjectKC/UI/HUD/Widget/KCHUDWidget.h"

#include "CommonTextBlock.h"
#include "Components/TextBlock.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"
#include "ProjectKC/UI/HUD/Widget/KCHUDRecipeListWidget.h"

void UKCHUDWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	ApplyHUDTextStyles();
}

void UKCHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!HUDViewModel)
	{
		HUDViewModel = NewObject<UKCHUDViewModel>(this);
	}

	HUDViewModel->StartListening(this);
	HUDViewModel->OnTeamScoresChangedNative.AddUObject(this, &ThisClass::HandleTeamScoresChanged);
	HUDViewModel->OnRecipesChangedNative.AddUObject(this, &ThisClass::HandleRecipesChanged);
	RefreshRoughHUD();
}

void UKCHUDWidget::NativeDestruct()
{
	if (HUDViewModel)
	{
		HUDViewModel->OnTeamScoresChangedNative.RemoveAll(this);
		HUDViewModel->OnRecipesChangedNative.RemoveAll(this);
		HUDViewModel->StopListening();
	}

	Super::NativeDestruct();
}

void UKCHUDWidget::HandleTeamScoresChanged(const TArray<int32>& TeamScores)
{
	RefreshRoughScore();
}

void UKCHUDWidget::HandleRecipesChanged()
{
	RefreshRoughRecipes();
}

void UKCHUDWidget::RefreshRoughHUD()
{
	RefreshRoughScore();
	RefreshRoughRecipes();
}

void UKCHUDWidget::RefreshRoughScore()
{
	if (!HUDViewModel)
	{
		return;
	}

	const int32 LeftScore = HUDViewModel->GetTeamScore(0);
	const int32 RightScore = HUDViewModel->GetTeamScore(1);

	if (HUDScoreText)
	{
		HUDScoreText->SetText(FText::FromString(FString::Printf(TEXT("%d vs %d"), LeftScore, RightScore)));
	}
}

void UKCHUDWidget::RefreshRoughRecipes()
{
	if (!HUDViewModel)
	{
		return;
	}

	const TArray<FKCRecipeViewData>& Recipes = HUDViewModel->GetRecipes();
	if (RecipeListWidget)
	{
		RecipeListWidget->SetRecipes(Recipes);
	}

	UTextBlock* StarTexts[] = { RecipeStars_0, RecipeStars_1, RecipeStars_2 };
	UTextBlock* FoodLabels[] = { FoodLabel_0, FoodLabel_1, FoodLabel_2 };
	UTextBlock* IngredientTexts[] = { IngredientSlots_0, IngredientSlots_1, IngredientSlots_2 };
	UTextBlock* CheckTexts[] = { IngredientChecks_0, IngredientChecks_1, IngredientChecks_2 };

	for (int32 Index = 0; Index < 3; ++Index)
	{
		const bool bHasRecipe = Recipes.IsValidIndex(Index);
		const FKCRecipeViewData* Recipe = bHasRecipe ? &Recipes[Index] : nullptr;

		if (StarTexts[Index])
		{
			StarTexts[Index]->SetText(Recipe ? BuildStarsText(Recipe->DifficultyStars) : FText::FromString(TEXT("* *")));
		}

		if (FoodLabels[Index])
		{
			FoodLabels[Index]->SetText(Recipe ? Recipe->DisplayName : FText::FromString(TEXT("FOOD")));
		}

		if (IngredientTexts[Index])
		{
			IngredientTexts[Index]->SetText(Recipe ? BuildIngredientsText(*Recipe) : FText::FromString(TEXT("[ ]  [ ]  [ ]  [ ]")));
		}

		if (CheckTexts[Index])
		{
			CheckTexts[Index]->SetText(Recipe ? BuildChecksText(*Recipe) : FText::GetEmpty());
		}
	}
}

FText UKCHUDWidget::BuildStarsText(int32 DifficultyStars)
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

FText UKCHUDWidget::BuildIngredientsText(const FKCRecipeViewData& Recipe)
{
	if (Recipe.Ingredients.Num() == 0)
	{
		return FText::FromString(TEXT("[ ]  [ ]  [ ]  [ ]"));
	}

	FString Result;
	for (const FKCRecipeIngredientViewData& Ingredient : Recipe.Ingredients)
	{
		const FString Name = Ingredient.DisplayName.IsEmpty()
			? Ingredient.IngredientId.ToString()
			: Ingredient.DisplayName.ToString();
		Result += FString::Printf(TEXT("[%s] "), *Name.Left(3));
	}

	return FText::FromString(Result.TrimEnd());
}

FText UKCHUDWidget::BuildChecksText(const FKCRecipeViewData& Recipe)
{
	FString Result;
	for (const FKCRecipeIngredientViewData& Ingredient : Recipe.Ingredients)
	{
		Result += Ingredient.bSubmitted ? TEXT("v    ") : TEXT("     ");
	}

	return FText::FromString(Result.TrimEnd());
}

void UKCHUDWidget::ApplyHUDTextStyles()
{
	ApplyTextStyle(HUDScoreText, ScoreTextStyle);

	UTextBlock* TitleTexts[] = { RecipeStars_0, RecipeStars_1, RecipeStars_2, FoodLabel_0, FoodLabel_1, FoodLabel_2 };
	for (UTextBlock* TextBlock : TitleTexts)
	{
		ApplyTextStyle(TextBlock, RecipeTitleTextStyle);
	}

	UTextBlock* IngredientTexts[] = { IngredientSlots_0, IngredientSlots_1, IngredientSlots_2 };
	for (UTextBlock* TextBlock : IngredientTexts)
	{
		ApplyTextStyle(TextBlock, RecipeIngredientTextStyle);
	}

	UTextBlock* CheckTexts[] = { IngredientChecks_0, IngredientChecks_1, IngredientChecks_2 };
	for (UTextBlock* TextBlock : CheckTexts)
	{
		ApplyTextStyle(TextBlock, RecipeCheckTextStyle);
	}
}

void UKCHUDWidget::ApplyTextStyle(UTextBlock* TextBlock, TSubclassOf<UCommonTextStyle> TextStyleClass)
{
	if (!TextBlock || !TextStyleClass)
	{
		return;
	}

	const UCommonTextStyle* TextStyle = TextStyleClass.GetDefaultObject();
	if (!TextStyle)
	{
		return;
	}

	FSlateFontInfo FontInfo;
	TextStyle->GetFont(FontInfo);
	TextBlock->SetFont(FontInfo);

	FLinearColor TextColor;
	TextStyle->GetColor(TextColor);
	TextBlock->SetColorAndOpacity(FSlateColor(TextColor));

	FVector2D ShadowOffset;
	TextStyle->GetShadowOffset(ShadowOffset);
	TextBlock->SetShadowOffset(ShadowOffset);

	FLinearColor ShadowColor;
	TextStyle->GetShadowColor(ShadowColor);
	TextBlock->SetShadowColorAndOpacity(TextStyle->bUsesDropShadow ? ShadowColor : FLinearColor::Transparent);

	TextBlock->SetLineHeightPercentage(TextStyle->GetLineHeightPercentage());
	TextBlock->SetApplyLineHeightToBottomLine(TextStyle->GetApplyLineHeightToBottomLine());
}
