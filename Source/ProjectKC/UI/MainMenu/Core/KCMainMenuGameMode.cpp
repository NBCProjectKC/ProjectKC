#include "ProjectKC/UI/MainMenu/Core/KCMainMenuGameMode.h"

#include "ProjectKC/UI/MainMenu/Core/KCMainMenuPlayerController.h"

AKCMainMenuGameMode::AKCMainMenuGameMode()
{
	PlayerControllerClass = AKCMainMenuPlayerController::StaticClass();
}
