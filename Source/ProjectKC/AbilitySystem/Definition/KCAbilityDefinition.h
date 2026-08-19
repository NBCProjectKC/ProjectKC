#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Struct/KCActionHookStruct.h"
#include "ProjectKC/AbilitySystem/Struct/KCActionMontageSpec.h"
#include "UObject/Object.h"
#include "KCAbilityDefinition.generated.h"

class UKCActionConfig;
class UKCGameplayAbility;

/** 소스의 상위 Definition 안에 인라인으로 조립되는 불변 Action 정의다. */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTKC_API UKCAbilityDefinition : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Ability")
	TSubclassOf<UKCGameplayAbility> ActionClass;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Ability",
		meta = (ClampMin = "1"))
	int32 AbilityLevel = 1;

	/**
	 * 사용 행동의 몽타주다. 아이템 사용은 이 몽타주의 Execute Notify 시점에 결과가 발생한다.
	 * 함정처럼 Avatar 애니메이션이 없는 소스는 비워 둔다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Ability|Presentation")
	FKCActionMontageSpec ActionMontage;

	/** 특정 Action GA에만 필요한 데이터다. 지원하지 않는 GA에서는 비어 있어야 한다. */
	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "KC|Ability|Action")
	TObjectPtr<UKCActionConfig> ActionConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Ability|Action")
	TArray<FKCActionHookStruct> ActionHooks;

	const FKCActionHookStruct* FindActionHook(FGameplayTag HookTag) const;
	bool DeclaresSetByCallerTag(FGameplayTag DataTag) const;
	bool Validate(FString& OutError) const;
	bool ValidateWithActionContract(FString& OutError) const;
};
