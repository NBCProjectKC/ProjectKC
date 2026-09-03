#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Definition/KCAbilityDefinition.h"
#include "KCSingleActionDefinition.generated.h"

class UKCThrowProjectileFragment;

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

	/** 충전 투척 Fragment가 있으면 Press가 아니라 Release에서 실행한다. */
	bool ExecutesOnInputRelease() const;
	const UKCThrowProjectileFragment* FindChargedThrowProjectileFragment() const;

protected:
	virtual bool ValidateLifecycle(FString& OutError) const override;
};
