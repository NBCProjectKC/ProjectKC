#include "ProjectKC/AbilitySystem/Definition/KCSingleActionDefinition.h"

#include "ProjectKC/AbilitySystem/Ability/KCGA_Action.h"

TSubclassOf<UKCGA_Base> UKCSingleActionDefinition::GetAbilityClass() const
{
	return UKCGA_Action::StaticClass();
}
