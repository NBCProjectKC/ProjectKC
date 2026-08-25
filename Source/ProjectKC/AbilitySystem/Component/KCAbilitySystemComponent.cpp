#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_Base.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Interface/KCAbilitySourceInterface.h"
#include "GameplayAbilitySpec.h"

DEFINE_LOG_CATEGORY_STATIC(LogKCAbilitySystem, Log, All);

bool UKCAbilitySystemComponent::GetDefinitionFromSource(
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

	return true;
}

bool UKCAbilitySystemComponent::ResolveDefinitionFromSource(
	const UObject* SourceObject,
	const UKCAbilityDefinition*& OutDefinition,
	FString* OutError)
{
	if (!GetDefinitionFromSource(SourceObject, OutDefinition, OutError))
	{
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

FGameplayAbilitySpecHandle UKCAbilitySystemComponent::GrantAbilityFromSource(
	UObject* SourceObject,
	int32 InputId)
{
	// 검증은 GrantAbilityDefinition에서 정확히 한 번 수행한다.
	const UKCAbilityDefinition* Definition = nullptr;
	FString Error;
	if (!GetDefinitionFromSource(SourceObject, Definition, &Error))
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

bool UKCAbilitySystemComponent::TryActivateGrantedAbility(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	return AbilityHandle.IsValid() && TryActivateAbility(AbilityHandle);
}

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

void UKCAbilitySystemComponent::StopActionMontageForRemoteOwner(
	UAnimMontage* Montage)
{
	if (!IsValid(Montage) || !IsRemoteOwnerMontageTarget())
	{
		return;
	}

	ClientStopActionMontage(Montage);
}

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

void UKCAbilitySystemComponent::ClientStopActionMontage_Implementation(
	UAnimMontage* Montage)
{
	if (IsValid(Montage) && AbilityActorInfo.IsValid() &&
		AbilityActorInfo->IsLocallyControlled())
	{
		StopMontageIfCurrent(*Montage);
	}
}

bool UKCAbilitySystemComponent::IsRemoteOwnerMontageTarget() const
{
	// 리슨 서버 호스트는 서버에서 이미 재생하므로 중복 재생을 막는다.
	return IsOwnerActorAuthoritative() && AbilityActorInfo.IsValid() &&
		!AbilityActorInfo->IsLocallyControlled();
}
