#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "KCAnimNotify_SendActionEvent.generated.h"

class UAnimSequenceBase;
class USkeletalMeshComponent;

/**
 * 몽타주의 실제 결과 프레임에서 Action Gameplay Event를 보낸다.
 * 소리나 Trail 같은 순수 연출은 이 Notify와 분리한다.
 */
UCLASS(
	const,
	hidecategories = Object,
	collapsecategories,
	meta = (DisplayName = "KC Send Action Event"))
class PROJECTKC_API UKCAnimNotify_SendActionEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	UKCAnimNotify_SendActionEvent();

	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITOR
	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override;
#endif

protected:
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "KC|Ability|Action",
		meta = (Categories = "GameplayEvent.Action"))
	FGameplayTag ActionEventTag;
};
