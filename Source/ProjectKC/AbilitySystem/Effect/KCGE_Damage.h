#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "KCGE_Damage.generated.h"

/** signed Data.Damage.Flat 값을 Health에 즉시 더하는 공용 네이티브 GE다. */
UCLASS(meta = (DisplayName = "KCGE_Damage"))
class PROJECTKC_API UKCGE_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UKCGE_Damage();
};
