#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "KCChannelActionDefinition.generated.h"

/** Press부터 Release까지 유지되며 Execute Notify마다 결과를 만드는 Action Definition이다. */
UCLASS(
	BlueprintType,
	EditInlineNew,
	DefaultToInstanced,
	meta = (DisplayName = "Channel Action"))
class PROJECTKC_API UKCChannelActionDefinition : public UKCAbilityDefinition
{
	GENERATED_BODY()

public:
	virtual TSubclassOf<UKCGA_Base> GetAbilityClass() const override;

protected:
	virtual bool ValidateLifecycle(FString& OutError) const override;
};
