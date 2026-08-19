#include "ProjectKC/AbilitySystem/Ability/KCGAInstantTargetAction.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Tag/KCGameplayTags.h"

UKCGAInstantTargetAction::UKCGAInstantTargetAction()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	AddRequiredActionHook(TAG_KC_ActionHook_Target_OnTrigger);
}

void UKCGAInstantTargetAction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}

	AActor* TargetActor = TriggerEventData
		? const_cast<AActor*>(TriggerEventData->Target.Get())
		: nullptr;
	UAbilitySystemComponent* TargetAbilitySystem =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	const FHitResult* HitResult = TriggerEventData
		? TriggerEventData->ContextHandle.GetHitResult()
		: nullptr;

	const bool bCommitted = IsValid(TargetActor) &&
		CommitAbility(Handle, ActorInfo, ActivationInfo);
	const bool bExecuted = bCommitted && ExecuteActionHook(
		TAG_KC_ActionHook_Target_OnTrigger,
		TargetAbilitySystem,
		TargetActor,
		HitResult);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, !bExecuted);
}
