#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Ability/KCGAUseActionBase.h"
#include "KCGAInstantSelfAction.generated.h"

/** 자신을 실행 대상으로 삼아 Self.OnActivate Fragment를 실행한다. */
UCLASS(Blueprintable)
class PROJECTKC_API UKCGAInstantSelfAction : public UKCGAUseActionBase
{
	GENERATED_BODY()

public:
	UKCGAInstantSelfAction();

protected:
	virtual bool PrepareUseAction(
		const FGameplayEventData* TriggerEventData) override;
	virtual bool ExecuteUseAction() override;
};
