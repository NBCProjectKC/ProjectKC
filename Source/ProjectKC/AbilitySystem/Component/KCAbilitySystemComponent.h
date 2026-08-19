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
class /**
 * Manages abilities granted from source objects and supports remote-owner montage presentation.
 */

/**
 * Resolves the ability definition associated with a source object.
 * @param SourceObject Object that provides the ability definition.
 * @param OutDefinition Resolved ability definition.
 * @param OutError Optional destination for an error description.
 * @returns `true` if a definition was resolved, `false` otherwise.
 */

/**
 * Grants the ability associated with a source object.
 * @param SourceObject Object that provides the ability definition.
 * @param InputId Input identifier assigned to the granted ability.
 * @returns Handle identifying the granted ability.
 */

/**
 * Grants an ability definition with its associated source object.
 * @param Definition Ability definition to grant.
 * @param SourceObject Object associated with the granted ability.
 * @param InputId Input identifier assigned to the granted ability.
 * @returns Handle identifying the granted ability.
 */

/**
 * Attempts to activate a granted ability.
 * @param AbilityHandle Handle identifying the ability to activate.
 * @returns `true` if activation succeeds, `false` otherwise.
 */

/**
 * Attempts to activate a granted ability with gameplay event data.
 * @param AbilityHandle Handle identifying the ability to activate.
 * @param EventTag Tag identifying the gameplay event.
 * @param EventData Data supplied with the gameplay event.
 * @returns `true` if activation succeeds, `false` otherwise.
 */

/**
 * Revokes a granted ability.
 * @param AbilityHandle Handle identifying the ability to revoke.
 * @param bCancelActiveAbility Whether to cancel an active instance of the ability.
 * @returns `true` if the ability was revoked, `false` otherwise.
 */

/**
 * Finds the granted ability associated with a source object.
 * @param SourceObject Object associated with the granted ability.
 * @returns Handle identifying the granted ability, or an invalid handle if none is found.
 */

/**
 * Sends an action montage to the remote owning client for presentation.
 * @param Montage Montage to play.
 * @param PlayRate Playback rate.
 * @param StartSection Section from which playback starts.
 */

/**
 * Sends a request to the remote owning client to stop an action montage.
 * @param Montage Montage to stop.
 */
PROJECTKC_API UKCAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	static bool ResolveDefinitionFromSource(
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
