#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"
#include "KCDropHeldItemFragment.generated.h"

UENUM(BlueprintType)
enum class EKCDropItemImpulseDirectionMode : uint8
{
	SourceToTarget,
	SourceForward,
	InverseHitNormal
};

/** 실행 문맥의 대상이 들고 있는 아이템을 즉시 드롭시킨다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Drop Held Item"))
class PROJECTKC_API UKCDropHeldItemFragment : public UKCActionFragment
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const override;
	virtual bool CanExecute(
		const FKCActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FKCActionExecutionContext& Context) const override;

	/** 켜면 기존 드롭 Impulse에 아래 값을 추가한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop|Impulse")
	bool bApplyImpulse = false;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Drop|Impulse",
		meta = (EditCondition = "bApplyImpulse", ClampMin = "0.0"))
	float HorizontalImpulse = 600.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Drop|Impulse",
		meta = (EditCondition = "bApplyImpulse", ClampMin = "0.0"))
	float VerticalImpulse = 200.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Drop|Impulse",
		meta = (EditCondition = "bApplyImpulse"))
	EKCDropItemImpulseDirectionMode DirectionMode =
		EKCDropItemImpulseDirectionMode::SourceToTarget;

private:
	FVector BuildAdditionalImpulse(
		const FKCActionExecutionContext& Context) const;
	FVector ResolveHorizontalDirection(
		const FKCActionExecutionContext& Context) const;
};
