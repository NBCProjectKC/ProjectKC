#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "KCGA_PlayerDash.generated.h"

class UAbilityTask_ApplyRootMotionConstantForce;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/** Stamina와 Cooldown은 GAS로, 이동 예측과 충돌은 CharacterMovement로 처리하는 플레이어 대시다. */
UCLASS(meta = (DisplayName = "KCGA_PlayerDash"))
class PROJECTKC_API UKCGA_PlayerDash : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UKCGA_PlayerDash();
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	UAnimMontage* GetDashMontage() const;
	float GetDashMontagePlayRate() const;

protected:
	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	UFUNCTION()
	void HandleDashFinished();

	UFUNCTION()
	void HandleDashInterrupted();

	void FinishDash(bool bWasCancelled);
	void StartDashMontage();

	UPROPERTY(EditDefaultsOnly, Category = "KC|Dash", meta = (ClampMin = "1.0"))
	float DashDistance = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Dash", meta = (ClampMin = "0.01"))
	float DashDuration = 0.16f;

	/** 이동은 RootMotion Force가 담당하므로 Root Motion이 없는 몽타주를 사용한다. */
	UPROPERTY(EditDefaultsOnly, Category = "KC|Dash|Animation")
	TObjectPtr<UAnimMontage> DashMontage;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Dash|Animation",
		meta = (ClampMin = "0.01"))
	float DashMontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Dash|Animation")
	FName DashMontageStartSection = NAME_None;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_ApplyRootMotionConstantForce> ActiveDashTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveDashMontageTask;

	bool bIsEndingDash = false;
};
