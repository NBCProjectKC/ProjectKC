#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Struct/KCActionHookStruct.h"
#include "UObject/Object.h"
#include "KCAbilityDefinition.generated.h"

class UKCActionTargeting;
class UKCActionTiming;
class UKCGA_Base;

/**
 * 소스의 상위 Definition 안에 인라인으로 조립되는 불변 Action 정의다.
 * 아이템·함정·AI·캐릭터 내재 능력이 모두 이 타입 하나를 공유하고,
 * 서로 다른 점은 인라인 축(Config / Timing / Fragment)의 조합으로 표현한다.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTKC_API UKCAbilityDefinition : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action")
	TSubclassOf<UKCGA_Base> ActionClass;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Action",
		meta = (ClampMin = "1"))
	int32 AbilityLevel = 1;

	/** 결과를 누구에게 적용할지 정한다. 대상 수집 방식이다. */
	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "Action")
	TObjectPtr<UKCActionTargeting> ActionTargeting;

	/**
	 * 결과를 언제 실행할지 정한다. 비어 있으면 활성화 즉시 실행한다.
	 * 아바타 애니메이션이 없는 소스는 비워 두므로 데이터를 갖지 않는다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "Action")
	TObjectPtr<UKCActionTiming> ActionTiming;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action")
	TArray<FKCActionHookStruct> ActionHooks;

	const FKCActionHookStruct* FindActionHook(FGameplayTag HookTag) const;
	bool DeclaresSetByCallerTag(FGameplayTag DataTag) const;
	bool Validate(FString& OutError) const;
	bool ValidateWithActionContract(FString& OutError) const;
};
