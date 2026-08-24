#include "ProjectKC/AbilitySystem/Fragment/KCActionExecutionContext.h"

#include "AbilitySystemComponent.h"

bool FKCActionExecutionContext::IsAuthoritative() const
{
	return SourceAbilitySystem &&
		SourceAbilitySystem->IsOwnerActorAuthoritative();
}

AActor* FKCActionExecutionContext::ResolveScopedActor(
	EKCActionScope Scope) const
{
	return Scope == EKCActionScope::Source ? SourceActor : TargetActor;
}

UAbilitySystemComponent* FKCActionExecutionContext::ResolveScopedAbilitySystem(
	EKCActionScope Scope) const
{
	return Scope == EKCActionScope::Source
		? SourceAbilitySystem
		: TargetAbilitySystem;
}
