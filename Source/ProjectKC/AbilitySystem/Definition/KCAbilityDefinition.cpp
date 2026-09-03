#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"

#include "ProjectKC/AbilitySystem/Ability/KCGA_Base.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"

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

TSubclassOf<UKCGA_Base> UKCAbilityDefinition::GetAbilityClass() const
{
	return nullptr;
}

bool UKCAbilityDefinition::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!GetAbilityClass())
	{
		OutError = TEXT("Definition에 대응하는 Ability 클래스가 없습니다.");
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

	FString CooldownError;
	if (!ActionCooldown.Validate(CooldownError))
	{
		OutError = FString::Printf(
			TEXT("ActionCooldown이 유효하지 않습니다: %s"),
			*CooldownError);
		return false;
	}

	if (!ActionTargeting)
	{
		OutError = TEXT("ActionTargeting이 비어 있습니다. 대상 수집 방식을 지정해야 합니다.");
		return false;
	}

	FString TargetingError;
	if (!ActionTargeting->Validate(TargetingError))
	{
		OutError = FString::Printf(
			TEXT("ActionTargeting이 유효하지 않습니다: %s"),
			*TargetingError);
		return false;
	}

	TSet<FGameplayTag> SeenHooks;
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

	}

	return ValidateLifecycle(OutError);
}

bool UKCAbilityDefinition::ValidateWithActionContract(FString& OutError) const
{
	if (!Validate(OutError))
	{
		return false;
	}

	const TSubclassOf<UKCGA_Base> AbilityClass = GetAbilityClass();
	const UKCGA_Base* AbilityCDO = AbilityClass
		? AbilityClass->GetDefaultObject<UKCGA_Base>()
		: nullptr;
	if (!AbilityCDO)
	{
		OutError = TEXT("대응하는 Ability 클래스의 UKCGA_Base CDO를 읽을 수 없습니다.");
		return false;
	}

	if (!ActionTargeting->IsA<UKCInstantActionTargeting>() &&
		!ActionTargeting->IsA<UKCTraceWindowTargeting>())
	{
		OutError = TEXT("ActionTargeting이 Instant 또는 TraceWindow 수집 계약을 구현하지 않습니다.");
		return false;
	}

	return AbilityCDO->ValidateDefinitionContract(*this, OutError);
}

bool UKCAbilityDefinition::ValidateLifecycle(FString& OutError) const
{
	return true;
}
