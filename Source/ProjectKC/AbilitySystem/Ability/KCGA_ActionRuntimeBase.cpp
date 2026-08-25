#include "ProjectKC/AbilitySystem/Ability/KCGA_ActionRuntimeBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCActionRuntime, Log, All);

UKCGA_ActionRuntimeBase::UKCGA_ActionRuntimeBase()
{
	AddSupportedActionHook(TAG_KC_ActionHook_OnStart);
	AddSupportedActionHook(TAG_KC_ActionHook_OnExecute);
	AddSupportedActionHook(TAG_KC_ActionHook_OnComplete);
}

void UKCGA_ActionRuntimeBase::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ActivationTarget = nullptr;
	ActivationHitResult = FHitResult();
	bHasActivationHitResult = false;
	bFinishingAction = false;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}

	const UKCAbilityDefinition* Definition = GetActiveDefinition();
	const UKCActionTargeting* Targeting =
		Definition ? Definition->ActionTargeting : nullptr;
	if (!Targeting)
	{
		UE_LOG(
			LogKCActionRuntime,
			Warning,
			TEXT("Definition에 ActionTargeting이 없어 대상을 정할 수 없습니다."));
		FinishAction(true, false);
		return;
	}

	if (TriggerEventData)
	{
		ActivationTarget = const_cast<AActor*>(TriggerEventData->Target.Get());
		if (const FHitResult* Hit = TriggerEventData->ContextHandle.GetHitResult())
		{
			ActivationHitResult = *Hit;
			bHasActivationHitResult = true;
		}
	}

	if (Targeting->RequiresActivationTarget() && !ActivationTarget.IsValid())
	{
		UE_LOG(
			LogKCActionRuntime,
			Warning,
			TEXT("'%s'는 활성화 이벤트의 Target이 필요합니다. ")
			TEXT("소스가 TryActivateWithTarget()으로 발동해야 합니다."),
			*Targeting->GetClass()->GetName());
		FinishAction(true, false);
		return;
	}

	// 명중 횟수와 무관하게 한 번의 활성화에 Cost/Cooldown을 한 번만 확정한다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishAction(true, false);
		return;
	}

	ExecuteSourceHook(TAG_KC_ActionHook_OnStart);
}

void UKCGA_ActionRuntimeBase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bFinishingAction = true;
	ActivationTarget = nullptr;
	ActivationHitResult = FHitResult();
	bHasActivationHitResult = false;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UKCGA_ActionRuntimeBase::ExecutePulse()
{
	AActor* SourceActor = GetAvatarActorFromActorInfo();
	const UKCAbilityDefinition* Definition = GetActiveDefinition();
	const UKCActionTargeting* Targeting =
		Definition ? Definition->ActionTargeting : nullptr;
	if (!SourceActor || !Targeting)
	{
		return;
	}

	FKCActionTargetingContext TargetingContext;
	TargetingContext.SourceActor = SourceActor;
	TargetingContext.SourceObject = GetCurrentSourceObject();
	TargetingContext.ActivationTarget = ActivationTarget.Get();
	TargetingContext.ActivationHitResult = ActivationHitResult;
	TargetingContext.bHasActivationHitResult = bHasActivationHitResult;

	TArray<FKCActionTarget> Targets;
	Targeting->GatherTargets(TargetingContext, Targets);
	for (const FKCActionTarget& Target : Targets)
	{
		if (!IsValid(Target.Actor))
		{
			continue;
		}

		UAbilitySystemComponent* TargetAbilitySystem =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target.Actor);
		ExecuteActionHook(
			TAG_KC_ActionHook_OnExecute,
			TargetAbilitySystem,
			Target.Actor,
			Target.bHasHitResult ? &Target.HitResult : nullptr);
	}
}

void UKCGA_ActionRuntimeBase::ExecuteSourceHook(FGameplayTag HookTag)
{
	const UKCAbilityDefinition* Definition = GetActiveDefinition();
	if (!Definition || !Definition->FindActionHook(HookTag))
	{
		return;
	}

	ExecuteActionHook(
		HookTag,
		GetAbilitySystemComponentFromActorInfo(),
		GetAvatarActorFromActorInfo());
}

void UKCGA_ActionRuntimeBase::FinishAction(
	bool bWasCancelled,
	bool bRunCompleteHook)
{
	if (bFinishingAction || !IsActive())
	{
		return;
	}

	bFinishingAction = true;
	if (!bWasCancelled && bRunCompleteHook)
	{
		ExecuteSourceHook(TAG_KC_ActionHook_OnComplete);
	}

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		bWasCancelled);
}

bool UKCGA_ActionRuntimeBase::IsFinishingAction() const
{
	return bFinishingAction;
}
