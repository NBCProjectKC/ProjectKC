/**
 * @file KCVerbMessageHelpers.cpp
 * @brief UKCVerbMessageHelpers 구현부
 */

#include "ProjectKC/Messages/Library/KCVerbMessageHelpers.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(KCVerbMessageHelpers)

APlayerState* UKCVerbMessageHelpers::GetPlayerStateFromObject(UObject* Object)
{
	if (!Object)
	{
		return nullptr;
	}

	if (APlayerController* PC = Cast<APlayerController>(Object))
	{
		return PC->PlayerState;
	}

	if (APlayerState* TargetPS = Cast<APlayerState>(Object))
	{
		return TargetPS;
	}

	if (APawn* TargetPawn = Cast<APawn>(Object))
	{
		if (APlayerState* TargetPS = TargetPawn->GetPlayerState())
		{
			return TargetPS;
		}
	}

	if (const UActorComponent* Component = Cast<UActorComponent>(Object))
	{
		return GetPlayerStateFromObject(Component->GetOwner());
	}

	return nullptr;
}

APlayerController* UKCVerbMessageHelpers::GetPlayerControllerFromObject(UObject* Object)
{
	if (!Object)
	{
		return nullptr;
	}

	if (APlayerController* PC = Cast<APlayerController>(Object))
	{
		return PC;
	}

	if (APlayerState* TargetPS = Cast<APlayerState>(Object))
	{
		return TargetPS->GetPlayerController();
	}

	if (APawn* TargetPawn = Cast<APawn>(Object))
	{
		return Cast<APlayerController>(TargetPawn->GetController());
	}

	if (const UActorComponent* Component = Cast<UActorComponent>(Object))
	{
		return GetPlayerControllerFromObject(Component->GetOwner());
	}

	return nullptr;
}
