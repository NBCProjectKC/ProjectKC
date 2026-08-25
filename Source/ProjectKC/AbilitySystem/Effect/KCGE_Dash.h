#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "KCGE_Dash.generated.h"

/** 예측 가능한 플레이어 대시의 고정 Stamina 비용이다. */
UCLASS(meta = (DisplayName = "KCGE_DashCost"))
class PROJECTKC_API UKCGE_DashCost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UKCGE_DashCost();
};

/** 대시 재사용 대기시간을 제공한다. Cooldown 태그는 Ability가 예측 스펙에 추가한다. */
UCLASS(meta = (DisplayName = "KCGE_DashCooldown"))
class PROJECTKC_API UKCGE_DashCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UKCGE_DashCooldown();
};
