#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "KCGA_PlayerDash.generated.h"

class UAbilityTask_ApplyRootMotionConstantForce;

/** Stamina와 Cooldown은 GAS로, 이동 예측과 충돌은 CharacterMovement로 처리하는 플레이어 대시다. */
UCLASS(meta = (DisplayName = "KCGA_PlayerDash"))
class PROJECTKC_API UKCGA_PlayerDash : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UKCGA_PlayerDash();
	virtual const FGameplayTagContainer* GetCooldownTags() const override;

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

	UPROPERTY(EditDefaultsOnly, Category = "KC|Dash", meta = (ClampMin = "1.0"))
	float DashDistance = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Dash", meta = (ClampMin = "0.01"))
	float DashDuration = 0.16f;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_ApplyRootMotionConstantForce> ActiveDashTask;
};
