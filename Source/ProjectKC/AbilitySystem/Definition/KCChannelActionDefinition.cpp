#include "ProjectKC/AbilitySystem/Definition/KCChannelActionDefinition.h"

#include "ProjectKC/AbilitySystem/Ability/KCGA_ChannelAction.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"

TSubclassOf<UKCGA_Base> UKCChannelActionDefinition::GetAbilityClass() const
{
	return UKCGA_ChannelAction::StaticClass();
}

bool UKCChannelActionDefinition::ValidateLifecycle(FString& OutError) const
{
	if (!IsValid(ActionMontage.Montage))
	{
		OutError = TEXT("Channel Action에는 유지 동작을 재생할 Montage가 필요합니다.");
		return false;
	}

	if (ExecutionMode == EKCChannelExecutionMode::FixedInterval)
	{
		if (!ActionTargeting ||
			!ActionTargeting->IsA<UKCInstantActionTargeting>())
		{
			OutError = TEXT(
				"FixedInterval Channel Action에는 Instant Targeting이 필요합니다.");
			return false;
		}

		if (!FMath::IsFinite(PulseInterval) ||
			PulseInterval < MinimumPulseInterval)
		{
			OutError = FString::Printf(
				TEXT("FixedInterval의 PulseInterval은 %.2f초 이상의 유한한 값이어야 합니다."),
				MinimumPulseInterval);
			return false;
		}
	}

	return true;
}
