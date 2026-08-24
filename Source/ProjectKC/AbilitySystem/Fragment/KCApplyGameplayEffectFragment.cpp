#include "ProjectKC/AbilitySystem/Fragment/KCApplyGameplayEffectFragment.h"

#include "ProjectKC/AbilitySystem/Ability/KCGA_Base.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionExecutionContext.h"
#include "GameplayEffect.h"

bool UKCApplyGameplayEffectFragment::Validate(FString& OutError) const
{
	if (!EffectRecipe.Validate(OutError))
	{
		return false;
	}

	if (bTrackUntilAbilityEnds)
	{
		const UGameplayEffect* EffectCDO = EffectRecipe.EffectClass
			? EffectRecipe.EffectClass->GetDefaultObject<UGameplayEffect>()
			: nullptr;
		if (!EffectCDO ||
			EffectCDO->DurationPolicy != EGameplayEffectDurationType::Infinite)
		{
			OutError = FString::Printf(
				TEXT("GA 종료까지 추적할 Effect '%s'는 Infinite Duration이어야 합니다."),
				*GetNameSafe(EffectCDO));
			return false;
		}
	}

	return true;
}

bool UKCApplyGameplayEffectFragment::DeclaresSetByCallerTag(
	FGameplayTag DataTag) const
{
	return EffectRecipe.SetByCallers.ContainsByPredicate(
		[DataTag](const FKCSetByCallerValueStruct& Value)
		{
			return Value.DataTag.MatchesTagExact(DataTag);
		});
}

void UKCApplyGameplayEffectFragment::AppendDeclaredSetByCallerTags(
	FGameplayTagContainer& OutTags) const
{
	for (const FKCSetByCallerValueStruct& Value : EffectRecipe.SetByCallers)
	{
		if (Value.DataTag.IsValid())
		{
			OutTags.AddTag(Value.DataTag);
		}
	}
}

bool UKCApplyGameplayEffectFragment::CanExecute(
	const FKCActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	if (!Context.Ability || !Context.SourceAbilitySystem ||
		!Context.ResolveScopedAbilitySystem(ApplicationScope))
	{
		OutError = TEXT("Gameplay Effect를 적용할 Source 또는 Target ASC가 없습니다.");
		return false;
	}

	if (!Context.IsAuthoritative())
	{
		OutError = TEXT("Gameplay Effect 적용 권한이 없습니다.");
		return false;
	}

	return true;
}

bool UKCApplyGameplayEffectFragment::Execute(
	const FKCActionExecutionContext& Context) const
{
	return Context.Ability && Context.Ability->ApplyGameplayEffectRecipe(
		EffectRecipe,
		Context,
		Context.ResolveScopedAbilitySystem(ApplicationScope),
		bTrackUntilAbilityEnds);
}
