#include "KCLevelTypeLibrary.h"

FName UKCLevelTypeLibrary::GetLevelName(EKCLevelType LevelType)
{
	switch (LevelType)
	{
	case EKCLevelType::SplashScreen:	return TEXT("L_SplashScreen");
	case EKCLevelType::MainMenu:		return TEXT("L_MainMeun");
	case EKCLevelType::LoadInLevel:		return TEXT("L_LoadInLevel");
	case EKCLevelType::LobbyLevel:		return TEXT("L_LobbyLevel");
	case EKCLevelType::Loading:			return TEXT("L_Loading");
	case EKCLevelType::GasRange:		return TEXT("L_GasRange");
	default:							return NAME_None;
	}
}

EKCLevelType UKCLevelTypeLibrary::GetLevelType(FName LevelName)
{
	if (LevelName == TEXT("L_SplashScreen")) return EKCLevelType::SplashScreen;
	if (LevelName == TEXT("L_MainMeun"))     return EKCLevelType::MainMenu;
	if (LevelName == TEXT("L_LoadInLevel"))  return EKCLevelType::LoadInLevel;
	if (LevelName == TEXT("L_LobbyLevel"))   return EKCLevelType::LobbyLevel;
	if (LevelName == TEXT("L_Loading"))      return EKCLevelType::Loading;
	if (LevelName == TEXT("L_GasRange"))     return EKCLevelType::GasRange;

	return EKCLevelType::None;
}

EKCLevelType UKCLevelTypeLibrary::GetLevelTypeFromWorld(const UWorld* World)
{
	if (!World)
	{
		return EKCLevelType::None;
	}

	FString MapName = World->GetMapName();
	MapName.RemoveFromStart(World->StreamingLevelsPrefix);

	return GetLevelType(FName(*MapName));
}
