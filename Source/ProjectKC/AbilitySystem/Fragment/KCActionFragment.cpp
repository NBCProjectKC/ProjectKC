#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"

bool UKCActionFragment::Validate(FString& OutError) const
{
	OutError.Reset();
	return true;
}

bool UKCActionFragment::DeclaresSetByCallerTag(FGameplayTag DataTag) const
{
	return false;
}

void UKCActionFragment::AppendDeclaredSetByCallerTags(
	FGameplayTagContainer& OutTags) const
{
}

bool UKCActionFragment::SupportsDeferredExecution() const
{
	return false;
}

bool UKCActionFragment::PrepareDeferredExecution(
	const FKCActionExecutionContext& Context,
	FString& OutError)
{
	OutError.Reset();
	if (!SupportsDeferredExecution())
	{
		OutError = TEXT("이 Fragment는 지연 실행을 지원하지 않습니다.");
		return false;
	}
	return true;
}

bool UKCActionFragment::CanExecute(
	const FKCActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	return true;
}
