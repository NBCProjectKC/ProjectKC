#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "KCGE_StaminaRegen.generated.h"

/** 서버에서 지속 적용되어 Stamina를 자연 회복시키는 주기형 GameplayEffect다. */
UCLASS(meta = (DisplayName = "KCGE_StaminaRegen"))
class PROJECTKC_API UKCGE_StaminaRegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UKCGE_StaminaRegen();
};
