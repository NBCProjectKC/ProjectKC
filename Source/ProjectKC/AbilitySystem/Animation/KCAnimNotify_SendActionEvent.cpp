#include "ProjectKC/AbilitySystem/Animation/KCAnimNotify_SendActionEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "ProjectKC/AbilitySystem/Tag/KCAbilityGameplayTags.h"

/**
 * @brief Initializes the notify with the default action event tag and editor settings.
 */
UKCAnimNotify_SendActionEvent::UKCAnimNotify_SendActionEvent()
{
	ActionEventTag = TAG_KC_GameplayEvent_Action_Execute;

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(220, 80, 60);

	// ASC가 없는 애님 에디터 프리뷰에서 Event 전송 실패 로그가 쌓이지 않게 한다.
	bShouldFireInEditor = false;
#endif
}

/**
 * @brief Gets the notify name from the configured action event tag.
 *
 * @return FString The action event tag name, or a default notify name when the tag is invalid.
 */
FString UKCAnimNotify_SendActionEvent::GetNotifyName_Implementation() const
{
	return ActionEventTag.IsValid()
		? ActionEventTag.ToString()
		: TEXT("KC Send Action Event");
}

/**
 * @brief Sends the configured gameplay action event to the owning actor when running with authority.
 *
 * @param MeshComp Skeletal mesh component whose owner receives the event.
 * @param Animation Animation containing the notify.
 * @param EventReference Reference to the animation notify event.
 */
void UKCAnimNotify_SendActionEvent::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	// 클라이언트 프록시에서도 Notify 자체는 호출되므로 Event 전송만 Authority로 막는다.
	AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority() ||
		!ActionEventTag.IsValid())
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = ActionEventTag;
	EventData.Instigator = OwnerActor;
	EventData.Target = OwnerActor;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		OwnerActor,
		ActionEventTag,
		EventData);
}

#if WITH_EDITOR
/**
 * @brief Determines whether the notify can be placed on the specified animation.
 *
 * @param Animation Animation asset to evaluate.
 * @return `true` if the animation is a montage, `false` otherwise.
 */
bool UKCAnimNotify_SendActionEvent::CanBePlaced(
	UAnimSequenceBase* Animation) const
{
	// Action 실행 시점은 Ability가 재생하는 몽타주 타임라인에서만 의미가 있다.
	return Animation && Animation->IsA<UAnimMontage>();
}
#endif
