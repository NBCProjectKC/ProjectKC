#include "ProjectKC/AbilitySystem/Ability/KCGAUseActionBase.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Tag/KCGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCUseAction, Log, All);

UKCGAUseActionBase::UKCGAUseActionBase()
{
	SetSupportsActionMontage(true);
}

void UKCGAUseActionBase::ActivateAbility(
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

	// InstancedPerActor 인스턴스는 재사용되므로 사용 상태를 매번 초기화한다.
	RemoteOwnerMontage = nullptr;
	bStopRemoteOwnerMontageOnEnd = false;
	bUseActionAttempted = false;
	bUseActionSucceeded = false;
	bFinishingUseAction = false;

	if (!PrepareUseAction(TriggerEventData))
	{
		FinishUseAction(true);
		return;
	}

	// 명중 여부가 아니라 사용 시도 자체에 Cost/Cooldown을 소모한다.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishUseAction(true);
		return;
	}

	const UKCAbilityDefinition* Definition = GetActiveDefinition();
	if (!Definition)
	{
		FinishUseAction(true);
		return;
	}

	// 함정처럼 Avatar 애니메이션이 없는 소스는 기존 즉시 실행 경로를 유지한다.
	if (!Definition->ActionMontage.HasMontage())
	{
		RunUseActionOnce();
		FinishUseAction(!bUseActionSucceeded);
		return;
	}

	if (!StartActionMontage(Definition->ActionMontage))
	{
		FinishUseAction(true);
	}
}

void UKCGAUseActionBase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bFinishingUseAction = true;

	// 취소된 사용만 원격 소유 클라이언트의 연출을 끊는다.
	// 정상 완료는 클라이언트 몽타주가 자기 타임라인대로 끝나게 둔다.
	if (bWasCancelled && bStopRemoteOwnerMontageOnEnd &&
		IsValid(RemoteOwnerMontage))
	{
		UKCAbilitySystemComponent* KCAbilitySystem =
			Cast<UKCAbilitySystemComponent>(
				ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr);
		if (KCAbilitySystem)
		{
			KCAbilitySystem->StopActionMontageForRemoteOwner(RemoteOwnerMontage);
		}
	}

	RemoteOwnerMontage = nullptr;
	bStopRemoteOwnerMontageOnEnd = false;

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

bool UKCGAUseActionBase::StartActionMontage(
	const FKCActionMontageSpec& MontageSpec)
{
	UAnimMontage* Montage = MontageSpec.Montage.Get();
	if (!IsValid(Montage))
	{
		return false;
	}

	// 첫 프레임에 배치된 Notify도 놓치지 않도록 Event 대기를 재생보다 먼저 등록한다.
	UAbilityTask_WaitGameplayEvent* EventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			TAG_KC_GameplayEvent_Action_Execute,
			nullptr,
			false,
			true);
	if (!EventTask)
	{
		return false;
	}

	EventTask->EventReceived.AddDynamic(
		this,
		&UKCGAUseActionBase::OnActionExecuteEventReceived);
	EventTask->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			Montage,
			MontageSpec.PlayRate,
			MontageSpec.StartSection,
			MontageSpec.bStopWhenAbilityEnds);
	if (!MontageTask)
	{
		return false;
	}

	MontageTask->OnCompleted.AddDynamic(
		this,
		&UKCGAUseActionBase::OnActionMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(
		this,
		&UKCGAUseActionBase::OnActionMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(
		this,
		&UKCGAUseActionBase::OnActionMontageAborted);
	MontageTask->OnCancelled.AddDynamic(
		this,
		&UKCGAUseActionBase::OnActionMontageAborted);

	// AnimInstance가 없거나 재생에 실패하면 이 호출 안에서 바로 취소가 전파된다.
	MontageTask->ReadyForActivation();
	if (!IsActive())
	{
		UE_LOG(
			LogKCUseAction,
			Warning,
			TEXT("Action Montage '%s'를 재생하지 못해 사용을 취소했습니다. Ability='%s'"),
			*GetNameSafe(Montage),
			*GetName());
		return false;
	}

	RemoteOwnerMontage = Montage;
	bStopRemoteOwnerMontageOnEnd = MontageSpec.bStopWhenAbilityEnds;

	if (UKCAbilitySystemComponent* KCAbilitySystem =
		Cast<UKCAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		KCAbilitySystem->PlayActionMontageForRemoteOwner(
			Montage,
			MontageSpec.PlayRate,
			MontageSpec.StartSection);
	}

	return true;
}

void UKCGAUseActionBase::OnActionExecuteEventReceived(FGameplayEventData Payload)
{
	if (bFinishingUseAction || bUseActionAttempted)
	{
		return;
	}

	RunUseActionOnce();
	if (!bUseActionSucceeded)
	{
		// 대기 중 대상이 사라진 경우처럼 결과를 만들 수 없으면 사용을 취소한다.
		FinishUseAction(true);
	}
}

void UKCGAUseActionBase::OnActionMontageFinished()
{
	if (bFinishingUseAction)
	{
		return;
	}

	if (!bUseActionAttempted)
	{
		UE_LOG(
			LogKCUseAction,
			Warning,
			TEXT("Action Montage '%s'가 끝날 때까지 '%s' Notify가 도착하지 않았습니다. Ability='%s'"),
			*GetNameSafe(RemoteOwnerMontage),
			*TAG_KC_GameplayEvent_Action_Execute.GetTag().ToString(),
			*GetName());
		FinishUseAction(true);
		return;
	}

	FinishUseAction(!bUseActionSucceeded);
}

void UKCGAUseActionBase::OnActionMontageAborted()
{
	FinishUseAction(true);
}

void UKCGAUseActionBase::RunUseActionOnce()
{
	if (bUseActionAttempted)
	{
		return;
	}

	// Execute Event가 중복 도착해도 같은 사용에서 결과는 한 번만 만든다.
	bUseActionAttempted = true;
	bUseActionSucceeded = ExecuteUseAction();
}

void UKCGAUseActionBase::FinishUseAction(bool bWasCancelled)
{
	if (bFinishingUseAction)
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
