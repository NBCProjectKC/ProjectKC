#pragma once

#include "CoreMinimal.h"

class AActor;
class UAbilitySystemComponent;
class UKCGameplayAbility;

/**
 * Determines whether the execution context is authoritative.
 *
 * @returns `true` if the context is authoritative, `false` otherwise.
 */
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
