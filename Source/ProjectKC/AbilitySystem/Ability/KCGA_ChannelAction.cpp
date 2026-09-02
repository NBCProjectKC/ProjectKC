#include "ProjectKC/AbilitySystem/Ability/KCGA_ChannelAction.h"

#include "Engine/World.h"
#include "ProjectKC/AbilitySystem/Definition/KCChannelActionDefinition.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"
#include "ProjectKC/AbilitySystem/Task/KCAbilityTask_PlayActionMontage.h"
#include "TimerManager.h"

UKCGA_ChannelAction::UKCGA_ChannelAction()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UKCGA_ChannelAction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ActiveMontageTask = nullptr;
	StopFixedIntervalExecution();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}

	const UKCChannelActionDefinition* Definition =
		Cast<UKCChannelActionDefinition>(GetActiveDefinition());
	if (!Definition || !IsValid(Definition->ActionMontage.Montage))
	{
		FinishAction(true, false);
		return;
	}

	const bool bListenForExecuteEvent =
		Definition->ExecutionMode == EKCChannelExecutionMode::MontageEvent &&
		Definition->ActionTargeting->IsA<UKCInstantActionTargeting>();
	ActiveMontageTask = UKCAbilityTask_PlayActionMontage::Create(
		this,
		Definition->ActionMontage,
		bListenForExecuteEvent);
	if (!ActiveMontageTask)
	{
		FinishAction(true, false);
		return;
	}

	if (bListenForExecuteEvent)
	{
		ActiveMontageTask->OnExecute.AddDynamic(
			this,
			&UKCGA_ChannelAction::HandleExecuteEvent);
	}
	ActiveMontageTask->OnCompleted.AddDynamic(
		this,
		&UKCGA_ChannelAction::HandleMontageEnded);
	ActiveMontageTask->OnInterrupted.AddDynamic(
		this,
		&UKCGA_ChannelAction::HandleMontageEnded);
	ActiveMontageTask->ReadyForActivation();

	if (IsActive() && !IsFinishingAction() &&
		Definition->ExecutionMode == EKCChannelExecutionMode::FixedInterval)
	{
		StartFixedIntervalExecution();
	}
}

void UKCGA_ChannelAction::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	FinishAction(false, true);
}

void UKCGA_ChannelAction::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	StopFixedIntervalExecution();
	ActiveMontageTask = nullptr;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UKCGA_ChannelAction::HandleExecuteEvent(FGameplayEventData Payload)
{
	if (!IsFinishingAction())
	{
		ExecutePulse();
	}
}

void UKCGA_ChannelAction::HandleMontageEnded()
{
	// Channel은 Release가 정상 종료다. 재생 완료·중단은 모두 비정상 종료다.
	FinishAction(true, false);
}

bool UKCGA_ChannelAction::TryBeginExecutionWindow()
{
	return !IsFinishingAction();
}

void UKCGA_ChannelAction::StartFixedIntervalExecution()
{
	const UKCChannelActionDefinition* Definition =
		Cast<UKCChannelActionDefinition>(GetActiveDefinition());
	UWorld* World = GetWorld();
	if (!Definition || !World ||
		Definition->ExecutionMode != EKCChannelExecutionMode::FixedInterval)
	{
		FinishAction(true, false);
		return;
	}

	World->GetTimerManager().SetTimer(
		FixedIntervalTimerHandle,
		this,
		&UKCGA_ChannelAction::HandleFixedIntervalPulse,
		Definition->PulseInterval,
		true);

	if (Definition->bExecuteImmediately && IsActive() && !IsFinishingAction())
	{
		ExecutePulse();
	}
}

void UKCGA_ChannelAction::StopFixedIntervalExecution()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FixedIntervalTimerHandle);
	}
	else
	{
		FixedIntervalTimerHandle.Invalidate();
	}
}

void UKCGA_ChannelAction::HandleFixedIntervalPulse()
{
	if (!IsActive() || IsFinishingAction())
	{
		StopFixedIntervalExecution();
		return;
	}

	ExecutePulse();
}
