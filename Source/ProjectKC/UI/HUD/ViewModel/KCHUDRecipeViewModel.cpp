#include "ProjectKC/UI/HUD/ViewModel/KCHUDRecipeViewModel.h"

void UKCHUDRecipeViewModel::SetRecipe(const FKCRecipeViewData& NewRecipe)
{
	Recipe = NewRecipe;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Recipe);
	RefreshLocalState();
	OnRecipeChangedNative.Broadcast(Recipe);
}

void UKCHUDRecipeViewModel::SetLocalTeamId(int32 NewLocalTeamId)
{
	NewLocalTeamId = FMath::Max(0, NewLocalTeamId);
	if (LocalTeamId == NewLocalTeamId)
	{
		return;
	}

	LocalTeamId = NewLocalTeamId;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(LocalTeamId);
	RefreshLocalState();
	OnRecipeChangedNative.Broadcast(Recipe);
}

void UKCHUDRecipeViewModel::RefreshLocalState()
{
	const bool bNewDisabledForLocalTeam =
		(LocalTeamId == 0 && Recipe.bTeam0Cooking) ||
		(LocalTeamId == 1 && Recipe.bTeam1Cooking);

	if (bDisabledForLocalTeam == bNewDisabledForLocalTeam)
	{
		return;
	}

	bDisabledForLocalTeam = bNewDisabledForLocalTeam;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bDisabledForLocalTeam);
}
