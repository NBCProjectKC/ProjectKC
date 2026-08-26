#include "ProjectKC/Player/Animation/KCPlayerAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UKCPlayerAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
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
