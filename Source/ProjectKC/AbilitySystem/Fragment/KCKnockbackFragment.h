#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"
#include "KCKnockbackFragment.generated.h"

UENUM(BlueprintType)
enum /**
 * Applies configurable physical knockback to the execution context's target.
 */
class EKCKnockbackDirectionMode : uint8
{
	SourceToTarget,
	SourceForward,
	InverseHitNormal
};

/** 실행 문맥의 Target에 선택적으로 물리 넉백을 적용한다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Knockback"))
class PROJECTKC_API UKCKnockbackFragment : public UKCActionFragment
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const override;
	virtual bool CanExecute(
		const FKCActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FKCActionExecutionContext& Context) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Knockback")
	float HorizontalSpeed = 950.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Knockback")
	float VerticalSpeed = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Knockback")
	EKCKnockbackDirectionMode DirectionMode =
		EKCKnockbackDirectionMode::SourceToTarget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Knockback")
	bool bOverrideHorizontalVelocity = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Knockback")
	bool bOverrideVerticalVelocity = true;

private:
	bool BuildRequest(
		const FKCActionExecutionContext& Context,
		struct FKCKnockbackRequest& OutRequest) const;
	FVector ResolveDirection(const FKCActionExecutionContext& Context) const;
};
