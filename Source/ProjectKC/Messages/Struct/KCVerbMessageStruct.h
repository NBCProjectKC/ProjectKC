/**
 * @file KCVerbMessageStruct.h
 * @brief 게임 내 모든 행위(공격, 피격, 상호작용 등)를 표현하는 범용 게임플레이 메시지 구조체 정의
 */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "KCVerbMessageStruct.generated.h"

/**
 * @struct FKCVerbMessageStruct
 * @brief [시전자(Instigator)]가 [대상(Target)]에게 [동사(Verb)] 행위를 수행했음을 나타내는 표준 메시지 규격
 */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCVerbMessageStruct
{
	GENERATED_BODY()

	/** @brief 행위의 종류를 식별하는 태그 (예: Ability.Action.Damage, GameplayEvent.Interaction) */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages")
	FGameplayTag Verb;

	/** @brief 행위를 유발한 주체 (공격자, 상호작용 개시자 등) */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages")
	TObjectPtr<UObject> Instigator = nullptr;

	/** @brief 행위의 대상 (피격자, 상호작용 대상 액터 등) */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages")
	TObjectPtr<UObject> Target = nullptr;

	/** @brief 시전자가 보유하고 있던 태그 컨테이너 */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages")
	FGameplayTagContainer InstigatorTags;

	/** @brief 대상이 보유하고 있던 태그 컨테이너 */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages")
	FGameplayTagContainer TargetTags;

	/** @brief 이벤트 발생 상황의 부가 문맥 태그 컨테이너 (예: 치명타, 헤드샷 등) */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages")
	FGameplayTagContainer ContextTags;

	/** @brief 행위의 수치적 크기 (예: 데미지 량, 회복량, 점수 등) */
	UPROPERTY(BlueprintReadWrite, Category = "ProjectKC|Messages")
	double Magnitude = 1.0;

	/**
	 * @brief 디버깅을 위한 문자열 표현을 반환합니다.
	 * @return FString 디버그용 문자열
	 */
	FString ToString() const;
};
