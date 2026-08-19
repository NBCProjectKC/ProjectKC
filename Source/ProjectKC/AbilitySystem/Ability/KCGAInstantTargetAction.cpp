#include "ProjectKC/AbilitySystem/Ability/KCGAInstantTargetAction.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

UKCGAInstantTargetAction::UKCGAInstantTargetAction()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	AddRequiredActionHook(TAG_KC_ActionHook_Target_OnTrigger);
}

bool UKCGAInstantTargetAction::PrepareUseAction(
	const FGameplayEventData* TriggerEventData)
{
	PreparedTargetActor.Reset();
	PreparedHitResult = FHitResult();
	bHasPreparedHitResult = false;

	AActor* TargetActor = TriggerEventData
		? const_cast<AActor*>(TriggerEventData->Target.Get())
		: nullptr;
	if (!IsValid(TargetActor))
	{
		return false;
	}

	if (const FHitResult* HitResult =
		TriggerEventData->ContextHandle.GetHitResult())
	{
		PreparedHitResult = *HitResult;
		bHasPreparedHitResult = true;
	}

	PreparedTargetActor = TargetActor;
	return true;
}

bool UKCGAInstantTargetAction::ExecuteUseAction()
{
	// 대기 중 대상이 사라졌다면 결과를 실행하지 않고 취소한다.
	AActor* TargetActor = PreparedTargetActor.Get();
	if (!IsValid(TargetActor))
	{
		return false;
	}

	UAbilitySystemComponent* TargetAbilitySystem =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	return ExecuteActionHook(
		TAG_KC_ActionHook_Target_OnTrigger,
		TargetAbilitySystem,
		TargetActor,
		bHasPreparedHitResult ? &PreparedHitResult : nullptr);
}
