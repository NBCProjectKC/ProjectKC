#pragma once

#include "CoreMinimal.h"
#include "KCActionExecutionContext.generated.h"

class AActor;
class UAbilitySystemComponent;
class UKCGA_Base;

/** Fragment가 결과를 누구에게 적용할지 정한다. */
UENUM(BlueprintType)
enum class EKCActionScope : uint8
{
	/** 이 Hook이 지목한 대상. 대상이 없는 시점에서는 소스와 같다. */
	Target,

	/** 행동을 수행한 소스 자신. 흡혈·반동·자기 버프에 쓴다. */
	Source
};

/** Action Hook의 Fragment들이 공유하는 한 번의 실행 문맥이다. */
struct PROJECTKC_API FKCActionExecutionContext
{
	UKCGA_Base* Ability = nullptr;
	UAbilitySystemComponent* SourceAbilitySystem = nullptr;
	UAbilitySystemComponent* TargetAbilitySystem = nullptr;
	AActor* SourceActor = nullptr;
	AActor* TargetActor = nullptr;
	FHitResult HitResult;
	bool bHasHitResult = false;

	bool IsAuthoritative() const;

	AActor* ResolveScopedActor(EKCActionScope Scope) const;
	UAbilitySystemComponent* ResolveScopedAbilitySystem(EKCActionScope Scope) const;
};
