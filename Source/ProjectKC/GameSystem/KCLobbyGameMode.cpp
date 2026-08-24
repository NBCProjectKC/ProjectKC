#include "KCLobbyGameMode.h"

AKCLobbyGameMode::AKCLobbyGameMode()
{
}

void AKCLobbyGameMode::SetRequiredPlayerCount(int32 InCount)
{
	RequiredPlayerCount = InCount;
}

bool AKCLobbyGameMode::ReadyToStartMatch_Implementation()
{
	return GetNumPlayers() >= RequiredPlayerCount;
}

void AKCLobbyGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	GetWorld()->ServerTravel(BattleLevelName);
}