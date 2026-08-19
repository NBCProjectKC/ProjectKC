#include "ProjectKC/AbilitySystem/Component/KCAbilitySourceComponent.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

/**
 * @brief Creates an ability source component with replication enabled by default.
 */
UKCAbilitySourceComponent::UKCAbilitySourceComponent()
{
	SetIsReplicatedByDefault(true);
}

/**
 * @brief Resolves the configured ability definition.
 *
 * @param OutDefinition Receives the configured ability definition.
 * @return true if the configured ability definition is valid, false otherwise.
 */
bool UKCAbilitySourceComponent::ResolveAbilityDefinition(
	const UKCAbilityDefinition*& OutDefinition) const
{
	OutDefinition = AbilityDefinition;
	return IsValid(AbilityDefinition);
}

/**
 * @brief Configures the ability definition used by this component.
 *
 * @param NewDefinition Ability definition to configure.
 * @return true if the definition was configured, false if a different definition is already bound to an ability.
 */
bool UKCAbilitySourceComponent::ConfigureAbilityDefinition(
	UKCAbilityDefinition* NewDefinition)
{
	if (BindingState.AbilityHandle.IsValid() && AbilityDefinition != NewDefinition)
	{
		return false;
	}

	AbilityDefinition = NewDefinition;
	return true;
}

/**
 * @brief Grants this component's configured ability to an ability system.
 *
 * Reuses an existing valid grant, revokes a conflicting grant, or creates a
 * new grant as needed. The owning actor must have server authority.
 *
 * @param AbilitySystem Ability system that receives the ability.
 * @return `true` if the ability is already validly granted or was granted successfully, `false` otherwise.
 */
bool UKCAbilitySourceComponent::GrantToAbilitySystem(
	UKCAbilitySystemComponent* AbilitySystem)
{
	if (!AbilitySystem || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (BindingState.AbilityHandle.IsValid())
	{
		UKCAbilitySystemComponent* ExistingAbilitySystem =
			GetGrantedAbilitySystem();
		if (ExistingAbilitySystem == AbilitySystem &&
			AbilitySystem->FindAbilitySpecFromHandle(
				BindingState.AbilityHandle))
		{
			return true;
		}

		if (ExistingAbilitySystem &&
			ExistingAbilitySystem != AbilitySystem &&
			!Revoke(true))
		{
			return false;
		}
		else if (!ExistingAbilitySystem ||
			ExistingAbilitySystem == AbilitySystem)
		{
			BindingState.Reset();
			AuthorityGrantedAbilitySystem = nullptr;
		}
	}

	const FGameplayAbilitySpecHandle NewHandle =
		AbilitySystem->GrantAbilityFromSource(this);
	if (!NewHandle.IsValid())
	{
		return false;
	}

	BindingState.AbilityHandle = NewHandle;
	BindingState.AbilitySystemOwner = AbilitySystem->GetOwnerActor();
	AuthorityGrantedAbilitySystem = AbilitySystem;
	return true;
}

/**
 * @brief Attempts to activate the granted ability.
 *
 * @return true if the granted ability is activated, false otherwise.
 */
bool UKCAbilitySourceComponent::TryActivate()
{
	UKCAbilitySystemComponent* AbilitySystem = GetGrantedAbilitySystem();
	return AbilitySystem &&
		AbilitySystem->TryActivateGrantedAbility(BindingState.AbilityHandle);
}

/**
 * @brief Requests activation of the granted ability with a target actor.
 *
 * @param TargetActor Actor associated with the activation request.
 * @return true if the activation request succeeds, false otherwise.
 */
bool UKCAbilitySourceComponent::TryActivateWithTarget(AActor* TargetActor)
{
	return TryActivateWithTargetInternal(TargetActor, nullptr);
}

/**
 * @brief Requests activation of the granted ability for a target actor with hit-result context.
 *
 * @param TargetActor Actor associated with the activation request.
 * @param HitResult Hit information included in the activation event.
 * @return true if the activation request succeeds, false otherwise.
 */
bool UKCAbilitySourceComponent::TryActivateWithHitResult(
	AActor* TargetActor,
	const FHitResult& HitResult)
{
	return TryActivateWithTargetInternal(TargetActor, &HitResult);
}

/**
 * @brief Requests activation of the granted ability for a target actor.
 *
 * Optionally includes hit-result context in the activation event.
 *
 * @param TargetActor Actor associated with the activation request.
 * @param HitResult Optional hit result to include in the activation context.
 * @return true if the ability activation request succeeds, false otherwise.
 */
bool UKCAbilitySourceComponent::TryActivateWithTargetInternal(
	AActor* TargetActor,
	const FHitResult* HitResult)
{
	if (!TargetActor || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	UKCAbilitySystemComponent* AbilitySystem = GetGrantedAbilitySystem();
	if (!AbilitySystem)
	{
		return false;
	}

	FGameplayEventData EventData;
	EventData.EventTag = TAG_KC_GameplayEvent_AbilitySource_Trigger;
	EventData.Instigator = AbilitySystem->GetAvatarActor();
	EventData.Target = TargetActor;
	EventData.OptionalObject = this;
	if (HitResult)
	{
		EventData.ContextHandle = AbilitySystem->MakeEffectContext();
		EventData.ContextHandle.AddHitResult(*HitResult, true);
	}

	return AbilitySystem->TryActivateGrantedAbilityWithEvent(
		BindingState.AbilityHandle,
		EventData.EventTag,
		EventData);
}

/**
 * @brief Removes the granted ability and clears its binding state.
 *
 * @param bCancelActiveAbility Whether active instances of the ability should be canceled.
 * @return true if the binding is cleared or the ability is revoked, false if the owner lacks authority or revocation fails.
 */
bool UKCAbilitySourceComponent::Revoke(bool bCancelActiveAbility)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!BindingState.AbilityHandle.IsValid())
	{
		BindingState.Reset();
		AuthorityGrantedAbilitySystem = nullptr;
		return true;
	}

	UKCAbilitySystemComponent* AbilitySystem = GetGrantedAbilitySystem();
	if (!AbilitySystem)
	{
		if (!IsValid(BindingState.AbilitySystemOwner))
		{
			BindingState.Reset();
			AuthorityGrantedAbilitySystem = nullptr;
			return true;
		}
		return false;
	}

	if (!AbilitySystem->FindAbilitySpecFromHandle(BindingState.AbilityHandle))
	{
		BindingState.Reset();
		AuthorityGrantedAbilitySystem = nullptr;
		return true;
	}

	if (!AbilitySystem->RevokeAbilityByHandle(
		BindingState.AbilityHandle,
		bCancelActiveAbility))
	{
		return false;
	}

	BindingState.Reset();
	AuthorityGrantedAbilitySystem = nullptr;
	return true;
}

/**
 * @brief Retrieves the handle of the granted ability.
 *
 * @return FGameplayAbilitySpecHandle The current granted ability handle.
 */
FGameplayAbilitySpecHandle UKCAbilitySourceComponent::GetGrantedAbilityHandle() const
{
	return BindingState.AbilityHandle;
}

/**
 * @brief Determines whether an ability definition is configured and valid.
 *
 * @return `true` if a valid ability definition is configured, `false` otherwise.
 */
bool UKCAbilitySourceComponent::HasAbilityDefinition() const
{
	return IsValid(AbilityDefinition);
}

/**
 * @brief Revokes the granted ability and completes component end-play cleanup.
 *
 * @param EndPlayReason Reason the component is being removed from play.
 */
void UKCAbilitySourceComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	Revoke(true);
	Super::EndPlay(EndPlayReason);
}

/**
 * @brief Registers the component properties replicated across the network.
 *
 * @param OutLifetimeProps Array that receives the properties to replicate.
 */
void UKCAbilitySourceComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKCAbilitySourceComponent, BindingState);
}

/**
 * @brief Retrieves the ability system associated with the granted ability.
 *
 * @return UKCAbilitySystemComponent* The valid authoritative ability system, or the ability system resolved from the stored owner; nullptr if none is available.
 */
UKCAbilitySystemComponent* UKCAbilitySourceComponent::GetGrantedAbilitySystem() const
{
	if (IsValid(AuthorityGrantedAbilitySystem))
	{
		return AuthorityGrantedAbilitySystem;
	}

	return BindingState.AbilitySystemOwner
		? Cast<UKCAbilitySystemComponent>(
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
				BindingState.AbilitySystemOwner))
		: nullptr;
}
