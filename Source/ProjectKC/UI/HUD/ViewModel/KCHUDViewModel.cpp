#include "ProjectKC/UI/HUD/ViewModel/KCHUDViewModel.h"

#include "Blueprint/UserWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "ProjectKC/GameSystem/KCGameState.h"
#include "ProjectKC/GameSystem/Recipe/KCRecipeStruct.h"
#include "ProjectKC/GameSystem/Recipe/KCRecipeTierType.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Player/KCPlayerState.h"
#include "ProjectKC/Messages/KCGameplayTags.h"
#include "ProjectKC/Messages/Struct/KCActiveRecipesChangedStruct.h"
#include "ProjectKC/Messages/Struct/KCGamePhaseChangedStruct.h"
#include "ProjectKC/Messages/Struct/KCPotIngredientsChangedStruct.h"
#include "ProjectKC/Messages/Struct/KCPotProgressChangedStruct.h"
#include "ProjectKC/Messages/Struct/KCScoreChangedStruct.h"
#include "ProjectKC/Player/KCPlayerController.h"

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

	PotProgressChangedHandle = MessageSystem.RegisterListener<FKCPotProgressChangedStruct>(
		KCGameplayTags::Message_Game_PotProgressChanged,
		this,
		&ThisClass::HandlePotProgressChanged);

	SyncLocalTeamIdFromContext();
	SyncFromGameState();
	StartMatchTimerRefresh();
}

void UKCHUDViewModel::StopListening()
{
	StopMatchTimerRefresh();
	UnbindLocalTeamPlayerState();
	ScoreChangedHandle.Unregister();
	PhaseChangedHandle.Unregister();
	ActiveRecipesChangedHandle.Unregister();
	PotIngredientsChangedHandle.Unregister();
	PotProgressChangedHandle.Unregister();
	ListeningWorldContext.Reset();
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

void UKCHUDViewModel::SetRemainingMatchSeconds(int32 NewRemainingSeconds)
{
	NewRemainingSeconds = FMath::Max(0, NewRemainingSeconds);
	if (RemainingMatchSeconds == NewRemainingSeconds)
	{
		return;
	}

	RemainingMatchSeconds = NewRemainingSeconds;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(RemainingMatchSeconds);
	OnMatchTimerChangedNative.Broadcast(RemainingMatchSeconds);
}

void UKCHUDViewModel::SetLocalTeamId(int32 NewTeamId)
{
	if (NewTeamId < 0)
	{
		NewTeamId = 0;
	}

	if (LocalTeamId == NewTeamId)
	{
		return;
	}

	LocalTeamId = NewTeamId;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(LocalTeamId);
	RebuildSubmittedStates();
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
	FName RecipeRowName,
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
	PotProgress.RecipeRowName = RecipeRowName;
	PotProgress.bVisible = bVisible;
	PotProgress.bCompleted = bCompleted;

	PotProgresses = NewPotProgresses;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(PotProgresses);
	RebuildCookingStates();
	OnPotProgressChangedNative.Broadcast(TeamId, PotProgresses[TeamId]);
}

void UKCHUDViewModel::HandleLocalTeamIdChanged(int32 NewTeamId)
{
	SetLocalTeamId(NewTeamId);
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
	SyncLocalTeamIdFromContext();

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
	SyncLocalTeamIdFromContext();

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

void UKCHUDViewModel::HandlePotProgressChanged(
	FGameplayTag Channel,
	const FKCPotProgressChangedStruct& Message)
{
	SetPotProgress(
		Message.TeamId,
		Message.ProgressPercent,
		Message.RemainingSeconds,
		Message.RecipeRowName,
		Message.bVisible,
		Message.bCompleted);
}

void UKCHUDViewModel::SyncFromGameState()
{
	const UObject* WorldContextObject = ListeningWorldContext.Get();
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const AKCGameState* GameState = World ? World->GetGameState<AKCGameState>() : nullptr;
	if (!GameState)
	{
		return;
	}

	SetCurrentPhase(GameState->GetGamePhase());
	SetTeamScore(0, GameState->GetTeamScore(0));
	SetTeamScore(1, GameState->GetTeamScore(1));
	RefreshMatchTimer();

	TeamPotIngredients.SetNum(2);
	TeamPotIngredients[0] = GameState->GetPotIngredients(0);
	TeamPotIngredients[1] = GameState->GetPotIngredients(1);

	TArray<FKCRecipeViewData> NewRecipes;
	NewRecipes.Reserve(GameState->GetActiveRecipes().Num());
	for (const FName& RecipeRowName : GameState->GetActiveRecipes())
	{
		NewRecipes.Add(BuildRecipeViewData(RecipeRowName));
	}

	SetRecipes(NewRecipes);
	RebuildSubmittedStates();
}

void UKCHUDViewModel::RefreshMatchTimer()
{
	const UObject* WorldContextObject = ListeningWorldContext.Get();
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const AKCGameState* GameState = World ? World->GetGameState<AKCGameState>() : nullptr;
	const AKCPlayerController* PlayerController = nullptr;

	if (const UUserWidget* UserWidget = Cast<UUserWidget>(WorldContextObject))
	{
		PlayerController = Cast<AKCPlayerController>(UserWidget->GetOwningPlayer());
	}

	if (!PlayerController && World)
	{
		PlayerController = Cast<AKCPlayerController>(World->GetFirstPlayerController());
	}

	const int32 RemainingSeconds = GameState && PlayerController
		? GameState->GetRemainingMatchSeconds(PlayerController->GetServerTime())
		: 0;

	SetRemainingMatchSeconds(RemainingSeconds);
}

void UKCHUDViewModel::StartMatchTimerRefresh()
{
	if (const UObject* WorldContextObject = ListeningWorldContext.Get())
	{
		if (UWorld* World = WorldContextObject->GetWorld())
		{
			World->GetTimerManager().SetTimer(
				MatchTimerRefreshHandle,
				this,
				&ThisClass::RefreshMatchTimer,
				0.2f,
				true);
		}
	}
}

void UKCHUDViewModel::StopMatchTimerRefresh()
{
	if (const UObject* WorldContextObject = ListeningWorldContext.Get())
	{
		if (UWorld* World = WorldContextObject->GetWorld())
		{
			World->GetTimerManager().ClearTimer(MatchTimerRefreshHandle);
		}
	}
}

void UKCHUDViewModel::SyncLocalTeamIdFromContext()
{
	UObject* WorldContextObject = ListeningWorldContext.Get();
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	APlayerController* PlayerController = nullptr;

	if (UUserWidget* UserWidget = Cast<UUserWidget>(WorldContextObject))
	{
		PlayerController = UserWidget->GetOwningPlayer();
	}

	if (!PlayerController && World)
	{
		PlayerController = World->GetFirstPlayerController();
	}

	AKCPlayerState* PlayerState = PlayerController
		? PlayerController->GetPlayerState<AKCPlayerState>()
		: nullptr;

	BindLocalTeamPlayerState(PlayerState);
	SetLocalTeamId(PlayerState ? PlayerState->GetTeamId() : 0);
}

void UKCHUDViewModel::BindLocalTeamPlayerState(AKCPlayerState* PlayerState)
{
	if (BoundLocalTeamPlayerState.Get() == PlayerState)
	{
		return;
	}

	UnbindLocalTeamPlayerState();
	BoundLocalTeamPlayerState = PlayerState;

	if (PlayerState)
	{
		PlayerState->OnTeamIdChanged.AddUniqueDynamic(this, &ThisClass::HandleLocalTeamIdChanged);
	}
}

void UKCHUDViewModel::UnbindLocalTeamPlayerState()
{
	if (AKCPlayerState* PlayerState = BoundLocalTeamPlayerState.Get())
	{
		PlayerState->OnTeamIdChanged.RemoveDynamic(this, &ThisClass::HandleLocalTeamIdChanged);
	}

	BoundLocalTeamPlayerState.Reset();
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
			Ingredient.bSubmittedByTeam0 = false;
			Ingredient.bSubmittedByTeam1 = false;

			const bool bSubmittedByTeam0 = TeamPotIngredients.IsValidIndex(0) &&
				TeamPotIngredients[0].HasTagExact(Ingredient.IngredientId);
			const bool bSubmittedByTeam1 = TeamPotIngredients.IsValidIndex(1) &&
				TeamPotIngredients[1].HasTagExact(Ingredient.IngredientId);

			Ingredient.bSubmittedByTeam0 = bSubmittedByTeam0;
			Ingredient.bSubmittedByTeam1 = bSubmittedByTeam1;
			Ingredient.bSubmitted = TeamPotIngredients.IsValidIndex(LocalTeamId) &&
				TeamPotIngredients[LocalTeamId].HasTagExact(Ingredient.IngredientId);
			Ingredient.SubmittedTeamId = Ingredient.bSubmitted ? LocalTeamId : INDEX_NONE;

			if (bSubmittedByTeam0)
			{
				++Team0SubmittedCount;
			}

			if (bSubmittedByTeam1)
			{
				++Team1SubmittedCount;
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

void UKCHUDViewModel::RebuildCookingStates()
{
	TArray<FKCRecipeViewData> NewRecipes = Recipes;

	for (FKCRecipeViewData& Recipe : NewRecipes)
	{
		Recipe.bTeam0Cooking = false;
		Recipe.bTeam1Cooking = false;

		if (PotProgresses.IsValidIndex(0))
		{
			const FKCPotProgressViewData& Team0PotProgress = PotProgresses[0];
			Recipe.bTeam0Cooking =
				Team0PotProgress.bVisible &&
				!Team0PotProgress.bCompleted &&
				Team0PotProgress.RecipeRowName == Recipe.RecipeRowName;
		}

		if (PotProgresses.IsValidIndex(1))
		{
			const FKCPotProgressViewData& Team1PotProgress = PotProgresses[1];
			Recipe.bTeam1Cooking =
				Team1PotProgress.bVisible &&
				!Team1PotProgress.bCompleted &&
				Team1PotProgress.RecipeRowName == Recipe.RecipeRowName;
		}
	}

	SetRecipes(NewRecipes);
}

FKCRecipeViewData UKCHUDViewModel::BuildRecipeViewData(FName RecipeRowName) const
{
	FKCRecipeViewData RecipeViewData;
	RecipeViewData.RecipeRowName = RecipeRowName;
	RecipeViewData.DisplayName = FText::FromName(RecipeRowName);

	const UObject* WorldContextObject = ListeningWorldContext.Get();
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const AKCGameState* GameState = World ? World->GetGameState<AKCGameState>() : nullptr;
	const FKCRecipeStruct* Recipe = GameState ? GameState->FindRecipeByRowName(RecipeRowName) : nullptr;

	if (!Recipe)
	{
		UE_LOG(LogTemp, Warning, TEXT("KCHUDViewModel: GameState recipe row '%s' was not found."), *RecipeRowName.ToString());
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
		IngredientViewData.Icon = FindIngredientIcon(IngredientTag);
		RecipeViewData.Ingredients.Add(IngredientViewData);
	}

	return RecipeViewData;
}

TSoftObjectPtr<UTexture2D> UKCHUDViewModel::FindIngredientIcon(const FGameplayTag& IngredientTag) const
{
	if (!IngredientTag.IsValid())
	{
		return nullptr;
	}

	if (const TSoftObjectPtr<UTexture2D>* CachedIcon = IngredientIconCache.Find(IngredientTag))
	{
		return *CachedIcon;
	}

	TSoftObjectPtr<UTexture2D> FoundIcon;
	UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FPrimaryAssetId> ItemAssetIds;
	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("Item")), ItemAssetIds);

	for (const FPrimaryAssetId& ItemAssetId : ItemAssetIds)
	{
		const FSoftObjectPath ItemPath = AssetManager.GetPrimaryAssetPath(ItemAssetId);
		const UKCItemDefinition* ItemDefinition = Cast<UKCItemDefinition>(ItemPath.TryLoad());
		if (!ItemDefinition || ItemDefinition->ItemId != IngredientTag)
		{
			continue;
		}

		FoundIcon = ItemDefinition->Icon;
		break;
	}

	IngredientIconCache.Add(IngredientTag, FoundIcon);
	return FoundIcon;
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
