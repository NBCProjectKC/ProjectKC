#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"
#include "ProjectKC/AbilitySystem/Struct/KCGameplayEffectRecipeStruct.h"
#include "KCApplyGameplayEffectFragment.generated.h"

/** 실행 문맥의 대상 ASC에 하나의 GameplayEffect Recipe를 적용한다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Apply Gameplay Effect"))
class /**
 * Applies a gameplay effect recipe to the target ability system component.
 *
 * @note Infinite effects can optionally be tracked and removed when the ability ends.
 */
PROJECTKC_API UKCApplyGameplayEffectFragment : public UKCActionFragment
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const override;
	virtual bool DeclaresSetByCallerTag(FGameplayTag DataTag) const override;
	virtual void AppendDeclaredSetByCallerTags(
		FGameplayTagContainer& OutTags) const override;
	virtual bool CanExecute(
		const FKCActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FKCActionExecutionContext& Context) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Ability|Effect")
	FKCGameplayEffectRecipeStruct EffectRecipe;

	/** Infinite GE를 GA 종료 시 자동 회수할 때 사용한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Ability|Effect")
	bool bTrackUntilAbilityEnds = false;
};
