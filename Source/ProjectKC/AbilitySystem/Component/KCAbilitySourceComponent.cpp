#include "ProjectKC/AbilitySystem/Component/KCAbilitySourceComponent.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UKCAbilitySourceComponent::UKCAbilitySourceComponent()
{
	SetIsReplicatedByDefault(true);
}

bool UKCAbilitySourceComponent::ResolveAbilityDefinition(
	const UKCAbilityDefinition*& OutDefinition) const
{
	OutDefinition = AbilityDefinition;
	return IsValid(AbilityDefinition);
}

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

bool UKCAbilitySourceComponent::TryActivate()
{
	UKCAbilitySystemComponent* AbilitySystem = GetGrantedAbilitySystem();
	return AbilitySystem &&
		AbilitySystem->TryActivateGrantedAbility(BindingState.AbilityHandle);
}

bool UKCAbilitySourceComponent::TryActivateWithTarget(AActor* TargetActor)
{
	return TryActivateWithTargetInternal(TargetActor, nullptr);
}

bool UKCAbilitySourceComponent::TryActivateWithHitResult(
	AActor* TargetActor,
	const FHitResult& HitResult)
{
	return TryActivateWithTargetInternal(TargetActor, &HitResult);
}

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

FGameplayAbilitySpecHandle UKCAbilitySourceComponent::GetGrantedAbilityHandle() const
{
	return BindingState.AbilityHandle;
}

bool UKCAbilitySourceComponent::HasAbilityDefinition() const
{
	return IsValid(AbilityDefinition);
}

void UKCAbilitySourceComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	Revoke(true);
	Super::EndPlay(EndPlayReason);
}

void UKCAbilitySourceComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UKCAbilitySourceComponent, BindingState);
}

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
