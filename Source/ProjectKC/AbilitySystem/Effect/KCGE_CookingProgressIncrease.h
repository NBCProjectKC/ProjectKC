#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "KCGE_CookingProgressIncrease.generated.h"

/** 조리 진행도를 즉시 증가시키는 GE다. */
UCLASS(meta = (DisplayName = "KCGE_CookingProgressIncrease"))
class PROJECTKC_API UKCGE_CookingProgressIncrease : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UKCGE_CookingProgressIncrease();
};
