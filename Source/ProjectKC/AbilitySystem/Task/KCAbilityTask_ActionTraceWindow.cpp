#include "ProjectKC/AbilitySystem/Task/KCAbilityTask_ActionTraceWindow.h"

#include "GameFramework/Actor.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_ActionRuntimeBase.h"

UKCAbilityTask_ActionTraceWindow* UKCAbilityTask_ActionTraceWindow::Create(
	UKCGA_ActionRuntimeBase* OwningAbility,
	const UKCTraceWindowTargeting* InTargeting)
{
	UKCAbilityTask_ActionTraceWindow* Task =
		NewAbilityTask<UKCAbilityTask_ActionTraceWindow>(OwningAbility);
	if (Task)
	{
		Task->RuntimeAbility = OwningAbility;
		Task->Targeting = const_cast<UKCTraceWindowTargeting*>(InTargeting);
	}
	return Task;
}

void UKCAbilityTask_ActionTraceWindow::Activate()
{
	if (!IsValid(RuntimeAbility) || !IsValid(Targeting))
	{
		EndTask();
	}
}

void UKCAbilityTask_ActionTraceWindow::BeginTraceWindow()
{
	if (bWindowActive || !IsValid(RuntimeAbility) ||
		RuntimeAbility->IsFinishingAction() || !IsValid(Targeting))
	{
		return;
	}

	const FKCActionTargetingContext Context =
		RuntimeAbility->BuildTargetingContext();
	UObject* ResolvedSource = nullptr;
	if (!Targeting->ResolveTraceSource(Context, ResolvedSource) ||
		!ResolvedSource ||
		!Targeting->GetTraceSegment(
			*ResolvedSource, PreviousStart, PreviousEnd) ||
		!RuntimeAbility->TryBeginExecutionWindow())
	{
		return;
	}

	TraceSource = ResolvedSource;
	HitActors.Reset();
	bWindowActive = true;

	// 구간 시작 프레임에 이미 겹쳐 있는 대상도 놓치지 않는다.
	TraceToCurrentSegment();
}

void UKCAbilityTask_ActionTraceWindow::TickTraceWindow()
{
	if (bWindowActive && IsValid(RuntimeAbility) &&
		!RuntimeAbility->IsFinishingAction())
	{
		TraceToCurrentSegment();
	}
}

void UKCAbilityTask_ActionTraceWindow::EndTraceWindow()
{
	if (!bWindowActive)
	{
		return;
	}

	// Montage 중단 시에도 NotifyEnd가 Interrupted Delegate보다 먼저 호출된다.
	// 취소되는 공격이 여기서 새 판정을 만들지 않도록 구간 상태만 정리한다.
	ResetWindow();
}

void UKCAbilityTask_ActionTraceWindow::OnDestroy(bool bAbilityEnded)
{
	ResetWindow();
	RuntimeAbility = nullptr;
	Targeting = nullptr;
	Super::OnDestroy(bAbilityEnded);
}

bool UKCAbilityTask_ActionTraceWindow::TraceToCurrentSegment()
{
	if (!bWindowActive || !IsValid(RuntimeAbility) ||
		!IsValid(Targeting) || !IsValid(TraceSource) ||
		HitActors.Num() >= Targeting->GetMaxTargets())
	{
		return false;
	}

	FVector CurrentStart;
	FVector CurrentEnd;
	if (!Targeting->GetTraceSegment(
		*TraceSource, CurrentStart, CurrentEnd))
	{
		return false;
	}

	const FKCActionTargetingContext Context =
		RuntimeAbility->BuildTargetingContext();
	TArray<FKCActionTarget> GatheredTargets;
	Targeting->GatherTraceTargets(
		Context,
		PreviousStart,
		PreviousEnd,
		CurrentStart,
		CurrentEnd,
		GatheredTargets);

	TArray<FKCActionTarget> NewTargets;
	for (const FKCActionTarget& Target : GatheredTargets)
	{
		if (HitActors.Num() >= Targeting->GetMaxTargets())
		{
			break;
		}

		const TWeakObjectPtr<AActor> TargetKey(Target.Actor.Get());
		if (!TargetKey.IsValid() || HitActors.Contains(TargetKey))
		{
			continue;
		}

		HitActors.Add(TargetKey);
		NewTargets.Add(Target);
	}

	if (!NewTargets.IsEmpty())
	{
		RuntimeAbility->ExecuteTargets(NewTargets);
	}

	PreviousStart = CurrentStart;
	PreviousEnd = CurrentEnd;
	return true;
}

void UKCAbilityTask_ActionTraceWindow::ResetWindow()
{
	bWindowActive = false;
	TraceSource = nullptr;
	HitActors.Reset();
	PreviousStart = FVector::ZeroVector;
	PreviousEnd = FVector::ZeroVector;
}
