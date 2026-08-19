#pragma once

#include "NativeGameplayTags.h"

/**
 * 태그 :
 * - Event.Game.PhaseChanged     : Waiting/Playing/Ended 전환 시
 * - Event.Game.ScoreChanged     : 팀 점수 변경 시
 * - Event.Ingredient.Submitted  : 재료 투입 시
 *
 * 새로운 이벤트 채널이 필요하면 여기에 추가하고 공유해주세요.
 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Game_PhaseChanged);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Game_ScoreChanged);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Ingredient_Submitted);
