#include "ProjectKC/AbilitySystem/Ability/KCGA_PlayerDash.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectKC/AbilitySystem/Effect/KCGE_Dash.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"
#include "ProjectKC/Player/KCPlayerCharacter.h"

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
	BlockAbilitiesWithTag.AddTag(TAG_KC_Ability_Attack);
}

UAnimMontage* UKCGA_PlayerDash::GetDashMontage() const
{
	return DashMontage;
}

float UKCGA_PlayerDash::GetDashMontagePlayRate() const
{
	return DashMontagePlayRate;
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
	bIsEndingDash = false;
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
			ERootMotionFinishVelocityMode::ClampVelocity,
			FVector::ZeroVector,
			Character->GetCharacterMovement()->GetMaxSpeed(),
			true);
	if (!ActiveDashTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveDashTask->OnFinish.AddDynamic(
		this,
		&UKCGA_PlayerDash::HandleDashMovementFinished);
	ActiveDashTask->ReadyForActivation();
	if (AKCPlayerCharacter* PlayerCharacter = Cast<AKCPlayerCharacter>(Character))
	{
		PlayerCharacter->TriggerDashCameraPunch();
	}
	ExecuteDashFeedback(DashDirection);
	StartDashMontage();
}

void UKCGA_PlayerDash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility,
	const bool bWasCancelled)
{
	if (bIsEndingDash)
	{
		return;
	}

	TGuardValue<bool> EndingDashGuard(bIsEndingDash, true);
	ActiveDashTask = nullptr;
	ActiveDashMontageTask = nullptr;
	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UKCGA_PlayerDash::StartDashMontage()
{
	if (!DashMontage || DashMontagePlayRate <= 0.0f)
	{
		return;
	}

	ActiveDashMontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			TEXT("DashMontage"),
			DashMontage,
			DashMontagePlayRate,
			DashMontageStartSection,
			true,
			0.0f);
	if (ActiveDashMontageTask)
	{
		// 몽타주는 표현 전용이다. 길이와 Blend Out이 실제 대시 거리·시간을
		// 바꾸지 않도록 Ability 종료는 이동 Task만 결정한다.
		ActiveDashMontageTask->ReadyForActivation();
	}
}

void UKCGA_PlayerDash::ExecuteDashFeedback(const FVector& DashDirection)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		return;
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Instigator = AvatarActor;
	CueParameters.EffectCauser = AvatarActor;
	CueParameters.Location = AvatarActor->GetActorLocation();
	CueParameters.Normal = DashDirection.GetSafeNormal();
	CueParameters.AbilityLevel = GetAbilityLevel();
	CueParameters.bReplicateLocationWhenUsingMinimalRepProxy = true;

	// 일회성 Cue가 대시 시작 이펙트와 소리를 함께 재생한다.
	K2_ExecuteGameplayCueWithParams(
		TAG_KC_GameplayCue_Player_Dash,
		CueParameters);
}

void UKCGA_PlayerDash::HandleDashMovementFinished()
{
	FinishDash(false);
}

void UKCGA_PlayerDash::FinishDash(const bool bWasCancelled)
{
	if (!IsActive() || bIsEndingDash)
	{
		return;
	}

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		bWasCancelled);
}
