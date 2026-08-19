/**
 * @file KCGameplayTags.cpp
 * @brief KCGameplayTags의 Native 태그 정의 및 탐색 구현부
 */

#include "ProjectKC/Messages/KCGameplayTags.h"
#include "GameplayTagsManager.h"

namespace KCGameplayTags
{
	/* =========================================================================
	 *  Core Gameplay & Combat Messages
	 * ========================================================================= */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Verb, "Message.Verb", "범용 상호작용/액션 메시지를 위한 루트 채널입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Damage, "Message.Damage", "데미지 부여 및 피격 발생 시 브로드캐스트되는 메시지 채널입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Elimination, "Message.Elimination", "캐릭터/플레이어 사망 및 처치 시 브로드캐스트되는 메시지 채널입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Ability_SimpleFailure, "Message.Ability.SimpleFailure", "스킬/어빌리티 발동 실패 시 UI 알림을 위해 브로드캐스트되는 메시지 채널입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Notification, "Message.Notification", "UI 토스트 팝업, 피드, 시스템 알림용 메시지 채널입니다.");

	/* =========================================================================
	 *  Lobby & Network Messages
	 * ========================================================================= */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Lobby_CreateResult, "Message.Lobby.CreateResult", "세션 생성 완료 시 발행되는 메시지 채널입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Lobby_FindResult, "Message.Lobby.FindResult", "세션 검색 완료 시 발행되는 메시지 채널입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Lobby_JoinResult, "Message.Lobby.JoinResult", "세션 참가 완료 시 발행되는 메시지 채널입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Lobby_DestroyResult, "Message.Lobby.DestroyResult", "세션 파괴 완료 시 발행되는 메시지 채널입니다.");

	FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString)
	{
		const UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
		FGameplayTag Tag = Manager.RequestGameplayTag(FName(*TagString), false);

		if (!Tag.IsValid() && bMatchPartialString)
		{
			FGameplayTagContainer AllTags;
			Manager.RequestAllGameplayTags(AllTags, true);

			for (const FGameplayTag& TestTag : AllTags)
			{
				if (TestTag.ToString().Contains(TagString))
				{
					UE_LOG(LogTemp, Display, TEXT("Could not find exact match for tag [%s] but found partial match on tag [%s]."), *TagString, *TestTag.ToString());
					Tag = TestTag;
					break;
				}
			}
		}

		return Tag;
	}
}
