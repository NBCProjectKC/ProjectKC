#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "KCAbilitySourceInterface.generated.h"

class UKCAbilityDefinition;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UKCAbilitySourceInterface : public UInterface
{
	GENERATED_BODY()
};

/** 아이템, 함정 등 구체 타입과 무관하게 불변 Ability Definition을 제공하는 계약이다. */
class PROJECTKC_API IKCAbilitySourceInterface
{
	GENERATED_BODY()

public:
	virtual bool ResolveAbilityDefinition(
		const UKCAbilityDefinition*& OutDefinition) const = 0;
};
