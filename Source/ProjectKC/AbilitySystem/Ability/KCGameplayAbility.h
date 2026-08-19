#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "KCGameplayAbility.generated.h"

class AActor;
class UAbilitySystemComponent;
class UKCActionConfig;
class UKCAbilityDefinition;
struct FKCActionExecutionContext;
struct FKCGameplayEffectRecipeStruct;

/** Definition의 Action Hook을 실행하는 소스 독립적인 GA 기반 클래스다. */
UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UKCGameplayAbility();

	bool ValidateDefinitionContract(
		const UKCAbilityDefinition& Definition,
		FString& OutError) const;

	/** ApplyGameplayEffect Fragment가 사용하는 공용 GE 적용 진입점이다. */
	bool ApplyGameplayEffectRecipe(
		const FKCGameplayEffectRecipeStruct& Recipe,
		const FKCActionExecutionContext& Context,
		bool bTrackUntilAbilityEnds);

protected:
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

	void AddSupportedActionHook(FGameplayTag HookTag);
	void AddRequiredActionHook(FGameplayTag HookTag);
	void SetSupportedActionConfigClass(
		TSubclassOf<UKCActionConfig> ConfigClass,
		bool bRequired = true);

	/** Action Montage 수명주기를 실행할 수 있는 GA만 true로 선언한다. */
	void SetSupportsActionMontage(bool bSupported);

	bool SetRuntimeMagnitude(FGameplayTag DataTag, float Magnitude);

	bool ExecuteActionHook(
		FGameplayTag HookTag,
		UAbilitySystemComponent* TargetAbilitySystem,
		AActor* TargetActor,
		const FHitResult* HitResult = nullptr);

	const UKCAbilityDefinition* GetActiveDefinition() const;
	const UKCActionConfig* GetActiveActionConfig() const;

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

	TMap<FGameplayTag, float> RuntimeMagnitudes;
	TArray<FKCTrackedActiveEffect> TrackedActiveEffects;
	FGameplayTagContainer SupportedActionHooks;
	FGameplayTagContainer RequiredActionHooks;
	TSubclassOf<UKCActionConfig> SupportedActionConfigClass;
	bool bActionConfigRequired = false;
	bool bSupportsActionMontage = false;
};
