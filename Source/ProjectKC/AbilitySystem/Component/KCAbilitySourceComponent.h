#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "ProjectKC/AbilitySystem/Interface/KCAbilitySourceInterface.h"
#include "KCAbilitySourceComponent.generated.h"

class AActor;
class UKCAbilityDefinition;
class UKCAbilitySystemComponent;

/** Handle과 대상 ASC Owner를 한 번에 복제하는 Source Binding 상태다. */
USTRUCT()
struct PROJECTKC_API FKCAbilitySourceBindingState
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayAbilitySpecHandle AbilityHandle;

	UPROPERTY()
	TObjectPtr<AActor> AbilitySystemOwner;

	void Reset()
	{
		AbilityHandle = FGameplayAbilitySpecHandle();
		AbilitySystemOwner = nullptr;
	}
};

/** 아이템·함정 등 Actor에 붙여 동일한 Ability Source 수명주기를 제공한다. */
UCLASS(BlueprintType, ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCAbilitySourceComponent
	: public UActorComponent
	, public IKCAbilitySourceInterface
{
	GENERATED_BODY()

public:
	UKCAbilitySourceComponent();

	virtual bool ResolveAbilityDefinition(
		const UKCAbilityDefinition*& OutDefinition) const override;

	/** Grant 중에는 Definition을 바꿀 수 없다. */
	bool ConfigureAbilityDefinition(UKCAbilityDefinition* NewDefinition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Ability")
	bool GrantToAbilitySystem(UKCAbilitySystemComponent* AbilitySystem);

	UFUNCTION(BlueprintCallable, Category = "KC|Ability")
	bool TryActivate();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Ability")
	bool TryActivateWithTarget(AActor* TargetActor);

	bool TryActivateWithHitResult(
		AActor* TargetActor,
		const FHitResult& HitResult);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Ability")
	bool Revoke(bool bCancelActiveAbility = true);

	UFUNCTION(BlueprintPure, Category = "KC|Ability")
	FGameplayAbilitySpecHandle GetGrantedAbilityHandle() const;

	UFUNCTION(BlueprintPure, Category = "KC|Ability")
	bool HasAbilityDefinition() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	bool TryActivateWithTargetInternal(
		AActor* TargetActor,
		const FHitResult* HitResult);
	UKCAbilitySystemComponent* GetGrantedAbilitySystem() const;

	UPROPERTY(Transient)
	TObjectPtr<UKCAbilityDefinition> AbilityDefinition;

	UPROPERTY(Replicated)
	FKCAbilitySourceBindingState BindingState;

	UPROPERTY(Transient)
	TObjectPtr<UKCAbilitySystemComponent> AuthorityGrantedAbilitySystem;
};
