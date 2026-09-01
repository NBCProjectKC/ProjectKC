#include "ProjectKC/AbilitySystem/Ability/KCGA_ActionRuntimeBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySourceComponent.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"
#include "ProjectKC/AbilitySystem/Task/KCAbilityTask_ActionTraceWindow.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCActionRuntime, Log, All);

UKCGA_ActionRuntimeBase::UKCGA_ActionRuntimeBase()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_KC_Ability_Attack);
	SetAssetTags(AssetTags);

	AddSupportedActionHook(TAG_KC_ActionHook_OnStart);
	AddSupportedActionHook(TAG_KC_ActionHook_OnExecute);
	AddSupportedActionHook(TAG_KC_ActionHook_OnComplete);
}

bool UKCGA_ActionRuntimeBase::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(
		Handle,
		ActorInfo,
		SourceTags,
		TargetTags,
		OptionalRelevantTags))
	{
		return false;
	}

	const AKCWorldItemActor* SourceItem = ResolveSourceItem(Handle, ActorInfo);
	return !SourceItem || SourceItem->IsUsable();
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
	bDurabilityConsumedThisActivation = false;
	bUseConsumptionPendingThisActivation = false;
	StopActiveDurabilityDrain(false);
	ActiveSourceItem = nullptr;
	ActiveTraceTask = nullptr;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}
	ActiveSourceItem = ResolveSourceItem(Handle, ActorInfo);

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

	const UKCInstantActionTargeting* InstantTargeting =
		Cast<UKCInstantActionTargeting>(Targeting);
	const UKCTraceWindowTargeting* TraceWindowTargeting =
		Cast<UKCTraceWindowTargeting>(Targeting);
	if (!InstantTargeting && !TraceWindowTargeting)
	{
		UE_LOG(
			LogKCActionRuntime,
			Warning,
			TEXT("Targeting '%s'가 Instant 또는 TraceWindow 계약을 구현하지 않습니다."),
			*Targeting->GetClass()->GetName());
		FinishAction(true, false);
		return;
	}

	if (TraceWindowTargeting)
	{
		UObject* TraceSource = nullptr;
		FString TraceSourceError;
		if (!TraceWindowTargeting->ResolveTraceSource(
			BuildTargetingContext(), TraceSource, &TraceSourceError) ||
			!IsValid(TraceSource))
		{
			if (TraceSourceError.IsEmpty())
			{
				TraceSourceError = TEXT("유효한 런타임 Trace Source를 반환하지 않았습니다.");
			}
			UE_LOG(
				LogKCActionRuntime,
				Warning,
				TEXT("TraceWindow Targeting을 시작할 수 없습니다: %s"),
				*TraceSourceError);
			FinishAction(true, false);
			return;
		}
	}

	// 명중 횟수와 무관하게 한 번의 활성화에 Cost/Cooldown을 한 번만 확정한다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishAction(true, false);
		return;
	}

	TryConsumeActiveItemDurability(EKCItemDurabilityConsumeMode::OnUse);
	StartActiveDurabilityDrain();

	if (TraceWindowTargeting)
	{
		ActiveTraceTask = UKCAbilityTask_ActionTraceWindow::Create(
			this, TraceWindowTargeting);
		if (!ActiveTraceTask)
		{
			FinishAction(true, false);
			return;
		}
		ActiveTraceTask->ReadyForActivation();
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
	StopActiveDurabilityDrain(true);
	AKCWorldItemActor* PendingConsumptionItem =
		bUseConsumptionPendingThisActivation
			? ActiveSourceItem.Get()
			: nullptr;
	bUseConsumptionPendingThisActivation = false;
	ActivationTarget = nullptr;
	ActivationHitResult = FHitResult();
	bHasActivationHitResult = false;
	// Ability 종료 중에는 GAS가 Task 배열을 순회해 직접 정리한다.
	ActiveTraceTask = nullptr;
	ActiveSourceItem = nullptr;

	// 실제 제거는 다음 틱이므로 현재 Ability 종료 스택과 몽타주 정리는
	// 끝까지 진행된다. Super 이후 SourceObject 상태에 의존하지 않게 먼저 예약한다.
	if (IsValid(PendingConsumptionItem))
	{
		PendingConsumptionItem->FinalizePendingUseConsumption();
	}

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UKCGA_ActionRuntimeBase::ExecutePulse()
{
	const UKCAbilityDefinition* Definition = GetActiveDefinition();
	const UKCInstantActionTargeting* Targeting = Cast<UKCInstantActionTargeting>(
		Definition ? Definition->ActionTargeting : nullptr);
	if (!Targeting || !TryBeginExecutionWindow())
	{
		return;
	}

	TArray<FKCActionTarget> Targets;
	Targeting->GatherTargets(BuildTargetingContext(), Targets);
	ExecuteTargets(Targets);
}

FKCActionTargetingContext UKCGA_ActionRuntimeBase::BuildTargetingContext() const
{
	FKCActionTargetingContext Context;
	Context.SourceActor = GetAvatarActorFromActorInfo();
	Context.SourceObject = GetCurrentSourceObject();
	Context.ActivationTarget = ActivationTarget.Get();
	Context.ActivationHitResult = ActivationHitResult;
	Context.bHasActivationHitResult = bHasActivationHitResult;
	return Context;
}

bool UKCGA_ActionRuntimeBase::ExecuteTargets(
	const TArray<FKCActionTarget>& Targets)
{
	bool bConfirmedHit = false;
	bool bAnyExecutionSucceeded = false;
	for (const FKCActionTarget& Target : Targets)
	{
		if (!IsValid(Target.Actor))
		{
			continue;
		}
		bConfirmedHit |= Target.bHasHitResult;

		UAbilitySystemComponent* TargetAbilitySystem =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target.Actor);
		bAnyExecutionSucceeded |= ExecuteActionHook(
			TAG_KC_ActionHook_OnExecute,
			TargetAbilitySystem,
			Target.Actor,
			Target.bHasHitResult ? &Target.HitResult : nullptr);
	}

	if (bConfirmedHit && !bDurabilityConsumedThisActivation &&
		TryConsumeActiveItemDurability(
			EKCItemDurabilityConsumeMode::OnFirstHit))
	{
		bDurabilityConsumedThisActivation = true;
	}

	if (bAnyExecutionSucceeded &&
		!bUseConsumptionPendingThisActivation &&
		TryBeginActiveItemUseConsumption())
	{
		bUseConsumptionPendingThisActivation = true;
	}

	return bAnyExecutionSucceeded;
}

AKCWorldItemActor* UKCGA_ActionRuntimeBase::ResolveSourceItem(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return nullptr;
	}

	const FGameplayAbilitySpec* Spec =
		ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
	const UKCAbilitySourceComponent* SourceComponent = Spec
		? Cast<UKCAbilitySourceComponent>(Spec->SourceObject.Get())
		: nullptr;
	return SourceComponent
		? Cast<AKCWorldItemActor>(SourceComponent->GetOwner())
		: nullptr;
}

bool UKCGA_ActionRuntimeBase::TryConsumeActiveItemDurability(
	EKCItemDurabilityConsumeMode ConsumeMode,
	float ConsumptionScale)
{
	AKCWorldItemActor* Item = ActiveSourceItem.Get();
	return Item && Item->TryConsumeDurability(ConsumeMode, ConsumptionScale);
}

bool UKCGA_ActionRuntimeBase::TryBeginActiveItemUseConsumption()
{
	AKCWorldItemActor* Item = ActiveSourceItem.Get();
	return Item && Item->TryBeginUseConsumption();
}

void UKCGA_ActionRuntimeBase::StartActiveDurabilityDrain()
{
	AKCWorldItemActor* Item = ActiveSourceItem.Get();
	const UKCItemDefinition* ItemDefinition = Item
		? Item->GetItemDefinition()
		: nullptr;
	UWorld* World = GetWorld();
	if (!World || !ItemDefinition ||
		ItemDefinition->Durability.ConsumeMode !=
			EKCItemDurabilityConsumeMode::WhileActive)
	{
		return;
	}

	bDurabilityDrainActive = true;
	LastDurabilityDrainTimeSeconds = World->GetTimeSeconds();
	World->GetTimerManager().SetTimer(
		DurabilityDrainTimerHandle,
		this,
		&UKCGA_ActionRuntimeBase::TickActiveDurabilityDrain,
		0.1f,
		true);
}

void UKCGA_ActionRuntimeBase::StopActiveDurabilityDrain(
	bool bConsumeRemainingTime)
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(DurabilityDrainTimerHandle);
	}

	if (bDurabilityDrainActive && bConsumeRemainingTime && World)
	{
		const double CurrentTimeSeconds = World->GetTimeSeconds();
		const float ElapsedSeconds = static_cast<float>(FMath::Max(
			0.0,
			CurrentTimeSeconds - LastDurabilityDrainTimeSeconds));
		if (ElapsedSeconds > 0.0f)
		{
			TryConsumeActiveItemDurability(
				EKCItemDurabilityConsumeMode::WhileActive,
				ElapsedSeconds);
		}
	}

	bDurabilityDrainActive = false;
	LastDurabilityDrainTimeSeconds = 0.0;
}

void UKCGA_ActionRuntimeBase::TickActiveDurabilityDrain()
{
	if (!bDurabilityDrainActive || !IsActive() || IsFinishingAction())
	{
		StopActiveDurabilityDrain(false);
		return;
	}

	UWorld* World = GetWorld();
	AKCWorldItemActor* Item = ActiveSourceItem.Get();
	if (!World || !Item)
	{
		StopActiveDurabilityDrain(false);
		return;
	}

	const double CurrentTimeSeconds = World->GetTimeSeconds();
	const float ElapsedSeconds = static_cast<float>(FMath::Max(
		0.0,
		CurrentTimeSeconds - LastDurabilityDrainTimeSeconds));
	LastDurabilityDrainTimeSeconds = CurrentTimeSeconds;
	if (ElapsedSeconds > 0.0f)
	{
		TryConsumeActiveItemDurability(
			EKCItemDurabilityConsumeMode::WhileActive,
			ElapsedSeconds);
	}

	if (Item->IsBroken())
	{
		FinishAction(true, false);
	}
}

void UKCGA_ActionRuntimeBase::NotifySocketTraceWindowBegin()
{
	if (ActiveTraceTask)
	{
		ActiveTraceTask->BeginTraceWindow();
	}
}

void UKCGA_ActionRuntimeBase::NotifySocketTraceWindowTick()
{
	if (ActiveTraceTask)
	{
		ActiveTraceTask->TickTraceWindow();
	}
}

void UKCGA_ActionRuntimeBase::NotifySocketTraceWindowEnd()
{
	if (ActiveTraceTask)
	{
		ActiveTraceTask->EndTraceWindow();
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
