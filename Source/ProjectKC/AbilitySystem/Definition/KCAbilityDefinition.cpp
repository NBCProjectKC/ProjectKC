#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"

#include "ProjectKC/AbilitySystem/Ability/KCGameplayAbility.h"
#include "ProjectKC/AbilitySystem/Config/KCActionConfig.h"
#include "ProjectKC/AbilitySystem/Presentation/KCActionMontageConfig.h"

const FKCActionHookStruct* UKCAbilityDefinition::FindActionHook(
	FGameplayTag HookTag) const
{
	return ActionHooks.FindByPredicate(
		[HookTag](const FKCActionHookStruct& Hook)
		{
			return Hook.HookTag.MatchesTagExact(HookTag);
		});
}

bool UKCAbilityDefinition::DeclaresSetByCallerTag(FGameplayTag DataTag) const
{
	return DataTag.IsValid() && ActionHooks.ContainsByPredicate(
		[DataTag](const FKCActionHookStruct& Hook)
		{
			return Hook.DeclaresSetByCallerTag(DataTag);
		});
}

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

	if (ActionMontage)
	{
		FString MontageError;
		if (!ActionMontage->Validate(MontageError))
		{
			OutError = FString::Printf(
				TEXT("ActionMontage가 유효하지 않습니다: %s"),
				*MontageError);
			return false;
		}
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
