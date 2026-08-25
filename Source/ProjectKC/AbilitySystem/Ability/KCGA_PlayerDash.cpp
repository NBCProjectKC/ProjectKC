#include "ProjectKC/AbilitySystem/Ability/KCGA_PlayerDash.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectKC/AbilitySystem/Effect/KCGE_Dash.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

namespace KCDashAbility
{
	constexpr float MinimumYaw = -180.0f;
	constexpr float MaximumYaw = 180.0f;
}

UKCGA_PlayerDash::UKCGA_PlayerDash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	CostGameplayEffectClass = UKCGE_DashCost::StaticClass();
	CooldownGameplayEffectClass = UKCGE_DashCooldown::StaticClass();

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_KC_Ability_Player_Dash);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(TAG_KC_State_Dashing);
}

const FGameplayTagContainer* UKCGA_PlayerDash::GetCooldownTags() const
{
	static const FGameplayTagContainer DashCooldownTags = []
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(TAG_KC_Cooldown_Ability_Dash);
		return Tags;
	}();
	return &DashCooldownTags;
}

void UKCGA_PlayerDash::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(
		Handle,
		ActorInfo,
		ActivationInfo,
		CooldownGameplayEffectClass,
		GetAbilityLevel(Handle, ActorInfo));
	if (!CooldownSpec.IsValid())
	{
		return;
	}

	CooldownSpec.Data->DynamicGrantedTags.AddTag(TAG_KC_Cooldown_Ability_Dash);
	ApplyGameplayEffectSpecToOwner(
		Handle,
		ActorInfo,
		ActivationInfo,
		CooldownSpec);
}

bool UKCGA_PlayerDash::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(
		Handle,
		ActorInfo,
		SourceTags,
		TargetTags,
		OptionalRelevantTags))
	{
		return false;
	}

	const ACharacter* Character = ActorInfo
		? Cast<ACharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	const UCharacterMovementComponent* Movement = Character
		? Character->GetCharacterMovement()
		: nullptr;
	return Movement && Movement->IsMovingOnGround();
}

void UKCGA_PlayerDash::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		TriggerEventData);

	ACharacter* Character = ActorInfo
		? Cast<ACharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	const bool bHasValidDashYaw = TriggerEventData &&
		FMath::IsFinite(TriggerEventData->EventMagnitude);
	const float DashYaw = bHasValidDashYaw
		? TriggerEventData->EventMagnitude
		: 0.0f;
	if (!Character || !bHasValidDashYaw ||
		DashYaw < KCDashAbility::MinimumYaw ||
		DashYaw > KCDashAbility::MaximumYaw ||
		DashDistance <= 0.0f || DashDuration <= 0.0f ||
		!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector DashDirection = FRotator(0.0f, DashYaw, 0.0f).Vector();
	ActiveDashTask = UAbilityTask_ApplyRootMotionConstantForce::
		ApplyRootMotionConstantForce(
			this,
			TEXT("PlayerDash"),
			DashDirection,
			DashDistance / DashDuration,
			DashDuration,
			false,
			nullptr,
			ERootMotionFinishVelocityMode::SetVelocity,
			FVector::ZeroVector,
			0.0f,
			true);
	if (!ActiveDashTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveDashTask->OnFinish.AddDynamic(
		this,
		&UKCGA_PlayerDash::HandleDashFinished);
	ActiveDashTask->ReadyForActivation();
}

void UKCGA_PlayerDash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	ActiveDashTask = nullptr;
	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UKCGA_PlayerDash::HandleDashFinished()
{
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		false);
}
