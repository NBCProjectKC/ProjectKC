#include "ProjectKC/AbilitySystem/Ability/KCGA_Action.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"
#include "ProjectKC/AbilitySystem/Timing/KCActionTiming.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCAction, Log, All);

UKCGA_Action::UKCGA_Action()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	AddSupportedActionHook(TAG_KC_ActionHook_OnStart);
	AddSupportedActionHook(TAG_KC_ActionHook_OnExecute);
	AddSupportedActionHook(TAG_KC_ActionHook_OnComplete);
}

void UKCGA_Action::ActivateAbility(
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

	// InstancedPerActor 인스턴스는 재사용되므로 실행 상태를 매번 초기화한다.
	ActiveTiming = nullptr;
	ActivationTarget = nullptr;
	ActivationHitResult = FHitResult();
	bHasActivationHitResult = false;
	bExecuteAttempted = false;
	bFinishingAction = false;

	const UKCAbilityDefinition* Definition = GetActiveDefinition();
	const UKCActionTargeting* Targeting =
		Definition ? Definition->ActionTargeting : nullptr;
	if (!Targeting)
	{
		UE_LOG(
			LogKCAction,
			Warning,
			TEXT("Definition에 ActionTargeting이 없어 대상을 정할 수 없습니다."));
		FinishAction(true);
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

	// 활성화 계약: Event 방식은 소스가 대상을 넘겨줘야 한다.
	if (Targeting->RequiresActivationTarget() && !ActivationTarget.IsValid())
	{
		UE_LOG(
			LogKCAction,
			Warning,
			TEXT("'%s'는 활성화 이벤트의 Target이 필요합니다. ")
			TEXT("소스가 TryActivateWithTarget()으로 발동해야 합니다."),
			*Targeting->GetClass()->GetName());
		FinishAction(true);
		return;
	}

	// 명중 여부가 아니라 시도 자체에 Cost/Cooldown을 소모한다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishAction(true);
		return;
	}

	ExecuteSourceHook(TAG_KC_ActionHook_OnStart);

	const UKCActionTiming* Timing = Definition->ActionTiming;
	if (!Timing)
	{
		RunExecuteOnce();
		FinishAction(false);
		return;
	}

	ActiveTiming = Timing;
	if (!Timing->ScheduleExecution(*this))
	{
		FinishAction(true);
	}
}

void UKCGA_Action::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bFinishingAction = true;

	if (bWasCancelled)
	{
		// 취소된 행동만 남은 연출을 끊는다. OnComplete는 실행된 행동에만 준다.
		if (const UKCActionTiming* Timing = ActiveTiming.Get())
		{
			Timing->CancelExecution(*this);
		}
	}
	else if (bExecuteAttempted)
	{
		ExecuteSourceHook(TAG_KC_ActionHook_OnComplete);
	}
	ActiveTiming = nullptr;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UKCGA_Action::HandleTimingExecuteEvent(FGameplayEventData Payload)
{
	if (bFinishingAction || bExecuteAttempted)
	{
		return;
	}

	RunExecuteOnce();
}

void UKCGA_Action::HandleTimingCompleted()
{
	if (bFinishingAction)
	{
		return;
	}

	if (!bExecuteAttempted)
	{
		UE_LOG(
			LogKCAction,
			Warning,
			TEXT("타이밍이 끝날 때까지 '%s'가 도착하지 않아 결과를 만들지 않았습니다."),
			*TAG_KC_GameplayEvent_Action_Execute.GetTag().ToString());
		FinishAction(true);
		return;
	}

	FinishAction(false);
}

void UKCGA_Action::HandleTimingAborted()
{
	FinishAction(true);
}

void UKCGA_Action::RunExecuteOnce()
{
	// Execute 신호가 중복 도착해도 같은 활성화에서 결과는 한 번만 만든다.
	if (bExecuteAttempted)
	{
		return;
	}
	bExecuteAttempted = true;

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

	// 대상이 없어도 정상 실행이다. 빗나간 행동도 한 번의 행동이다.
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

void UKCGA_Action::ExecuteSourceHook(FGameplayTag HookTag)
{
	const UKCAbilityDefinition* Definition = GetActiveDefinition();
	if (!Definition || !Definition->FindActionHook(HookTag))
	{
		return;
	}

	// 대상이 없는 시점이므로 소스를 대상 자리에 넣는다.
	// Fragment가 Scope=Target을 골라도 소스에 적용된다.
	ExecuteActionHook(
		HookTag,
		GetAbilitySystemComponentFromActorInfo(),
		GetAvatarActorFromActorInfo());
}

void UKCGA_Action::FinishAction(bool bWasCancelled)
{
	if (bFinishingAction)
	{
		return;
	}

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		bWasCancelled);
}
