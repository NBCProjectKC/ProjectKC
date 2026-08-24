#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KCActionTargeting.generated.h"

class AActor;

/** 한 번의 행동이 관여할 대상 하나다. */
USTRUCT()
struct PROJECTKC_API FKCActionTarget
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> Actor;

	FHitResult HitResult;
	bool bHasHitResult = false;
};

/** 대상 수집에 필요한 최소 입력이다. GA 구현에 의존하지 않는다. */
struct PROJECTKC_API FKCActionTargetingContext
{
	AActor* SourceActor = nullptr;
	const UObject* SourceObject = nullptr;

	/** 활성화 이벤트가 지정한 대상. Event 방식에서만 채워진다. */
	AActor* ActivationTarget = nullptr;
	FHitResult ActivationHitResult;
	bool bHasActivationHitResult = false;
};

/**
 * 행동이 "누구를" 대상으로 삼는지 정하는 방식이다.
 * 언제(ActionTiming), 무엇을(Fragment)과 독립적인 축이다.
 *
 * Definition에 인라인으로 담기는 불변 데이터이므로 실행 상태를 갖지 않는다.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTKC_API UKCActionTargeting : public UObject
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const;

	/** 활성화가 이벤트로 대상을 넘겨줘야 하는 방식인지 알려 준다. */
	virtual bool RequiresActivationTarget() const { return false; }

	/** 대상이 없을 수도 있다. 빗나간 행동도 정상 실행이다. */
	virtual void GatherTargets(
		const FKCActionTargetingContext& Context,
		TArray<FKCActionTarget>& OutTargets) const
		PURE_VIRTUAL(UKCActionTargeting::GatherTargets, );
};
