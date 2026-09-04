/**
 * @file KCGameplayTags.h
 * @brief 프로젝트 전체 Native Gameplay Tags 통합 관리 네임스페이스 
 */

#pragma once

#include "NativeGameplayTags.h"

namespace KCGameplayTags
{
	PROJECTKC_API FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString = false);

	/* =========================================================================
	 *  Core Gameplay & Combat Messages
	 * ========================================================================= */
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Verb);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Damage);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Elimination);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Ability_SimpleFailure);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Notification);

	/* =========================================================================
	 *  Lobby & Network Messages
	 * ========================================================================= */
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Lobby_CreateResult);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Lobby_FindResult);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Lobby_JoinResult);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Lobby_DestroyResult);
	
	/* =========================================================================
	 *  GameSystem Messages
	 * ========================================================================= */
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Game_PhaseChanged);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Game_ScoreChanged);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Game_ActiveRecipesChanged);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Game_PotIngredientsChanged);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Game_PotProgressChanged);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Ingredient_Submitted);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Level_Changed);
	
	/* =========================================================================
	 *  Recipe Completed or Ruind Messages
	 * ========================================================================= */
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Recipe_Completed);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Dish_Finished);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Dish_Ruined);

	/* =========================================================================
	 *  Interaction Prompt Tags
	 * ========================================================================= */
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Item_PickUp);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Item_Equip);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Pot_SubmitIngredient);
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Pot_Cook);
	
	/* =========================================================================
	 *  Loading Screen Tag
	 * ========================================================================= */
	PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_LoadingScreen_Hidden);
}
