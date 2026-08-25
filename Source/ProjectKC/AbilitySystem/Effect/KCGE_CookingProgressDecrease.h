#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "KCGE_CookingProgressDecrease.generated.h"

/** 조리 진행도를 즉시 감소시키는 GE다. */
UCLASS(meta = (DisplayName = "KCGE_CookingProgressDecrease"))
class PROJECTKC_API UKCGE_CookingProgressDecrease : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UKCGE_CookingProgressDecrease();
};
