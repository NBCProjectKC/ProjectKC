#pragma once

#include "CoreMinimal.h"

class AActor;
class UAbilitySystemComponent;
class UKCGameplayAbility;

/** Action Hook의 Fragment들이 공유하는 한 번의 실행 문맥이다. */
struct PROJECTKC_API FKCActionExecutionContext
{
	UKCGameplayAbility* Ability = nullptr;
	UAbilitySystemComponent* SourceAbilitySystem = nullptr;
	UAbilitySystemComponent* TargetAbilitySystem = nullptr;
	AActor* SourceActor = nullptr;
	AActor* TargetActor = nullptr;
	FHitResult HitResult;
	bool bHasHitResult = false;

	bool IsAuthoritative() const;
};
