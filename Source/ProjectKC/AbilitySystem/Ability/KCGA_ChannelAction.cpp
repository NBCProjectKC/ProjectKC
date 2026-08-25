#include "ProjectKC/AbilitySystem/Ability/KCGA_ChannelAction.h"

#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"
#include "ProjectKC/AbilitySystem/Task/KCAbilityTask_PlayActionMontage.h"

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

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}

	const UKCAbilityDefinition* Definition = GetActiveDefinition();
	if (!Definition || !IsValid(Definition->ActionMontage.Montage))
	{
		FinishAction(true, false);
		return;
	}

	ActiveMontageTask = UKCAbilityTask_PlayActionMontage::Create(
		this,
		Definition->ActionMontage,
		Definition->ActionTargeting->IsA<UKCInstantActionTargeting>());
	if (!ActiveMontageTask)
	{
		FinishAction(true, false);
		return;
	}

	ActiveMontageTask->OnExecute.AddDynamic(
		this,
		&UKCGA_ChannelAction::HandleExecuteEvent);
	ActiveMontageTask->OnCompleted.AddDynamic(
		this,
		&UKCGA_ChannelAction::HandleMontageEnded);
	ActiveMontageTask->OnInterrupted.AddDynamic(
		this,
		&UKCGA_ChannelAction::HandleMontageEnded);
	ActiveMontageTask->ReadyForActivation();
}

void UKCGA_ChannelAction::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	FinishAction(false, true);
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
