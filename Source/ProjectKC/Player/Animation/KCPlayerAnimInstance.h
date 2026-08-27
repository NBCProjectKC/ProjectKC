#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "KCPlayerAnimInstance.generated.h"

/** 플레이어 전신 애니메이션과 후처리 IK 사이의 런타임 상태를 전달한다. */
UCLASS(Transient, Blueprintable)
class PROJECTKC_API UKCPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	void SetEmoteActive(bool bNewEmoteActive);

	/** 점프 및 전신 몽타주 중에는 발 후처리 IK가 원본 포즈를 다시 꺾지 않도록 막는다. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "KC|Animation")
	bool bAllowGroundIK = true;

private:
	void RefreshPostProcessIKState();

	bool bEmoteActive = false;
};
