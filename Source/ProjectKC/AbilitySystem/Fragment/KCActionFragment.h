#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "KCActionFragment.generated.h"

struct FKCActionExecutionContext;

/** Ability Action Hook에 인라인으로 조립되는 결과 기능의 기반 클래스다. */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced)
class /**
 * Base class for composing ability action behavior from reusable fragments.
 *
 * Fragments validate their configuration, declare set-by-caller tags, determine
 * whether they can execute, and perform their action when executed.
 */
PROJECTKC_API UKCActionFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const;
	virtual bool DeclaresSetByCallerTag(FGameplayTag DataTag) const;
	virtual void AppendDeclaredSetByCallerTags(
		FGameplayTagContainer& OutTags) const;
	virtual bool CanExecute(
		const FKCActionExecutionContext& Context,
		FString& OutError) const;
	virtual bool Execute(const FKCActionExecutionContext& Context) const
		PURE_VIRTUAL(UKCActionFragment::Execute, return false;);

	/** false면 실행 조건이 맞지 않을 때 이 Fragment만 건너뛴다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Ability|Action")
	bool bRequired = true;
};
