#include "ProjectKC/AbilitySystem/Fragment/KCApplyGameplayEffectFragment.h"

#include "ProjectKC/AbilitySystem/Ability/KCGameplayAbility.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionExecutionContext.h"
#include "GameplayEffect.h"

/**
 * @brief Validates the configured gameplay effect recipe and tracking requirements.
 *
 * @param OutError Receives an error message when validation fails.
 * @return true if the recipe and tracking configuration are valid, false otherwise.
 */
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

/**
 * @brief Determines whether the effect recipe declares a set-by-caller tag.
 *
 * @param DataTag Tag to search for in the effect recipe.
 * @return true if the recipe contains an exact match for the tag, false otherwise.
 */
bool UKCApplyGameplayEffectFragment::DeclaresSetByCallerTag(
	FGameplayTag DataTag) const
{
	return EffectRecipe.SetByCallers.ContainsByPredicate(
		[DataTag](const FKCSetByCallerValueStruct& Value)
		{
			return Value.DataTag.MatchesTagExact(DataTag);
		});
}

/**
 * @brief Adds the valid SetByCaller data tags declared by the effect recipe to a tag container.
 *
 * @param OutTags Container to which the declared tags are added.
 */
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

/**
 * @brief Determines whether the gameplay effect can execute in the current context.
 *
 * @param Context Execution context containing the ability and source and target ability system components.
 * @param OutError Receives an error message when execution cannot proceed.
 * @return true if all required components exist and execution has authority, false otherwise.
 */
bool UKCApplyGameplayEffectFragment::CanExecute(
	const FKCActionExecutionContext& Context,
	FString& OutError) const
{
	OutError.Reset();
	if (!Context.Ability || !Context.SourceAbilitySystem ||
		!Context.TargetAbilitySystem)
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

/**
 * @brief Applies the configured gameplay effect recipe through the ability.
 *
 * @return true if the ability exists and applies the recipe successfully, false otherwise.
 */
bool UKCApplyGameplayEffectFragment::Execute(
	const FKCActionExecutionContext& Context) const
{
	return Context.Ability && Context.Ability->ApplyGameplayEffectRecipe(
		EffectRecipe,
		Context,
		bTrackUntilAbilityEnds);
}
