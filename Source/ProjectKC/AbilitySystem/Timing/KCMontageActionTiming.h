#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Timing/KCActionTiming.h"
#include "KCMontageActionTiming.generated.h"

class UAnimMontage;

/**
 * Avatar 몽타주의 Execute Notify가 실행 시점을 정한다.
 * 아바타가 애니메이션을 가진 소스(아이템·AI·캐릭터 내재)가 공통으로 사용한다.
 */
UCLASS(meta = (DisplayName = "Montage Timing"))
class PROJECTKC_API UKCMontageActionTiming : public UKCActionTiming
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const override;
	virtual bool ScheduleExecution(UKCGAAction& Ability) const override;
	virtual void CancelExecution(UKCGAAction& Ability) const override;

	/** 즉시 재생 안정성을 우선해 첫 버전은 하드 레퍼런스를 사용한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Playback")
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Playback",
		meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "3.0"))
	float PlayRate = 1.0f;

	/** 비어 있으면 몽타주 처음부터 재생한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Playback")
	FName StartSection = NAME_None;

	/** 버리기나 취소로 Ability가 끝날 때 남은 몽타주도 멈춘다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Playback")
	bool bStopWhenAbilityEnds = true;
};
