#include "ProjectKC/AbilitySystem/Definition/KCChannelActionDefinition.h"

#include "ProjectKC/AbilitySystem/Ability/KCGA_ChannelAction.h"

TSubclassOf<UKCGA_Base> UKCChannelActionDefinition::GetAbilityClass() const
{
	return UKCGA_ChannelAction::StaticClass();
}

bool UKCChannelActionDefinition::ValidateLifecycle(FString& OutError) const
{
	if (!IsValid(ActionMontage.Montage))
	{
		OutError = TEXT("Channel Action에는 반복 실행 시점을 제공할 Montage가 필요합니다.");
		return false;
	}

	return true;
}
