#include "ProjectKC/AbilitySystem/Definition/KCSingleActionDefinition.h"

#include "ProjectKC/AbilitySystem/Ability/KCGA_Action.h"
#include "ProjectKC/AbilitySystem/Fragment/KCThrowProjectileFragment.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"

TSubclassOf<UKCGA_Base> UKCSingleActionDefinition::GetAbilityClass() const
{
	return UKCGA_Action::StaticClass();
}

bool UKCSingleActionDefinition::ExecutesOnInputRelease() const
{
	return FindChargedThrowProjectileFragment() != nullptr;
}

const UKCThrowProjectileFragment*
UKCSingleActionDefinition::FindChargedThrowProjectileFragment() const
{
	for (const FKCActionHookStruct& Hook : ActionHooks)
	{
		for (const UKCActionFragment* Fragment : Hook.Fragments)
		{
			const UKCThrowProjectileFragment* ThrowFragment =
				Cast<UKCThrowProjectileFragment>(Fragment);
			if (ThrowFragment && ThrowFragment->LaunchConfig.bEnableCharge)
			{
				return ThrowFragment;
			}
		}
	}

	return nullptr;
}

bool UKCSingleActionDefinition::ValidateLifecycle(FString& OutError) const
{
	if (ActionTargeting &&
		ActionTargeting->IsA<UKCTraceWindowTargeting>() &&
		!IsValid(ActionMontage.Montage))
	{
		OutError = TEXT("TraceWindow Targeting을 쓰는 Single Action에는 NotifyState를 담을 Montage가 필요합니다.");
		return false;
	}

	int32 ChargedThrowCount = 0;
	for (const FKCActionHookStruct& Hook : ActionHooks)
	{
		for (const UKCActionFragment* Fragment : Hook.Fragments)
		{
			const UKCThrowProjectileFragment* ThrowFragment =
				Cast<UKCThrowProjectileFragment>(Fragment);
			if (ThrowFragment && ThrowFragment->LaunchConfig.bEnableCharge)
			{
				++ChargedThrowCount;
			}
		}
	}

	if (ChargedThrowCount > 1)
	{
		OutError = TEXT("Single Action 하나에는 충전 Throw Projectile Fragment를 하나만 둘 수 있습니다.");
		return false;
	}

	return true;
}
