#include "ProjectKC/AbilitySystem/Definition/KCSingleActionDefinition.h"

#include "ProjectKC/AbilitySystem/Ability/KCGA_Action.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"

TSubclassOf<UKCGA_Base> UKCSingleActionDefinition::GetAbilityClass() const
{
	return UKCGA_Action::StaticClass();
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

	return true;
}
