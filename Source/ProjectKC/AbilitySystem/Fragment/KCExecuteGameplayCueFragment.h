#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"
#include "KCExecuteGameplayCueFragment.generated.h"

/** 실행 문맥의 위치·표면·SourceObject를 담아 일회성 GameplayCue를 실행한다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Execute Gameplay Cue"))
class PROJECTKC_API UKCExecuteGameplayCueFragment : public UKCActionFragment
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const override;
	virtual bool SupportsDeferredExecution() const override;
	virtual bool CanExecute(
		const FKCActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FKCActionExecutionContext& Context) const override;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Cue",
		meta = (Categories = "GameplayCue"))
	FGameplayTag CueTag;
};
