/**
 * @file KCNotificationMessage.h
 * @brief UI 알림/토스트 피드(킬 피드, 획득 알림 등)를 위한 메시지 구조체 정의
 */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "KCNotificationMessage.generated.h"

class UObject;
class APlayerState;

PROJECTKC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KC_AddNotification_Message);

/**
 * @struct FKCNotificationMessage
 * @brief UI 알림/로그 피드로 전달되는 메시지 페이로드
 */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCNotificationMessage
{
	GENERATED_BODY()

	/** @brief 알림이 전송될 대상 채널 태그 */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages|Notification")
	FGameplayTag TargetChannel;

	/** @brief 특정 대상 플레이어 (nullptr일 경우 모든 로컬 플레이어에게 표시) */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages|Notification")
	TObjectPtr<APlayerState> TargetPlayer = nullptr;

	/** @brief 화면에 출력할 텍스트 메시지 */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages|Notification")
	FText PayloadMessage;

	/** @brief 알림 채널별 부가 메타데이터 태그 (스타일, 아이콘 등 식별) */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages|Notification")
	FGameplayTag PayloadTag;

	/** @brief 알림 채널별 부가 데이터 애셋 또는 UObject */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages|Notification")
	TObjectPtr<UObject> PayloadObject = nullptr;
};
