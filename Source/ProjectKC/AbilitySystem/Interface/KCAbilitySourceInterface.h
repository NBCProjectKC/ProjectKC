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

/**
 * Resolves the ability definition provided by this source.
 * @param OutDefinition Receives the resolved immutable ability definition.
 * @returns `true` if the definition was resolved, `false` otherwise.
 */
class PROJECTKC_API IKCAbilitySourceInterface
{
	GENERATED_BODY()

public:
	virtual bool ResolveAbilityDefinition(
		const UKCAbilityDefinition*& OutDefinition) const = 0;
};
