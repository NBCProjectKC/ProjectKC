#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "KCSingleActionDefinition.generated.h"

/** Press 한 번에 한 번 실행하고 종료하는 Action Definition이다. */
UCLASS(
	BlueprintType,
	EditInlineNew,
	DefaultToInstanced,
	meta = (DisplayName = "Single Action"))
class PROJECTKC_API UKCSingleActionDefinition : public UKCAbilityDefinition
{
	GENERATED_BODY()

public:
	virtual TSubclassOf<UKCGA_Base> GetAbilityClass() const override;
};
