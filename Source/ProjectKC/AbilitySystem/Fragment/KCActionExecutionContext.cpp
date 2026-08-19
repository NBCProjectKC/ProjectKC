#include "ProjectKC/AbilitySystem/Fragment/KCActionExecutionContext.h"

#include "AbilitySystemComponent.h"

bool FKCActionExecutionContext::IsAuthoritative() const
{
	return SourceAbilitySystem &&
		SourceAbilitySystem->IsOwnerActorAuthoritative();
}
