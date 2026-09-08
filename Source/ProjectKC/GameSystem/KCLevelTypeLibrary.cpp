#include "KCLevelTypeLibrary.h"
#include "KCLevelInfoRow.h"
#include "Engine/DataTable.h"

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
	case EKCLevelType::MicroWaveOven:	return TEXT("L_PortableGasStove");
	case EKCLevelType::FryingPan:		return TEXT("L_FryingPan");
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
	if (LevelName == TEXT("L_PortableGasStove"))return EKCLevelType::MicroWaveOven;
	if (LevelName == TEXT("L_FryingPan"))    return EKCLevelType::FryingPan;

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

const FKCLevelInfoRow* UKCLevelTypeLibrary::GetLevelInfoRow(EKCLevelType LevelType)
{
	const TSoftObjectPtr<UDataTable> LevelInfoTablePath(
		FSoftObjectPath(TEXT("/Game/KC/GameSystem/DT_LevelInfo.DT_LevelInfo")));   // static 제거

	UDataTable* Table = LevelInfoTablePath.LoadSynchronous();
	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("UKCLevelTypeLibrary::GetLevelInfoRow - DT_LevelInfo를 로드하지 못했습니다."));
		return nullptr;
	}

	const FName RowName = GetLevelName(LevelType);
	return Table->FindRow<FKCLevelInfoRow>(RowName, TEXT("GetLevelInfoRow"));
}