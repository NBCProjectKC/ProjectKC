#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "KCGE_ActionCooldown.generated.h"

/**
 * Definition이 정한 시간만큼 유지되는 공용 Action 쿨다운 GE다.
 *
 * 지속시간은 SetByCaller `Data.Cooldown.Duration`으로, 점유할 태그는 Spec의
 * DynamicGrantedTags로 받는다. 그래서 Action마다 GE 에셋을 만들지 않아도 된다.
 */
UCLASS(meta = (DisplayName = "KCGE_ActionCooldown"))
class PROJECTKC_API UKCGE_ActionCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UKCGE_ActionCooldown();
};
