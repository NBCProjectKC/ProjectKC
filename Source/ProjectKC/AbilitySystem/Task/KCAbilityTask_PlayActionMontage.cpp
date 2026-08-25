#include "ProjectKC/AbilitySystem/Task/KCAbilityTask_PlayActionMontage.h"

#include "Abilities/GameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

UKCAbilityTask_PlayActionMontage* UKCAbilityTask_PlayActionMontage::Create(
	UGameplayAbility* OwningAbility,
	const FKCActionMontageConfigStruct& MontageConfig)
{
	UKCAbilityTask_PlayActionMontage* Task =
		NewAbilityTask<UKCAbilityTask_PlayActionMontage>(OwningAbility);
	if (Task)
	{
		Task->Montage = MontageConfig.Montage;
		Task->PlayRate = MontageConfig.PlayRate;
		Task->StartSection = MontageConfig.StartSection;
	}
	return Task;
}

void UKCAbilityTask_PlayActionMontage::Activate()
{
	if (!Ability || !IsValid(Montage))
	{
		FinishTask(true);
		return;
	}

	// 첫 프레임 Notify도 놓치지 않도록 Event 대기를 몽타주보다 먼저 등록한다.
	EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		Ability,
		TAG_KC_GameplayEvent_Action_Execute,
		nullptr,
		false,
		true);
	if (!EventTask)
	{
		FinishTask(true);
		return;
	}
	EventTask->EventReceived.AddDynamic(
		this,
		&UKCAbilityTask_PlayActionMontage::HandleGameplayEvent);
	EventTask->ReadyForActivation();

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		Ability,
		NAME_None,
		Montage,
		PlayRate,
		StartSection,
		true);
	if (!MontageTask)
	{
		FinishTask(true);
		return;
	}
	MontageTask->OnCompleted.AddDynamic(
		this,
		&UKCAbilityTask_PlayActionMontage::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(
		this,
		&UKCAbilityTask_PlayActionMontage::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(
		this,
		&UKCAbilityTask_PlayActionMontage::HandleMontageInterrupted);
	MontageTask->ReadyForActivation();

	if (bTerminal)
	{
		return;
	}

	if (UKCAbilitySystemComponent* AbilitySystem =
		Cast<UKCAbilitySystemComponent>(
			Ability->GetAbilitySystemComponentFromActorInfo()))
	{
		AbilitySystem->PlayActionMontageForRemoteOwner(
			Montage,
			PlayRate,
			StartSection);
	}
}

void UKCAbilityTask_PlayActionMontage::OnDestroy(bool bAbilityEnded)
{
	bCleaningUp = true;
	UAbilitySystemComponent* AbilitySystem = Ability
		? Ability->GetAbilitySystemComponentFromActorInfo()
		: nullptr;
	if (!bTerminal && AbilitySystem &&
		AbilitySystem->GetCurrentMontage() == Montage)
	{
		// 부모 태스크가 자식보다 먼저 정리돼도 서버 몽타주가 남지 않게 직접 멈춘다.
		AbilitySystem->CurrentMontageStop();
	}

	// Ability 종료 중에는 GAS가 ActiveTasks를 직접 순회하며 정리한다.
	// 여기서 자식 Task까지 제거하면 같은 배열의 크기가 순회 도중 바뀌어
	// UGameplayAbility::EndAbility의 인덱스가 무효화될 수 있다.
	if (!bAbilityEnded)
	{
		if (EventTask && !EventTask->IsFinished())
		{
			EventTask->EndTask();
		}
		if (MontageTask && !MontageTask->IsFinished())
		{
			MontageTask->EndTask();
		}
	}

	if (UKCAbilitySystemComponent* KCAbilitySystem =
		Cast<UKCAbilitySystemComponent>(AbilitySystem))
	{
		KCAbilitySystem->StopActionMontageForRemoteOwner(Montage);
	}

	EventTask = nullptr;
	MontageTask = nullptr;
	Super::OnDestroy(bAbilityEnded);
}

void UKCAbilityTask_PlayActionMontage::HandleGameplayEvent(
	FGameplayEventData Payload)
{
	if (bTerminal || bCleaningUp || Payload.OptionalObject.Get() != Ability)
	{
		return;
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnExecute.Broadcast(Payload);
	}
}

void UKCAbilityTask_PlayActionMontage::HandleMontageCompleted()
{
	FinishTask(false);
}

void UKCAbilityTask_PlayActionMontage::HandleMontageInterrupted()
{
	FinishTask(true);
}

void UKCAbilityTask_PlayActionMontage::FinishTask(bool bInterrupted)
{
	if (bTerminal || bCleaningUp)
	{
		return;
	}

	bTerminal = true;
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		if (bInterrupted)
		{
			OnInterrupted.Broadcast();
		}
		else
		{
			OnCompleted.Broadcast();
		}
	}

	if (!IsFinished())
	{
		EndTask();
	}
}
