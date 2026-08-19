#include "Game/KCGameMode.h"

#include "Player/KCPlayerCharacter.h"
#include "Player/KCPlayerController.h"

AKCGameMode::AKCGameMode()
{
	DefaultPawnClass = AKCPlayerCharacter::StaticClass();
	PlayerControllerClass = AKCPlayerController::StaticClass();
}
