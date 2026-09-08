#include "ProjectKC/AbilitySystem/Fragment/KCApplyMontageHitLagFragment.h"

#include "ProjectKC/AbilitySystem/Ability/KCGA_ActionRuntimeBase.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionExecutionContext.h"

UKCApplyMontageHitLagFragment::UKCApplyMontageHitLagFragment()
{
	ApplicationScope = EKCActionScope::Source;
	bRequired = false;
}

bool UKCApplyMontageHitLagFragment::Validate(FString& OutError) const
{
	if (ApplicationScope != EKCActionScope::Source)
	{
		OutError = TEXT("Montage Hit Lag Fragment의 ApplicationScope는 Source여야 합니다.");
		return false;
	}

	return HitLag.Validate(OutError);
}

bool UKCApplyMontageHitLagFragment::CanExecute(
	const FKCActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	const UKCGA_ActionRuntimeBase* ActionAbility =
		Cast<UKCGA_ActionRuntimeBase>(Context.Ability);
	if (!Context.IsAuthoritative() || !Context.bHasHitResult || !ActionAbility ||
		!ActionAbility->CanApplyMontageHitLag())
	{
		OutError = TEXT("역경직을 적용할 명중 문맥, 서버 권한 또는 재생 중인 Action Montage가 없습니다.");
		return false;
	}

	return true;
}

bool UKCApplyMontageHitLagFragment::Execute(
	const FKCActionExecutionContext& Context) const
{
	UKCGA_ActionRuntimeBase* ActionAbility =
		Cast<UKCGA_ActionRuntimeBase>(Context.Ability);
	return ActionAbility && ActionAbility->ApplyMontageHitLag(HitLag);
}
