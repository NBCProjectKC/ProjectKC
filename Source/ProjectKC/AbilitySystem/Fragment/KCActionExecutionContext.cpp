#include "ProjectKC/AbilitySystem/Fragment/KCActionExecutionContext.h"

#include "AbilitySystemComponent.h"

/**
 * @brief Determines whether the source ability system's owner actor is authoritative.
 *
 * @return `true` if the source ability system exists and its owner actor is authoritative, `false` otherwise.
 */
bool FKCActionExecutionContext::IsAuthoritative() const
{
	return SourceAbilitySystem &&
		SourceAbilitySystem->IsOwnerActorAuthoritative();
}
