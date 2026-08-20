#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KCActionMontageConfig.generated.h"

class UAnimMontage;

/**
 * 한 번의 사용 행동이 재생할 몽타주 설정이다.
 * 결과가 아니라 행동의 수명주기와 판정 시점을 결정하므로 Fragment가 아니다.
 *
 * ActionConfig와 같은 인라인 Instanced 오브젝트다. 함정처럼 Avatar 애니메이션이
 * 없는 소스는 이 오브젝트를 아예 두지 않으므로 불필요한 값을 갖지 않는다.
 */
UCLASS(
	BlueprintType,
	EditInlineNew,
	DefaultToInstanced,
	meta = (DisplayName = "Action Montage"))
class PROJECTKC_API UKCActionMontageConfig : public UObject
{
	GENERATED_BODY()

public:
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

	/** 이 오브젝트가 존재한다는 것 자체가 몽타주 재생 의사이므로 Montage는 필수다. */
	virtual bool Validate(FString& OutError) const;
};
