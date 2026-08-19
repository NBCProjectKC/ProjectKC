/**
 * @file KCAbilitySimpleFailureMessage.h
 * @brief 게임플레이 어빌리티 활성화 실패 시 UI/피드백으로 전송되는 메시지 구조체 정의
 */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KCAbilitySimpleFailureMessage.generated.h"

class APlayerController;

/**
 * @struct FKCAbilitySimpleFailureMessage
 * @brief 어빌리티 발동 실패 원인(쿨다운 등)과 안내 문구를 전달하는 페이로드
 */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCAbilitySimpleFailureMessage
{
	GENERATED_BODY()

	/** @brief 실패 알림을 표시할 플레이어 컨트롤러 */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages|Ability")
	TObjectPtr<APlayerController> PlayerController = nullptr;

	/** @brief 실패 원인 관련 태그들 (예: Ability.Fail.Cooldown, Ability.Fail.Cost) */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages|Ability")
	FGameplayTagContainer FailureTags;

	/** @brief 유저에게 화면으로 표시할 텍스트 안내 문구 */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages|Ability")
	FText UserFacingReason;
};
