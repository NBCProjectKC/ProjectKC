#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"

#include "ProjectKC/AbilitySystem/Ability/KCGameplayAbility.h"
#include "ProjectKC/AbilitySystem/Config/KCActionConfig.h"

/**
 * @brief Finds an action hook with the specified tag.
 *
 * @param HookTag Tag to match exactly.
 * @return const FKCActionHookStruct* Pointer to the matching action hook, or nullptr if no match exists.
 */
const FKCActionHookStruct* UKCAbilityDefinition::FindActionHook(
	FGameplayTag HookTag) const
{
	return ActionHooks.FindByPredicate(
		[HookTag](const FKCActionHookStruct& Hook)
		{
			return Hook.HookTag.MatchesTagExact(HookTag);
		});
}

/**
 * @brief Determines whether any action hook declares the specified SetByCaller tag.
 *
 * @param DataTag SetByCaller tag to search for.
 * @return true if the tag is valid and declared by an action hook, false otherwise.
 */
bool UKCAbilityDefinition::DeclaresSetByCallerTag(FGameplayTag DataTag) const
{
	return DataTag.IsValid() && ActionHooks.ContainsByPredicate(
		[DataTag](const FKCActionHookStruct& Hook)
		{
			return Hook.DeclaresSetByCallerTag(DataTag);
		});
}

/**
 * @brief Validates the ability definition and its action hooks.
 *
 * @param OutError Receives a description of the first validation failure, or is cleared when validation succeeds.
 * @return bool `true` if the definition is valid, `false` otherwise.
 */
bool UKCAbilityDefinition::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!ActionClass)
	{
		OutError = TEXT("ActionClass가 비어 있습니다.");
		return false;
	}

	if (AbilityLevel < 1)
	{
		OutError = TEXT("AbilityLevel은 1 이상이어야 합니다.");
		return false;
	}

	FString MontageError;
	if (!ActionMontage.Validate(MontageError))
	{
		OutError = FString::Printf(
			TEXT("ActionMontage가 유효하지 않습니다: %s"),
			*MontageError);
		return false;
	}

	if (ActionConfig)
	{
		FString ConfigError;
		if (!ActionConfig->Validate(ConfigError))
		{
			OutError = FString::Printf(
				TEXT("ActionConfig가 유효하지 않습니다: %s"),
				*ConfigError);
			return false;
		}
	}

	TSet<FGameplayTag> SeenHooks;
	TSet<FGameplayTag> SeenSetByCallerTags;
	for (int32 Index = 0; Index < ActionHooks.Num(); ++Index)
	{
		const FKCActionHookStruct& Hook = ActionHooks[Index];
		FString HookError;
		if (!Hook.Validate(HookError))
		{
			OutError = FString::Printf(
				TEXT("ActionHooks[%d]가 유효하지 않습니다: %s"),
				Index,
				*HookError);
			return false;
		}

		if (SeenHooks.Contains(Hook.HookTag))
		{
			OutError = FString::Printf(
				TEXT("Action Hook '%s'가 중복됩니다."),
				*Hook.HookTag.ToString());
			return false;
		}
		SeenHooks.Add(Hook.HookTag);

		for (const UKCActionFragment* Fragment : Hook.Fragments)
		{
			FGameplayTagContainer FragmentTags;
			Fragment->AppendDeclaredSetByCallerTags(FragmentTags);
			for (const FGameplayTag& DataTag : FragmentTags)
			{
				if (SeenSetByCallerTags.Contains(DataTag))
				{
					OutError = FString::Printf(
						TEXT("SetByCaller 태그 '%s'가 Definition 전체에서 중복됩니다."),
						*DataTag.ToString());
					return false;
				}
				SeenSetByCallerTags.Add(DataTag);
			}
		}
	}

	return true;
}

/**
 * @brief Validates the ability definition against its action class contract.
 *
 * @param OutError Receives an error message when validation fails.
 * @return true if the definition passes local and action contract validation, false otherwise.
 */
bool UKCAbilityDefinition::ValidateWithActionContract(FString& OutError) const
{
	if (!Validate(OutError))
	{
		return false;
	}

	const UKCGameplayAbility* AbilityCDO =
		ActionClass->GetDefaultObject<UKCGameplayAbility>();
	if (!AbilityCDO)
	{
		OutError = TEXT("ActionClass의 UKCGameplayAbility CDO를 읽을 수 없습니다.");
		return false;
	}

	return AbilityCDO->ValidateDefinitionContract(*this, OutError);
}
