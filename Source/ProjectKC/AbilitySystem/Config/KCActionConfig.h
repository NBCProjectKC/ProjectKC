#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KCActionConfig.generated.h"

/**
 * 사거리·판정 형태처럼 특정 Action GA에만 필요한 설정의 기반 클래스다.
 * 구체 GA는 자신이 지원하는 Config 클래스를 선언하고 타입을 검증한다.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class /**
 * Validates the action configuration.
 * @param OutError Receives an error message when validation fails.
 * @returns `true` if the configuration is valid, `false` otherwise.
 */
PROJECTKC_API UKCActionConfig : public UObject
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const;
};
