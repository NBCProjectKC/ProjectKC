#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KCActionTiming.generated.h"

class UKCGA_Action;

/**
 * 행동의 결과를 "언제" 실행할지 정하는 방식이다.
 * 무엇을 실행할지(ActionClass), 무슨 결과를 만들지(Fragment)와 독립적인 축이다.
 *
 * 비어 있으면 활성화 즉시 실행한다. 소스 종류와 무관하므로 아이템·AI·캐릭터
 * 내재 능력이 같은 Timing을 그대로 재사용한다.
 *
 * Definition에 인라인으로 담기는 불변 데이터이므로 실행 상태를 갖지 않는다.
 * 한 번의 활성화에 대한 상태는 전부 GA가 소유한다.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTKC_API UKCActionTiming : public UObject
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const;

	/**
	 * 실행 시점을 예약한다. 예약에 성공하면 GA는 결과를 만들지 않고 대기한다.
	 * @return 예약에 실패하면 false. GA가 사용을 취소한다.
	 */
	virtual bool ScheduleExecution(UKCGA_Action& Ability) const
		PURE_VIRTUAL(UKCActionTiming::ScheduleExecution, return false;);

	/** Ability가 취소로 끝날 때 남은 연출을 정리한다. */
	virtual void CancelExecution(UKCGA_Action& Ability) const {}
};
