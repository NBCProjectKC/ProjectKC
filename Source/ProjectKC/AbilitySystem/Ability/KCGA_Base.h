#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "KCGA_Base.generated.h"

class AActor;
class UAbilitySystemComponent;
class UKCAbilityDefinition;
struct FKCActionExecutionContext;
struct FKCGameplayEffectRecipeStruct;

/** Definition의 Action Hook을 실행하는 소스 독립적인 GA 기반 클래스다. */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "KCGA_Base"))
class PROJECTKC_API UKCGA_Base : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UKCGA_Base();

	bool ValidateDefinitionContract(
		const UKCAbilityDefinition& Definition,
		FString& OutError) const;

	/** ApplyGameplayEffect Fragment가 사용하는 공용 GE 적용 진입점이다. */
	bool ApplyGameplayEffectRecipe(
		const FKCGameplayEffectRecipeStruct& Recipe,
		const FKCActionExecutionContext& Context,
		UAbilitySystemComponent* TargetAbilitySystem,
		bool bTrackUntilAbilityEnds);

protected:
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/**
	 * Definition이 정한 쿨다운 태그를 소유자가 이미 들고 있으면 활성화를 막는다.
	 *
	 * GA 클래스 하나를 모든 아이템이 공유하므로 CDO의 CooldownGameplayEffectClass에
	 * 고정하지 않고 매번 Spec의 SourceObject에서 Definition을 해석한다.
	 */
	virtual bool CheckCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** CommitAbility 시점에 서버가 쿨다운 GE를 소유자에게 적용한다. */
	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

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

	void AddSupportedActionHook(FGameplayTag HookTag);
	void AddRequiredActionHook(FGameplayTag HookTag);


	bool ExecuteActionHook(
		FGameplayTag HookTag,
		UAbilitySystemComponent* TargetAbilitySystem,
		AActor* TargetActor,
		const FHitResult* HitResult = nullptr);

	const UKCAbilityDefinition* GetActiveDefinition() const;
	void SetExecutionChargeAlpha(float ChargeAlpha);
	float GetExecutionChargeAlpha() const;

private:
	struct FKCTrackedActiveEffect
	{
		TWeakObjectPtr<UAbilitySystemComponent> TargetAbilitySystem;
		FActiveGameplayEffectHandle EffectHandle;
	};

	bool ResolveDefinitionForSpec(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const UKCAbilityDefinition*& OutDefinition,
		FString* OutError = nullptr) const;

	FGameplayEffectSpecHandle MakeEffectSpec(
		const FKCGameplayEffectRecipeStruct& Recipe) const;

	UPROPERTY(Transient)
	TObjectPtr<UKCAbilityDefinition> ActiveDefinition;

	TArray<FKCTrackedActiveEffect> TrackedActiveEffects;
	FGameplayTagContainer SupportedActionHooks;
	FGameplayTagContainer RequiredActionHooks;
	float ExecutionChargeAlpha = 1.0f;
};
