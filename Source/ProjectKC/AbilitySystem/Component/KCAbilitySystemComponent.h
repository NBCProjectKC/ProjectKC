#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "KCAbilitySystemComponent.generated.h"

struct FGameplayEventData;
class UAnimMontage;
class UKCAbilityDefinition;

/** 소스 독립적인 Ability Binding을 정확한 SpecHandle 단위로 관리한다. */
UCLASS(BlueprintType, Blueprintable, ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** Grant 시점 전용이다. Definition 전체를 재귀 검증하므로 비용이 크다. */
	static bool ResolveDefinitionFromSource(
		const UObject* SourceObject,
		const UKCAbilityDefinition*& OutDefinition,
		FString* OutError = nullptr);

	/**
	 * 검증 없이 소스가 제공하는 Definition만 가져온다.
	 * Definition은 Grant 시점에 이미 검증됐고 Grant 중에는 교체할 수 없으므로,
	 * 매 활성화마다 재검증하지 않는다.
	 */
	static bool GetDefinitionFromSource(
		const UObject* SourceObject,
		const UKCAbilityDefinition*& OutDefinition,
		FString* OutError = nullptr);

	UFUNCTION(BlueprintCallable, Category = "KC|Ability")
	FGameplayAbilitySpecHandle GrantAbilityFromSource(
		UObject* SourceObject,
		int32 InputId = -1);

	FGameplayAbilitySpecHandle GrantAbilityDefinition(
		const UKCAbilityDefinition* Definition,
		UObject* SourceObject,
		int32 InputId = INDEX_NONE);

	UFUNCTION(BlueprintCallable, Category = "KC|Ability")
	bool TryActivateGrantedAbility(FGameplayAbilitySpecHandle AbilityHandle);

	bool TryActivateGrantedAbilityWithEvent(
		FGameplayAbilitySpecHandle AbilityHandle,
		FGameplayTag EventTag,
		const FGameplayEventData& EventData);

	UFUNCTION(BlueprintCallable, Category = "KC|Ability")
	bool RevokeAbilityByHandle(
		FGameplayAbilitySpecHandle AbilityHandle,
		bool bCancelActiveAbility = true);

	FGameplayAbilitySpecHandle FindGrantedAbilityBySource(
		const UObject* SourceObject) const;

	/**
	 * 조종 중인 원격 클라이언트에게 연출용 Action Montage를 보낸다.
	 * ASC의 몽타주 복제는 OnRep_ReplicatedAnimMontage에서 자기 자신을 건너뛰므로,
	 * ServerOnly Ability로 실행한 사용 동작을 정작 사용한 본인이 볼 수 없다.
	 * 판정은 서버가 단독으로 확정하고 이 경로는 표현만 담당한다.
	 */
	void PlayActionMontageForRemoteOwner(
		UAnimMontage* Montage,
		float PlayRate,
		FName StartSection);

	void StopActionMontageForRemoteOwner(UAnimMontage* Montage);

protected:
	UFUNCTION(Client, Reliable)
	void ClientPlayActionMontage(
		UAnimMontage* Montage,
		float PlayRate,
		FName StartSection);

	UFUNCTION(Client, Reliable)
	void ClientStopActionMontage(UAnimMontage* Montage);

private:
	bool IsRemoteOwnerMontageTarget() const;
};
