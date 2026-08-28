#include "KCGameState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Messages/KCGameplayTags.h"
#include "Messages/Struct/KCActiveRecipesChangedStruct.h"
#include "Messages/Struct/KCGamePhaseChangedStruct.h"
#include "Messages/Struct/KCPotIngredientsChangedStruct.h"
#include "Messages/Struct/KCScoreChangedStruct.h"
#include "Recipe/KCRecipeStruct.h"
#include "Engine/DataTable.h"

AKCGameState::AKCGameState()
{
}

void AKCGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKCGameState, CurrentPhase);
	DOREPLIFETIME(AKCGameState, TeamScores);
	DOREPLIFETIME(AKCGameState, PotIngredients);
	DOREPLIFETIME(AKCGameState, ActiveRecipeRowNames);
	DOREPLIFETIME(AKCGameState, bIsFarmingOpen);
}

void AKCGameState::InitializeTeamCount(int32 InTeamCount)
{
	if (!HasAuthority())
	{
		return;
	}

	TeamScores.Init(0, InTeamCount);
	PotIngredients.Init(FGameplayTagContainer(), InTeamCount);
}

void AKCGameState::SetGamePhase(EKCGamePhaseType NewPhase)
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentPhase == NewPhase)
	{
		return;
	}

	CurrentPhase = NewPhase;
	OnRep_CurrentPhase();
}

void AKCGameState::SetTeamScore(int32 TeamId, int32 NewScore)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!TeamScores.IsValidIndex(TeamId))
	{
		return;
	}

	TeamScores[TeamId] = NewScore;
	OnRep_TeamScores();
}

void AKCGameState::SetPotIngredients(int32 TeamId, const FGameplayTagContainer& NewIngredients)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!PotIngredients.IsValidIndex(TeamId))
	{
		return;
	}

	PotIngredients[TeamId] = NewIngredients;
	OnRep_PotIngredients();
}

void AKCGameState::SetActiveRecipes(const TArray<FName>& InRecipeRowNames)
{
	if (!HasAuthority())
	{
		return;
	}

	ActiveRecipeRowNames = InRecipeRowNames;
	OnRep_ActiveRecipes();
}

int32 AKCGameState::GetTeamScore(int32 TeamId) const
{
	return TeamScores.IsValidIndex(TeamId) ? TeamScores[TeamId] : 0;
}

FGameplayTagContainer AKCGameState::GetPotIngredients(int32 TeamId) const
{
	return PotIngredients.IsValidIndex(TeamId) ? PotIngredients[TeamId] : FGameplayTagContainer();
}

const FKCRecipeStruct* AKCGameState::FindRecipeByRowName(FName RowName) const
{
	if (!RecipeDataTable)
	{
		return nullptr;
	}
	return RecipeDataTable->FindRow<FKCRecipeStruct>(RowName, TEXT("FindRecipeByRowName"));
}

TArray<FName> AKCGameState::GetAllRecipeRowNames() const
{
	if (!RecipeDataTable)
	{
		return TArray<FName>();
	}
	return RecipeDataTable->GetRowNames();
}

void AKCGameState::OnRep_CurrentPhase()
{
	FKCGamePhaseChangedStruct Message;
	Message.NewPhase = CurrentPhase;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(KCGameplayTags::Message_Game_PhaseChanged, Message);
}

void AKCGameState::OnRep_TeamScores()
{
	for (int32 TeamId = 0; TeamId < TeamScores.Num(); ++TeamId)
	{
		FKCScoreChangedStruct Message;
		Message.TeamId = TeamId;
		Message.NewScore = TeamScores[TeamId];

		UGameplayMessageSubsystem::Get(this).BroadcastMessage(KCGameplayTags::Message_Game_ScoreChanged, Message);
	}
}

void AKCGameState::OnRep_PotIngredients()
{
	for (int32 TeamId = 0; TeamId < PotIngredients.Num(); ++TeamId)
	{
		FKCPotIngredientsChangedStruct Message;
		Message.TeamId = TeamId;
		Message.Ingredients = PotIngredients[TeamId];

		UGameplayMessageSubsystem::Get(this).BroadcastMessage(KCGameplayTags::Message_Game_PotIngredientsChanged, Message);
	}
}

void AKCGameState::OnRep_ActiveRecipes()
{
	FKCActiveRecipesChangedStruct Message;
	Message.RecipeRowNames = ActiveRecipeRowNames;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(KCGameplayTags::Message_Game_ActiveRecipesChanged, Message);
}

void AKCGameState::SetFarmingOpen(bool bOpen)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsFarmingOpen = bOpen;
}
