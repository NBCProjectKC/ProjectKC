#include "ProjectKC/Player/Animation/KCPlayerAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"

void UKCPlayerAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	RefreshHeldPoseState();
	RefreshPostProcessIKState();
}

void UKCPlayerAnimInstance::SetEmoteActive(const bool bNewEmoteActive)
{
	bEmoteActive = bNewEmoteActive;
	RefreshPostProcessIKState();
}


void UKCPlayerAnimInstance::RefreshPostProcessIKState()
{
	const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
	const UCharacterMovementComponent* Movement = Character
		? Character->GetCharacterMovement()
		: nullptr;
	bAllowGroundIK = !bEmoteActive
		&& !IsAnyMontagePlaying()
		&& (!Movement || !Movement->IsFalling());
}

void UKCPlayerAnimInstance::RefreshHeldPoseState()
{
	HeldPose = EKCHeldPose::Default;
	const UKCHeldItemComponent* HeldItemComponent = ResolveHeldItemComponent();
	const AKCWorldItemActor* HeldItem = HeldItemComponent
		? HeldItemComponent->GetHeldItem()
		: nullptr;
	const UKCItemDefinition* ItemDefinition = HeldItem
		? HeldItem->GetItemDefinition()
		: nullptr;
	if (ItemDefinition)
	{
		HeldPose = ItemDefinition->Presentation.HeldPose;
	}
}

const UKCHeldItemComponent* UKCPlayerAnimInstance::ResolveHeldItemComponent()
{
	const APawn* PawnOwner = TryGetPawnOwner();
	if (CachedPawnOwner.Get() == PawnOwner)
	{
		return CachedHeldItemComponent.Get();
	}

	CachedPawnOwner = PawnOwner;
	CachedHeldItemComponent = PawnOwner
		? PawnOwner->FindComponentByClass<UKCHeldItemComponent>()
		: nullptr;
	return CachedHeldItemComponent.Get();
}
