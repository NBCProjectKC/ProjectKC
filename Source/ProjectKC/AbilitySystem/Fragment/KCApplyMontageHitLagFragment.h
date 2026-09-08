#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"
#include "ProjectKC/AbilitySystem/Struct/KCMontageHitLagConfigStruct.h"
#include "KCApplyMontageHitLagFragment.generated.h"

/** 명중이 확정됐을 때 소스의 현재 공격 몽타주를 잠깐 느리게 만든다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Apply Montage Hit Lag"))
class PROJECTKC_API UKCApplyMontageHitLagFragment : public UKCActionFragment
{
	GENERATED_BODY()

public:
	UKCApplyMontageHitLagFragment();

	virtual bool Validate(FString& OutError) const override;
	virtual bool CanExecute(
		const FKCActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FKCActionExecutionContext& Context) const override;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Hit Lag",
		meta = (ShowOnlyInnerProperties))
	FKCMontageHitLagConfigStruct HitLag;
};
