#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "ProjectKC/AbilitySystem/Ability/KCGameplayAbility.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Interface/KCAbilitySourceInterface.h"
#include "GameplayAbilitySpec.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCAbilitySystem, Log, All);

/**
 * @brief Resolves and validates an ability definition provided by a source object.
 *
 * @param SourceObject Object that provides the ability definition.
 * @param OutDefinition Receives the resolved ability definition on success.
 * @param OutError Optional string that receives the failure reason.
 * @return true if a valid ability definition with a valid action contract was resolved, false otherwise.
 */
bool UKCAbilitySystemComponent::ResolveDefinitionFromSource(
	const UObject* SourceObject,
	const UKCAbilityDefinition*& OutDefinition,
	FString* OutError)
{
	OutDefinition = nullptr;

	const IKCAbilitySourceInterface* AbilitySource =
		Cast<IKCAbilitySourceInterface>(SourceObject);
	if (!AbilitySource)
	{
		if (OutError)
		{
			*OutError = TEXT("SourceObject가 KCAbilitySourceInterface를 구현하지 않습니다.");
		}
		return false;
	}

	if (!AbilitySource->ResolveAbilityDefinition(OutDefinition) ||
		!IsValid(OutDefinition))
	{
		if (OutError)
		{
			*OutError = TEXT("SourceObject가 Ability Definition을 제공하지 않았습니다.");
		}
		return false;
	}

	FString ValidationError;
	if (!OutDefinition->ValidateWithActionContract(ValidationError))
	{
		if (OutError)
		{
			*OutError = ValidationError;
		}
		return false;
	}

	return true;
}

/**
 * @brief Grants the ability defined by a source object.
 *
 * @param SourceObject Object that provides the ability definition.
 * @param InputId Input identifier assigned to the granted ability.
 * @return Handle of the granted ability, or an invalid handle when the source does not provide a valid definition.
 */
FGameplayAbilitySpecHandle UKCAbilitySystemComponent::GrantAbilityFromSource(
	UObject* SourceObject,
	int32 InputId)
{
	const UKCAbilityDefinition* Definition = nullptr;
	FString Error;
	if (!ResolveDefinitionFromSource(SourceObject, Definition, &Error))
	{
		UE_LOG(
			LogKCAbilitySystem,
			Warning,
			TEXT("Ability 부여를 거부했습니다. Source='%s', 이유: %s"),
			*GetNameSafe(SourceObject),
			*Error);
		return FGameplayAbilitySpecHandle();
	}

	return GrantAbilityDefinition(Definition, SourceObject, InputId);
}

/**
 * @brief Grants an ability defined by the specified definition to the source object.
 *
 * Reuses an existing matching grant and rejects conflicting abilities associated with the same source object.
 *
 * @param Definition Ability definition to grant.
 * @param SourceObject Object associated with the granted ability.
 * @param InputId Input identifier assigned to the ability.
 * @return Handle for the granted or existing ability, or an invalid handle if the grant is rejected.
 */
FGameplayAbilitySpecHandle UKCAbilitySystemComponent::GrantAbilityDefinition(
	const UKCAbilityDefinition* Definition,
	UObject* SourceObject,
	int32 InputId)
{
	if (!IsOwnerActorAuthoritative() || !IsValid(Definition) ||
		!IsValid(SourceObject))
	{
		return FGameplayAbilitySpecHandle();
	}

	const UKCAbilityDefinition* SourceDefinition = nullptr;
	FString Error;
	if (!ResolveDefinitionFromSource(SourceObject, SourceDefinition, &Error) ||
		Definition != SourceDefinition)
	{
		UE_LOG(
			LogKCAbilitySystem,
			Warning,
			TEXT("Source에서 재구성한 Definition과 요청 Definition이 달라 부여를 거부했습니다. Source='%s', 이유: %s"),
			*GetNameSafe(SourceObject),
			*Error);
		return FGameplayAbilitySpecHandle();
	}

	for (const FGameplayAbilitySpec& ExistingSpec : GetActivatableAbilities())
	{
		if (ExistingSpec.SourceObject.Get() != SourceObject)
		{
			continue;
		}

		const bool bSameGrant =
			ExistingSpec.Ability &&
			ExistingSpec.Ability->GetClass() == Definition->ActionClass &&
			ExistingSpec.Level == Definition->AbilityLevel &&
			ExistingSpec.InputID == InputId;
		if (bSameGrant)
		{
			return ExistingSpec.Handle;
		}

		UE_LOG(
			LogKCAbilitySystem,
			Warning,
			TEXT("같은 SourceObject에 서로 다른 Ability가 이미 부여되어 있습니다. Source='%s'"),
			*GetNameSafe(SourceObject));
		return FGameplayAbilitySpecHandle();
	}

	return GiveAbility(FGameplayAbilitySpec(
		Definition->ActionClass,
		Definition->AbilityLevel,
		InputId,
		SourceObject));
}

/**
 * @brief Attempts to activate a granted ability by handle.
 *
 * @param AbilityHandle Handle of the granted ability to activate.
 * @return true if the handle is valid and activation succeeds, false otherwise.
 */
bool UKCAbilitySystemComponent::TryActivateGrantedAbility(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	return AbilityHandle.IsValid() && TryActivateAbility(AbilityHandle);
}

/**
 * @brief Attempts to activate a granted ability using gameplay event data.
 *
 * @param AbilityHandle Handle of the granted ability to activate.
 * @param EventTag Tag identifying the gameplay event.
 * @param EventData Data associated with the gameplay event.
 * @return `true` if the ability activation succeeds, `false` otherwise.
 */
bool UKCAbilitySystemComponent::TryActivateGrantedAbilityWithEvent(
	FGameplayAbilitySpecHandle AbilityHandle,
	FGameplayTag EventTag,
	const FGameplayEventData& EventData)
{
	if (!AbilityHandle.IsValid() || !EventTag.IsValid() || !AbilityActorInfo.IsValid())
	{
		return false;
	}
	
	return TriggerAbilityFromGameplayEvent(
		AbilityHandle,
		AbilityActorInfo.Get(),
		EventTag,
		&EventData,
		*this);
}

/**
 * @brief Revokes a granted ability identified by its handle.
 *
 * Optionally cancels the ability if it is active before removing it.
 *
 * @param AbilityHandle Handle of the ability to revoke.
 * @param bCancelActiveAbility Whether to cancel the ability before revoking it.
 * @return `true` if the ability was revoked, `false` if the owner lacks authority or the handle is invalid or unrecognized.
 */
bool UKCAbilitySystemComponent::RevokeAbilityByHandle(
	FGameplayAbilitySpecHandle AbilityHandle,
	bool bCancelActiveAbility)
{
	if (!IsOwnerActorAuthoritative() || !AbilityHandle.IsValid() ||
		!FindAbilitySpecFromHandle(AbilityHandle))
	{
		return false;
	}

	if (bCancelActiveAbility)
	{
		CancelAbilityHandle(AbilityHandle);
	}
	ClearAbility(AbilityHandle);
	return true;
}

/**
 * @brief Finds the granted ability associated with a source object.
 *
 * @param SourceObject Object associated with the granted ability.
 * @return FGameplayAbilitySpecHandle Handle of the matching ability, or an invalid handle if none is found.
 */
FGameplayAbilitySpecHandle UKCAbilitySystemComponent::FindGrantedAbilityBySource(
	const UObject* SourceObject) const
{
	if (!IsValid(SourceObject))
	{
		return FGameplayAbilitySpecHandle();
	}

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.SourceObject.Get() == SourceObject)
		{
			return Spec.Handle;
		}
	}

	return FGameplayAbilitySpecHandle();
}

/**
 * @brief Requests montage playback for a remote owner.
 *
 * @param Montage Montage to play.
 * @param PlayRate Playback rate.
 * @param StartSection Montage section at which playback begins.
 */
void UKCAbilitySystemComponent::PlayActionMontageForRemoteOwner(
	UAnimMontage* Montage,
	float PlayRate,
	FName StartSection)
{
	if (!IsValid(Montage) || !IsRemoteOwnerMontageTarget())
	{
		return;
	}

	ClientPlayActionMontage(Montage, PlayRate, StartSection);
}

/**
 * @brief Stops an action montage for a remote owner.
 *
 * @param Montage Montage to stop.
 */
void UKCAbilitySystemComponent::StopActionMontageForRemoteOwner(
	UAnimMontage* Montage)
{
	if (!IsValid(Montage) || !IsRemoteOwnerMontageTarget())
	{
		return;
	}

	ClientStopActionMontage(Montage);
}

/**
 * @brief Plays an action montage for the locally controlled owner.
 *
 * @param Montage Montage to play.
 * @param PlayRate Playback rate for the montage.
 * @param StartSection Section at which playback begins.
 */
void UKCAbilitySystemComponent::ClientPlayActionMontage_Implementation(
	UAnimMontage* Montage,
	float PlayRate,
	FName StartSection)
{
	if (!IsValid(Montage) || !AbilityActorInfo.IsValid() ||
		!AbilityActorInfo->IsLocallyControlled())
	{
		return;
	}

	if (PlayMontageSimulated(Montage, PlayRate) <= 0.0f)
	{
		return;
	}

	// PlayMontageSimulated는 StartSection을 사용하지 않으므로 직접 이동한다.
	if (!StartSection.IsNone())
	{
		if (UAnimInstance* AnimInstance = AbilityActorInfo->GetAnimInstance())
		{
			AnimInstance->Montage_JumpToSection(StartSection, Montage);
		}
	}
}

/**
 * @brief Stops the specified action montage when this component is locally controlled.
 *
 * @param Montage Montage to stop if it is currently playing.
 */
void UKCAbilitySystemComponent::ClientStopActionMontage_Implementation(
	UAnimMontage* Montage)
{
	if (IsValid(Montage) && AbilityActorInfo.IsValid() &&
		AbilityActorInfo->IsLocallyControlled())
	{
		StopMontageIfCurrent(*Montage);
	}
}

/**
 * @brief Determines whether the owner is a remote target for montage playback.
 *
 * @return `true` if the owner is authoritative and not locally controlled, `false` otherwise.
 */
bool UKCAbilitySystemComponent::IsRemoteOwnerMontageTarget() const
{
	// 리슨 서버 호스트는 서버에서 이미 재생하므로 중복 재생을 막는다.
	return IsOwnerActorAuthoritative() && AbilityActorInfo.IsValid() &&
		!AbilityActorInfo->IsLocallyControlled();
}
