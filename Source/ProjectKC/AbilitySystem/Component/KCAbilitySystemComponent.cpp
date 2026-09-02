#include "ProjectKC/AbilitySystem/Component/KCAbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "ProjectKC/AbilitySystem/Ability/KCGA_Base.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "ProjectKC/AbilitySystem/Definition/KCChannelActionDefinition.h"
#include "ProjectKC/AbilitySystem/Definition/KCSingleActionDefinition.h"
#include "ProjectKC/AbilitySystem/Interface/KCAbilitySourceInterface.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
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
			ExistingSpec.Ability->GetClass() == Definition->GetAbilityClass() &&
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
		Definition->GetAbilityClass(),
		Definition->AbilityLevel,
		InputId,
		SourceObject));
}

bool UKCAbilitySystemComponent::TryActivateGrantedAbility(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	return AbilityHandle.IsValid() && TryActivateAbility(AbilityHandle);
}

bool UKCAbilitySystemComponent::PressAbilityInputByHandle(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	if (!AbilityHandle.IsValid())
	{
		return false;
	}

	if (IsOwnerActorAuthoritative())
	{
		return ProcessAbilityInputPressed(AbilityHandle);
	}

	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	const FGameplayTagContainer* AbilityTags = Spec && Spec->Ability
		? &Spec->Ability->GetAssetTags()
		: nullptr;
	if (AbilityTags && AbilityTags->HasTag(TAG_KC_Ability_Attack) &&
		AreAbilityTagsBlocked(*AbilityTags))
	{
		return false;
	}

	// 이미 진행 중인 액션에 다시 들어온 Press는 여기서 버린다.
	// 서버는 활성 Spec의 재활성화를 거부하므로 이 입력은 어차피 아무것도
	// 시작하지 못한다. 그런데 예측 재생은 시작 전에 진행 중인 연출을 끊고
	// 처음부터 다시 트니, 유효한 연출만 망가뜨리고 되돌릴 방법이 없다.
	// 서버 RPC도 보내지 않아 Press와 Release 짝이 어긋나지 않게 한다.
	if (HasOutstandingLocalAction(AbilityHandle))
	{
		return false;
	}

	const uint32 ActionRequestId =
		BeginLocalActionMontagePrediction(AbilityHandle);
	ServerPressAbilityInputByHandle(AbilityHandle, ActionRequestId);
	return true;
}

bool UKCAbilitySystemComponent::ReleaseAbilityInputByHandle(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	if (!AbilityHandle.IsValid())
	{
		return false;
	}

	if (IsOwnerActorAuthoritative())
	{
		return ProcessAbilityInputReleased(AbilityHandle);
	}

	if (LocalActionAbilityHandle == AbilityHandle)
	{
		bLocalActionInputReleased = true;
		if (bLocalActionStopOnRelease)
		{
			// Channel은 서버 왕복을 기다리지 않고 소유자 연출부터 멈춘다.
			StopLocalActionMontagePrediction(false);
		}
	}

	ServerReleaseAbilityInputByHandle(AbilityHandle);
	return true;
}

void UKCAbilitySystemComponent::ServerPressAbilityInputByHandle_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle,
	uint32 ActionRequestId)
{
	PendingServerActionRequests.Add(AbilityHandle, ActionRequestId);
	const bool bProcessed = ProcessAbilityInputPressed(AbilityHandle);
	PendingServerActionRequests.Remove(AbilityHandle);

	const uint32* ActiveRequestId =
		ActiveServerActionRequests.Find(AbilityHandle);
	if (!bProcessed || !ActiveRequestId ||
		*ActiveRequestId != ActionRequestId)
	{
		// 이미 활성인 Spec에 다시 들어온 Press와 활성화 거부 모두 예측 연출을 회수한다.
		ClientRejectActionMontage(AbilityHandle, ActionRequestId);
	}
}

void UKCAbilitySystemComponent::ServerReleaseAbilityInputByHandle_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	ProcessAbilityInputReleased(AbilityHandle);
}

bool UKCAbilitySystemComponent::ProcessAbilityInputPressed(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	if (!IsOwnerActorAuthoritative() || !Spec)
	{
		return false;
	}

	AbilitySpecInputPressed(*Spec);
	return Spec->IsActive() || TryActivateAbility(AbilityHandle);
}

bool UKCAbilitySystemComponent::ProcessAbilityInputReleased(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	if (!IsOwnerActorAuthoritative() || !Spec)
	{
		return false;
	}

	AbilitySpecInputReleased(*Spec);
	return true;
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
	FGameplayAbilitySpecHandle AbilityHandle,
	UAnimMontage* Montage,
	float PlayRate,
	FName StartSection)
{
	if (!AbilityHandle.IsValid() || !IsValid(Montage) ||
		!IsRemoteOwnerMontageTarget())
	{
		return;
	}

	const uint32* PendingRequestId =
		PendingServerActionRequests.Find(AbilityHandle);
	const uint32 ActionRequestId = PendingRequestId ? *PendingRequestId : 0;
	ActiveServerActionRequests.Add(AbilityHandle, ActionRequestId);
	ClientPlayActionMontage(
		AbilityHandle,
		ActionRequestId,
		Montage,
		PlayRate,
		StartSection);
}

void UKCAbilitySystemComponent::StopActionMontageForRemoteOwner(
	FGameplayAbilitySpecHandle AbilityHandle,
	UAnimMontage* Montage)
{
	if (!AbilityHandle.IsValid() || !IsValid(Montage) ||
		!IsRemoteOwnerMontageTarget())
	{
		return;
	}

	const uint32* ActiveRequestId =
		ActiveServerActionRequests.Find(AbilityHandle);
	const uint32 ActionRequestId = ActiveRequestId ? *ActiveRequestId : 0;
	ClientStopActionMontage(AbilityHandle, ActionRequestId, Montage);
	ActiveServerActionRequests.Remove(AbilityHandle);
}

void UKCAbilitySystemComponent::ClientPlayActionMontage_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle,
	uint32 ActionRequestId,
	UAnimMontage* Montage,
	float PlayRate,
	FName StartSection)
{
	if (!IsValid(Montage) || !AbilityActorInfo.IsValid() ||
		!AbilityActorInfo->IsLocallyControlled())
	{
		return;
	}

	if (MatchesLocalActionRequest(AbilityHandle, ActionRequestId))
	{
		const bool bSamePlayback =
			LocalActionMontage.Get() == Montage &&
			FMath::IsNearlyEqual(LocalActionPlayRate, PlayRate) &&
			LocalActionStartSection == StartSection;
		if (bSamePlayback && bLocalActionMontagePlayed)
		{
			// 로컬 선재생이 자연 종료됐더라도 서버 승인 시 다시 시작하지 않는다.
			return;
		}

		StopLocalActionMontagePrediction(false);
		LocalActionMontage = Montage;
		LocalActionPlayRate = PlayRate;
		LocalActionStartSection = StartSection;
		if (bLocalActionInputReleased && bLocalActionStopOnRelease)
		{
			return;
		}

		bLocalActionMontagePlayed =
			PlayActionMontageLocally(Montage, PlayRate, StartSection);
		return;
	}

	if (ActionRequestId != 0 && LocalActionRequestId != 0 &&
		static_cast<int32>(ActionRequestId - LocalActionRequestId) < 0)
	{
		// 이전 Press의 늦은 승인이 더 최신 로컬 연출을 덮어쓰지 않는다.
		return;
	}

	StopLocalActionMontagePrediction(true);
	LocalActionAbilityHandle = AbilityHandle;
	LocalActionRequestId = ActionRequestId;
	LocalActionMontage = Montage;
	LocalActionPlayRate = PlayRate;
	LocalActionStartSection = StartSection;
	UAnimMontage* ResolvedMontage = nullptr;
	float ResolvedPlayRate = 1.0f;
	FName ResolvedStartSection = NAME_None;
	ResolveLocalActionMontage(
		AbilityHandle,
		ResolvedMontage,
		ResolvedPlayRate,
		ResolvedStartSection,
		bLocalActionStopOnRelease);
	bLocalActionMontagePlayed =
		PlayActionMontageLocally(Montage, PlayRate, StartSection);
}

void UKCAbilitySystemComponent::ClientStopActionMontage_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle,
	uint32 ActionRequestId,
	UAnimMontage* Montage)
{
	if (!IsValid(Montage) || !AbilityActorInfo.IsValid() ||
		!AbilityActorInfo->IsLocallyControlled())
	{
		return;
	}

	if (ActionRequestId != 0 &&
		!MatchesLocalActionRequest(AbilityHandle, ActionRequestId))
	{
		return;
	}

	if (ActionRequestId == 0 && LocalActionRequestId != 0)
	{
		return;
	}

	StopMontageIfCurrent(*Montage);
	ResetLocalActionMontagePrediction();
}

void UKCAbilitySystemComponent::ClientRejectActionMontage_Implementation(
	FGameplayAbilitySpecHandle AbilityHandle,
	uint32 ActionRequestId)
{
	if (MatchesLocalActionRequest(AbilityHandle, ActionRequestId))
	{
		StopLocalActionMontagePrediction(true);
	}
}

bool UKCAbilitySystemComponent::IsRemoteOwnerMontageTarget() const
{
	// 리슨 서버 호스트는 서버에서 이미 재생하므로 중복 재생을 막는다.
	return IsOwnerActorAuthoritative() && AbilityActorInfo.IsValid() &&
		!AbilityActorInfo->IsLocallyControlled();
}

uint32 UKCAbilitySystemComponent::BeginLocalActionMontagePrediction(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	do
	{
		++LastLocalActionRequestId;
	}
	while (LastLocalActionRequestId == 0);

	StopLocalActionMontagePrediction(true);
	LocalActionAbilityHandle = AbilityHandle;
	LocalActionRequestId = LastLocalActionRequestId;

	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	const UKCAbilityDefinition* Definition = nullptr;
	if (Spec && GetDefinitionFromSource(
		Spec->SourceObject.Get(), Definition))
	{
		const UKCSingleActionDefinition* SingleAction =
			Cast<UKCSingleActionDefinition>(Definition);
		if (SingleAction && SingleAction->ExecutesOnInputRelease())
		{
			// 충전 중에는 로컬 몽타주를 선재생하지 않는다. 서버가 Release를
			// 승인한 뒤 실제 실행 시점에 ClientPlayActionMontage로 시작한다.
			return LocalActionRequestId;
		}
	}

	UAnimMontage* Montage = nullptr;
	if (!ResolveLocalActionMontage(
		AbilityHandle,
		Montage,
		LocalActionPlayRate,
		LocalActionStartSection,
		bLocalActionStopOnRelease) ||
		!IsValid(Montage))
	{
		return LocalActionRequestId;
	}

	LocalActionMontage = Montage;
	bLocalActionMontagePlayed = PlayActionMontageLocally(
		Montage,
		LocalActionPlayRate,
		LocalActionStartSection);
	return LocalActionRequestId;
}

bool UKCAbilitySystemComponent::PlayActionMontageLocally(
	UAnimMontage* Montage,
	float PlayRate,
	FName StartSection)
{
	if (!IsValid(Montage) || !AbilityActorInfo.IsValid() ||
		!AbilityActorInfo->IsLocallyControlled() ||
		PlayMontageSimulated(Montage, PlayRate) <= 0.0f)
	{
		return false;
	}

	// PlayMontageSimulated는 StartSection을 사용하지 않으므로 직접 이동한다.
	if (!StartSection.IsNone())
	{
		if (UAnimInstance* AnimInstance = AbilityActorInfo->GetAnimInstance())
		{
			AnimInstance->Montage_JumpToSection(StartSection, Montage);
		}
	}
	return true;
}

void UKCAbilitySystemComponent::StopLocalActionMontagePrediction(
	bool bResetState)
{
	if (UAnimMontage* Montage = LocalActionMontage.Get())
	{
		StopMontageIfCurrent(*Montage);
	}
	bLocalActionMontagePlayed = false;

	if (bResetState)
	{
		ResetLocalActionMontagePrediction();
	}
}

void UKCAbilitySystemComponent::ResetLocalActionMontagePrediction()
{
	LocalActionAbilityHandle = FGameplayAbilitySpecHandle();
	LocalActionRequestId = 0;
	LocalActionMontage = nullptr;
	LocalActionPlayRate = 1.0f;
	LocalActionStartSection = NAME_None;
	bLocalActionStopOnRelease = false;
	bLocalActionInputReleased = false;
	bLocalActionMontagePlayed = false;
}

bool UKCAbilitySystemComponent::HasOutstandingLocalAction(
	FGameplayAbilitySpecHandle AbilityHandle)
{
	// 서버가 확정한 활성 상태다. 한 왕복 늦게 도착하지만,
	// 로컬 연출이 먼저 끝나고 Ability는 아직 살아 있는 구간을 메운다.
	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	if (Spec && Spec->IsActive())
	{
		return true;
	}

	if (LocalActionRequestId == 0 || LocalActionAbilityHandle != AbilityHandle)
	{
		return false;
	}

	// 아직 서버 응답 전이면 예측 연출이 살아 있는지로 판단한다.
	// 연출이 이미 끝났다면 응답이 유실돼도 다음 입력을 영구히 막지 않는다.
	const UAnimMontage* PredictedMontage = LocalActionMontage.Get();
	const UAnimInstance* AnimInstance = AbilityActorInfo.IsValid()
		? AbilityActorInfo->GetAnimInstance()
		: nullptr;
	return bLocalActionMontagePlayed && IsValid(PredictedMontage) &&
		AnimInstance && AnimInstance->Montage_IsPlaying(PredictedMontage);
}

bool UKCAbilitySystemComponent::MatchesLocalActionRequest(
	FGameplayAbilitySpecHandle AbilityHandle,
	uint32 ActionRequestId) const
{
	return LocalActionAbilityHandle == AbilityHandle &&
		LocalActionRequestId == ActionRequestId;
}

bool UKCAbilitySystemComponent::ResolveLocalActionMontage(
	FGameplayAbilitySpecHandle AbilityHandle,
	UAnimMontage*& OutMontage,
	float& OutPlayRate,
	FName& OutStartSection,
	bool& bOutStopOnRelease) const
{
	OutMontage = nullptr;
	OutPlayRate = 1.0f;
	OutStartSection = NAME_None;
	bOutStopOnRelease = false;

	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilityHandle);
	const UKCAbilityDefinition* Definition = nullptr;
	if (!Spec || !GetDefinitionFromSource(
		Spec->SourceObject.Get(), Definition) || !Definition)
	{
		return false;
	}

	OutMontage = Definition->ActionMontage.Montage;
	OutPlayRate = Definition->ActionMontage.PlayRate;
	OutStartSection = Definition->ActionMontage.StartSection;
	bOutStopOnRelease = Definition->IsA<UKCChannelActionDefinition>();
	return IsValid(OutMontage);
}
