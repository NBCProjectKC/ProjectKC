#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Ability/KCGAUseActionBase.h"
#include "KCGAInstantSelfAction.generated.h"

/** 자신을 실행 대상으로 삼아 Self.OnActivate Fragment를 실행한다. */
UCLASS(Blueprintable)
class /**
 * Prepares the self-targeted action for activation.
 *
 * @param TriggerEventData Optional gameplay event data associated with activation.
 * @return `true` if the action is prepared successfully, `false` otherwise.
 */

/**
 * Executes the self-targeted activation action.
 *
 * @return `true` if the action executes successfully, `false` otherwise.
 */
PROJECTKC_API UKCGAInstantSelfAction : public UKCGAUseActionBase
{
	GENERATED_BODY()

public:
	UKCGAInstantSelfAction();

protected:
	virtual bool PrepareUseAction(
		const FGameplayEventData* TriggerEventData) override;
	virtual bool ExecuteUseAction() override;
};
