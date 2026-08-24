#include "ProjectKC/AbilitySystem/Timing/KCMontageActionTiming.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_Action.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCActionTiming, Log, All);

bool UKCMontageActionTiming::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!IsValid(Montage))
	{
		OutError = TEXT("Montage가 비어 있습니다. 몽타주 타이밍을 쓰지 않으려면 ActionTiming을 비워 둡니다.");
		return false;
	}

	if (!FMath::IsFinite(PlayRate) || PlayRate <= 0.0f)
	{
		OutError = TEXT("PlayRate는 0보다 큰 유한한 수여야 합니다.");
		return false;
	}

	if (!StartSection.IsNone() &&
		Montage->GetSectionIndex(StartSection) == INDEX_NONE)
	{
		OutError = FString::Printf(
			TEXT("Montage '%s'에 StartSection '%s'가 없습니다."),
			*Montage->GetName(),
			*StartSection.ToString());
		return false;
	}

	return true;
}

bool UKCMontageActionTiming::ScheduleExecution(UKCGA_Action& Ability) const
{
	UAnimMontage* MontageToPlay = Montage.Get();
	if (!IsValid(MontageToPlay))
	{
		return false;
	}

	// 첫 프레임에 배치된 Notify도 놓치지 않도록 Event 대기를 재생보다 먼저 등록한다.
	UAbilityTask_WaitGameplayEvent* EventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			&Ability,
			TAG_KC_GameplayEvent_Action_Execute,
			nullptr,
			false,
			true);
	if (!EventTask)
	{
		return false;
	}

	EventTask->EventReceived.AddDynamic(
		&Ability,
		&UKCGA_Action::HandleTimingExecuteEvent);
	EventTask->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			&Ability,
			NAME_None,
			MontageToPlay,
			PlayRate,
			StartSection,
			bStopWhenAbilityEnds);
	if (!MontageTask)
	{
		return false;
	}

	MontageTask->OnCompleted.AddDynamic(
		&Ability, &UKCGA_Action::HandleTimingCompleted);
	MontageTask->OnBlendOut.AddDynamic(
		&Ability, &UKCGA_Action::HandleTimingCompleted);
	MontageTask->OnInterrupted.AddDynamic(
		&Ability, &UKCGA_Action::HandleTimingAborted);
	MontageTask->OnCancelled.AddDynamic(
		&Ability, &UKCGA_Action::HandleTimingAborted);

	// AnimInstance가 없거나 재생에 실패하면 이 호출 안에서 바로 취소가 전파된다.
	MontageTask->ReadyForActivation();
	if (!Ability.IsActive())
	{
		UE_LOG(
			LogKCActionTiming,
			Warning,
			TEXT("Montage '%s'를 재생하지 못해 사용을 취소했습니다. Ability='%s'"),
			*GetNameSafe(MontageToPlay),
			*Ability.GetName());
		return false;
	}

	// 서버 몽타주 복제는 조종 중인 클라이언트를 건너뛰므로 연출용으로 따로 보낸다.
	if (UKCAbilitySystemComponent* KCAbilitySystem =
		Cast<UKCAbilitySystemComponent>(
			Ability.GetAbilitySystemComponentFromActorInfo()))
	{
		KCAbilitySystem->PlayActionMontageForRemoteOwner(
			MontageToPlay,
			PlayRate,
			StartSection);
	}

	return true;
}

void UKCMontageActionTiming::CancelExecution(UKCGA_Action& Ability) const
{
	if (!bStopWhenAbilityEnds || !IsValid(Montage))
	{
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = Ability.GetCurrentActorInfo();
	UKCAbilitySystemComponent* KCAbilitySystem =
		Cast<UKCAbilitySystemComponent>(
			ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr);
	if (KCAbilitySystem)
	{
		KCAbilitySystem->StopActionMontageForRemoteOwner(Montage);
	}
}
