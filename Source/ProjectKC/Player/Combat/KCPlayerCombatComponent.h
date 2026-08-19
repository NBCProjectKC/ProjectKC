#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KCPlayerCombatComponent.generated.h"

class AKCPlayerCharacter;
class APawn;
class UAnimSequence;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKCPlayerAttackStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FKCPlayerHitReceivedSignature,
	APawn*, AttackerPawn,
	FVector, KnockbackVelocity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKCRequestDropHeldItemSignature);

UCLASS(ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCPlayerCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKCPlayerCombatComponent();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void TryAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat|Animation")
	void HandleAttackHitNotify();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Combat|Integration")
	void SetWeaponAttackEnabled(bool bEnabled);

	float HandleDamage(
		float DamageAmount,
		AController* EventInstigator,
		AActor* DamageCauser);

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAttacking() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Integration")
	bool IsWeaponAttackEnabled() const;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FKCPlayerAttackStartedSignature OnAttackStarted;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FKCPlayerHitReceivedSignature OnHitReceived;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Integration")
	FKCRequestDropHeldItemSignature OnRequestDropHeldItem;

private:
	UFUNCTION(Server, Reliable)
	void ServerTryAttack();

	UFUNCTION()
	void OnRep_IsAttacking();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayHitFeedback(APawn* AttackerPawn, FVector KnockbackVelocity);

	void PerformAttack();
	void FinishAttack();
	void TraceAttack(AKCPlayerCharacter& OwnerCharacter);
	void PlayAttackAnimation();
	void PlayHitAnimation();

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack", meta = (ClampMin = "0.0"))
	float AttackRange = 160.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float AttackHalfAngleDegrees = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack", meta = (ClampMin = "0.0"))
	float AttackCooldown = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Attack", meta = (ClampMin = "0.0"))
	float AttackStateDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Knockback", meta = (ClampMin = "0.0"))
	float HorizontalKnockbackStrength = 1200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Knockback", meta = (ClampMin = "0.0"))
	float VerticalKnockbackStrength = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimSequence> AttackAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	TObjectPtr<UAnimSequence> HitAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	FName AttackAnimationSlotName = TEXT("UpperBody");

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
	FName HitAnimationSlotName = TEXT("DefaultSlot");

	UPROPERTY(EditDefaultsOnly, Replicated, BlueprintReadOnly, Category = "Combat|Integration",
		meta = (AllowPrivateAccess = "true"))
	bool bWeaponAttackEnabled = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsAttacking)
	bool bIsAttacking = false;

	bool bAttackHitConsumed = false;

	float LastAttackServerTime = -BIG_NUMBER;
	FTimerHandle AttackStateTimerHandle;
};
