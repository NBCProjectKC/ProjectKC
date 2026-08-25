#include "ProjectKC/AbilitySystem/Ability/KCGA_Action.h"

#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"
#include "ProjectKC/AbilitySystem/Task/KCAbilityTask_PlayActionMontage.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCSingleAction, Log, All);

UKCGA_Action::UKCGA_Action()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UKCGA_Action::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bExecuteAttempted = false;
	ActiveMontageTask = nullptr;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (!IsActive())
	{
		return;
	}

	const UKCAbilityDefinition* Definition = GetActiveDefinition();
	if (!Definition)
	{
		FinishAction(true, false);
		return;
	}

	if (!IsValid(Definition->ActionMontage.Montage))
	{
		ExecutePulse();
		FinishAction(!bExecuteAttempted, bExecuteAttempted);
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
		&UKCGA_Action::HandleExecuteEvent);
	ActiveMontageTask->OnCompleted.AddDynamic(
		this,
		&UKCGA_Action::HandleMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(
		this,
		&UKCGA_Action::HandleMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
}

void UKCGA_Action::HandleExecuteEvent(FGameplayEventData Payload)
{
	ExecutePulse();
}

void UKCGA_Action::HandleMontageCompleted()
{
	if (IsFinishingAction())
	{
		return;
	}

	if (!bExecuteAttempted)
	{
		const UKCAbilityDefinition* Definition = GetActiveDefinition();
		const bool bTraceWindowExpected = Definition &&
			Definition->ActionTargeting &&
			Definition->ActionTargeting->IsA<UKCTraceWindowTargeting>();
		UE_LOG(
			LogKCSingleAction,
			Warning,
			TEXT("몽타주가 끝날 때까지 %s가 실행되지 않아 Action을 취소했습니다."),
			bTraceWindowExpected
				? TEXT("KC Action Socket Trace Window")
				: *TAG_KC_GameplayEvent_Action_Execute.GetTag().ToString());
		FinishAction(true, false);
		return;
	}

	FinishAction(false, true);
}

void UKCGA_Action::HandleMontageInterrupted()
{
	FinishAction(true, false);
}

bool UKCGA_Action::TryBeginExecutionWindow()
{
	if (IsFinishingAction() || bExecuteAttempted)
	{
		return false;
	}

	bExecuteAttempted = true;
	return true;
}
