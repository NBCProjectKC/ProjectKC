#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"

#include "Engine/DataTable.h"
#include "ProjectKC/GameSystem/Recipe/KCRecipeStruct.h"
#include "ProjectKC/GameSystem/Recipe/KCRecipeTierType.h"
#include "ProjectKC/Messages/KCGameplayTags.h"
#include "ProjectKC/Messages/Struct/KCActiveRecipesChangedStruct.h"
#include "ProjectKC/Messages/Struct/KCGamePhaseChangedStruct.h"
#include "ProjectKC/Messages/Struct/KCPotIngredientsChangedStruct.h"
#include "ProjectKC/Messages/Struct/KCScoreChangedStruct.h"

void UKCHUDViewModel::StartListening(UObject* WorldContextObject)
{
	StopListening();

	if (!WorldContextObject)
	{
		return;
	}

	ListeningWorldContext = WorldContextObject;
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(WorldContextObject);

	ScoreChangedHandle = MessageSystem.RegisterListener<FKCScoreChangedStruct>(
		KCGameplayTags::Message_Game_ScoreChanged,
		this,
		&ThisClass::HandleScoreChanged);

	PhaseChangedHandle = MessageSystem.RegisterListener<FKCGamePhaseChangedStruct>(
		KCGameplayTags::Message_Game_PhaseChanged,
		this,
		&ThisClass::HandleGamePhaseChanged);

	ActiveRecipesChangedHandle = MessageSystem.RegisterListener<FKCActiveRecipesChangedStruct>(
		KCGameplayTags::Message_Game_ActiveRecipesChanged,
		this,
		&ThisClass::HandleActiveRecipesChanged);

	PotIngredientsChangedHandle = MessageSystem.RegisterListener<FKCPotIngredientsChangedStruct>(
		KCGameplayTags::Message_Game_PotIngredientsChanged,
		this,
		&ThisClass::HandlePotIngredientsChanged);
}

void UKCHUDViewModel::StopListening()
{
	ScoreChangedHandle.Unregister();
	PhaseChangedHandle.Unregister();
	ActiveRecipesChangedHandle.Unregister();
	PotIngredientsChangedHandle.Unregister();
	ListeningWorldContext.Reset();
}

void UKCHUDViewModel::SetRecipeDataTable(UDataTable* InRecipeDataTable)
{
	RecipeDataTable = InRecipeDataTable;
}

void UKCHUDViewModel::SetCurrentPhase(EKCGamePhaseType NewPhase)
{
	UE_MVVM_SET_PROPERTY_VALUE(CurrentPhase, NewPhase);
}

int32 UKCHUDViewModel::GetTeamScore(int32 TeamId) const
{
	return TeamScores.IsValidIndex(TeamId) ? TeamScores[TeamId] : 0;
}

void UKCHUDViewModel::SetTeamScore(int32 TeamId, int32 NewScore)
{
	if (TeamId < 0)
	{
		return;
	}

	TArray<int32> NewTeamScores = TeamScores;
	if (TeamId >= NewTeamScores.Num())
	{
		NewTeamScores.SetNum(TeamId + 1);
	}

	NewTeamScores[TeamId] = NewScore;
	SetTeamScores(NewTeamScores);
}

void UKCHUDViewModel::SetTeamScores(const TArray<int32>& NewTeamScores)
{
	if (TeamScores == NewTeamScores)
	{
		return;
	}

	TeamScores = NewTeamScores;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TeamScores);
	OnTeamScoresChangedNative.Broadcast(TeamScores);
}

void UKCHUDViewModel::SetRecipes(const TArray<FKCRecipeViewData>& NewRecipes)
{
	Recipes = NewRecipes;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Recipes);
	OnRecipesChangedNative.Broadcast();
}

void UKCHUDViewModel::SetPreviewData(const TArray<int32>& NewTeamScores, const TArray<FKCRecipeViewData>& NewRecipes)
{
	SetTeamScores(NewTeamScores);
	SetRecipes(NewRecipes);
}

FKCPotProgressViewData UKCHUDViewModel::GetPotProgress(int32 TeamId) const
{
	return PotProgresses.IsValidIndex(TeamId) ? PotProgresses[TeamId] : FKCPotProgressViewData();
}

void UKCHUDViewModel::SetPotProgress(
	int32 TeamId,
	float ProgressPercent,
	int32 RemainingSeconds,
	bool bVisible,
	bool bCompleted)
{
	if (TeamId < 0)
	{
		return;
	}

	TArray<FKCPotProgressViewData> NewPotProgresses = PotProgresses;
	if (TeamId >= NewPotProgresses.Num())
	{
		NewPotProgresses.SetNum(TeamId + 1);
	}

	FKCPotProgressViewData& PotProgress = NewPotProgresses[TeamId];
	PotProgress.ProgressPercent = FMath::Clamp(ProgressPercent, 0.0f, 1.0f);
	PotProgress.RemainingSeconds = FMath::Max(0, RemainingSeconds);
	PotProgress.bVisible = bVisible;
	PotProgress.bCompleted = bCompleted;

	PotProgresses = NewPotProgresses;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(PotProgresses);
	OnPotProgressChangedNative.Broadcast(TeamId, PotProgresses[TeamId]);
}

void UKCHUDViewModel::HandleScoreChanged(FGameplayTag Channel, const FKCScoreChangedStruct& Message)
{
	TArray<int32> NewTeamScores = TeamScores;
	if (Message.TeamId >= NewTeamScores.Num())
	{
		NewTeamScores.SetNum(Message.TeamId + 1);
	}

	NewTeamScores[Message.TeamId] = Message.NewScore;
	SetTeamScores(NewTeamScores);
	OnScoreChanged(Message.TeamId, Message.NewScore);
}

void UKCHUDViewModel::HandleGamePhaseChanged(FGameplayTag Channel, const FKCGamePhaseChangedStruct& Message)
{
	SetCurrentPhase(Message.NewPhase);
	OnGamePhaseChanged(CurrentPhase);
}

void UKCHUDViewModel::HandleActiveRecipesChanged(FGameplayTag Channel, const FKCActiveRecipesChangedStruct& Message)
{
	TArray<FKCRecipeViewData> NewRecipes;
	NewRecipes.Reserve(Message.RecipeRowNames.Num());

	for (const FName& RecipeRowName : Message.RecipeRowNames)
	{
		NewRecipes.Add(BuildRecipeViewData(RecipeRowName));
	}

	SetRecipes(NewRecipes);
	RebuildSubmittedStates();
	OnRecipesChanged();
}

void UKCHUDViewModel::HandlePotIngredientsChanged(FGameplayTag Channel, const FKCPotIngredientsChangedStruct& Message)
{
	if (Message.TeamId < 0)
	{
		return;
	}

	if (Message.TeamId >= TeamPotIngredients.Num())
	{
		TeamPotIngredients.SetNum(Message.TeamId + 1);
	}

	TeamPotIngredients[Message.TeamId] = Message.Ingredients;
	RebuildSubmittedStates();
	OnPotIngredientsChanged(Message.TeamId);
}

void UKCHUDViewModel::RebuildSubmittedStates()
{
	TArray<FKCRecipeViewData> NewRecipes = Recipes;

	for (FKCRecipeViewData& Recipe : NewRecipes)
	{
		int32 Team0SubmittedCount = 0;
		int32 Team1SubmittedCount = 0;

		for (FKCRecipeIngredientViewData& Ingredient : Recipe.Ingredients)
		{
			Ingredient.bSubmitted = false;
			Ingredient.SubmittedTeamId = INDEX_NONE;

			const bool bSubmittedByTeam0 = TeamPotIngredients.IsValidIndex(0) &&
				TeamPotIngredients[0].HasTagExact(Ingredient.IngredientId);
			const bool bSubmittedByTeam1 = TeamPotIngredients.IsValidIndex(1) &&
				TeamPotIngredients[1].HasTagExact(Ingredient.IngredientId);

			if (bSubmittedByTeam0)
			{
				++Team0SubmittedCount;
			}

			if (bSubmittedByTeam1)
			{
				++Team1SubmittedCount;
			}

			for (int32 TeamId = 0; TeamId < TeamPotIngredients.Num(); ++TeamId)
			{
				if (TeamPotIngredients[TeamId].HasTagExact(Ingredient.IngredientId))
				{
					Ingredient.bSubmitted = true;
					Ingredient.SubmittedTeamId = TeamId;
					break;
				}
			}
		}

		const float RequiredIngredientCount = static_cast<float>(Recipe.Ingredients.Num());
		if (RequiredIngredientCount <= 0.0f)
		{
			Recipe.Team0Progress = 0.0f;
			Recipe.Team1Progress = 0.0f;
			Recipe.bTeam0ProgressVisible = false;
			Recipe.bTeam1ProgressVisible = false;
			continue;
		}

		Recipe.Team0Progress = Team0SubmittedCount / RequiredIngredientCount;
		Recipe.Team1Progress = Team1SubmittedCount / RequiredIngredientCount;
		Recipe.bTeam0ProgressVisible = Team0SubmittedCount > 0;
		Recipe.bTeam1ProgressVisible = Team1SubmittedCount > 0;
	}

	SetRecipes(NewRecipes);
}

FKCRecipeViewData UKCHUDViewModel::BuildRecipeViewData(FName RecipeRowName) const
{
	FKCRecipeViewData RecipeViewData;
	RecipeViewData.RecipeRowName = RecipeRowName;
	RecipeViewData.DisplayName = FText::FromName(RecipeRowName);

	const FKCRecipeStruct* Recipe = RecipeDataTable
		? RecipeDataTable->FindRow<FKCRecipeStruct>(RecipeRowName, TEXT("KCHUDViewModel"))
		: nullptr;

	if (!Recipe)
	{
		UE_LOG(LogTemp, Warning, TEXT("KCHUDViewModel: RecipeDataTable is missing or row '%s' was not found. This UI lookup is temporary until recipe ownership is unified."), *RecipeRowName.ToString());
		return RecipeViewData;
	}

	RecipeViewData.DisplayName = Recipe->RecipeName.IsEmpty() ? FText::FromName(RecipeRowName) : Recipe->RecipeName;
	switch (Recipe->Tier)
	{
	case EKCRecipeTierType::Low:
		RecipeViewData.DifficultyStars = 1;
		break;
	case EKCRecipeTierType::Medium:
		RecipeViewData.DifficultyStars = 2;
		break;
	case EKCRecipeTierType::High:
		RecipeViewData.DifficultyStars = 3;
		break;
	default:
		RecipeViewData.DifficultyStars = 0;
		break;
	}

	RecipeViewData.Ingredients.Reserve(Recipe->RequiredIngredients.Num());
	for (const FGameplayTag& IngredientTag : Recipe->RequiredIngredients)
	{
		FKCRecipeIngredientViewData IngredientViewData;
		IngredientViewData.IngredientId = IngredientTag;
		IngredientViewData.DisplayName = MakeDisplayNameFromTag(IngredientTag);
		RecipeViewData.Ingredients.Add(IngredientViewData);
	}

	return RecipeViewData;
}

FText UKCHUDViewModel::MakeDisplayNameFromTag(const FGameplayTag& Tag)
{
	FString TagString = Tag.ToString();
	FString Left;
	FString Right;
	if (TagString.Split(TEXT("."), &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		TagString = Right;
	}

	return FText::FromString(TagString);
}
