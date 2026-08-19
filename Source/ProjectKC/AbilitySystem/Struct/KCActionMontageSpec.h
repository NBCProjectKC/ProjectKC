#pragma once

#include "CoreMinimal.h"
#include "KCActionMontageSpec.generated.h"

class UAnimMontage;

/**
 * 한 번의 사용 행동이 재생할 몽타주 설정이다.
 * 결과가 아니라 행동의 수명주기와 판정 시점을 결정하므로 Fragment가 아니다.
 */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCActionMontageSpec
{
	GENERATED_BODY()

	/** 즉시 재생 안정성을 우선해 첫 버전은 하드 레퍼런스를 사용한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Ability|Presentation")
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Ability|Presentation",
		meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "3.0"))
	float PlayRate = 1.0f;

	/** 비어 있으면 몽타주 처음부터 재생한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Ability|Presentation")
	FName StartSection = NAME_None;

	/** 버리기나 취소로 Ability가 끝날 때 남은 몽타주도 멈춘다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Ability|Presentation")
	bool bStopWhenAbilityEnds = true;

	bool HasMontage() const;

	/** 몽타주가 비어 있는 것 자체는 유효하다. 필수 여부는 상위 Definition이 판단한다. */
	bool Validate(FString& OutError) const;
};
