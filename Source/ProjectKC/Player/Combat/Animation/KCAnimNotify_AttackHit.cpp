#include "Player/Combat/Animation/KCAnimNotify_AttackHit.h"

#include "Components/SkeletalMeshComponent.h"
#include "Player/Combat/KCPlayerCombatComponent.h"
#include "Player/KCPlayerCharacter.h"

void UKCAnimNotify_AttackHit::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AKCPlayerCharacter* PlayerCharacter = MeshComp
		? Cast<AKCPlayerCharacter>(MeshComp->GetOwner())
		: nullptr;
	if (!PlayerCharacter)
	{
		return;
	}

	if (UKCPlayerCombatComponent* CombatComponent = PlayerCharacter->GetCombatComponent())
	{
		CombatComponent->HandleAttackHitNotify();
	}
}

FString UKCAnimNotify_AttackHit::GetNotifyName_Implementation() const
{
	return TEXT("KC Attack Hit");
}
