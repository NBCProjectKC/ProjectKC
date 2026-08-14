#include "Game/KCGameMode.h"

#include "Player/KCPlayerCharacter.h"
#include "Player/KCPlayerController.h"
#include "UObject/ConstructorHelpers.h"

AKCGameMode::AKCGameMode()
{
	DefaultPawnClass = AKCPlayerCharacter::StaticClass();
	PlayerControllerClass = AKCPlayerController::StaticClass();

	static ConstructorHelpers::FClassFinder<AKCPlayerCharacter> PlayerCharacterBlueprint(
		TEXT("/Game/Blueprints/Player/BP_KCPlayerCharacter"));
	if (PlayerCharacterBlueprint.Succeeded())
	{
		DefaultPawnClass = PlayerCharacterBlueprint.Class;
	}
}
