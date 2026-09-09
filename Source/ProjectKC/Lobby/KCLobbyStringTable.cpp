/**
 * @file KCLobbyStringTable.cpp
 * @brief 로비 스트링 테이블 에셋 기반 조회 구현
 */

#include "ProjectKC/Lobby/KCLobbyStringTable.h"
#include "Internationalization/StringTableRegistry.h"
#include "UObject/Package.h"

const FName UKCLobbyStringTable::TableId = TEXT("ST_LobbyMessage");
const FName UKCLobbyStringTable::StringTablePath = TEXT("/Game/KC/SteamLobbySystem/Data/ST_LobbyMessage");

void UKCLobbyStringTable::EnsureStringTableLoaded()
{
	// 에셋 등록 여부 확인
	if (!FStringTableRegistry::Get().FindStringTable(StringTablePath) &&
		!FStringTableRegistry::Get().FindStringTable(TableId))
	{
		// 에셋이 아직 메모리에 로드되지 않았다면 로드 수행 
		StaticLoadObject(UObject::StaticClass(), nullptr, *StringTablePath.ToString());
	}
}

FName UKCLobbyStringTable::GetKeyName(EKCLobbyMessageType MessageType)
{
	switch (MessageType)
	{
	case EKCLobbyMessageType::LobbyFull:
		return TEXT("Lobby_Full");
	case EKCLobbyMessageType::HostClosed:
		return TEXT("Host_Closed");
	case EKCLobbyMessageType::HostLost:
		return TEXT("Host_Lost");
	case EKCLobbyMessageType::SessionNotFound:
		return TEXT("Session_NotFound");
	case EKCLobbyMessageType::NotAllReady:
		return TEXT("Not_All_Ready");
	default:
		return NAME_None;
	}
}

FText UKCLobbyStringTable::GetMessage(EKCLobbyMessageType MessageType)
{
	const FName Key = GetKeyName(MessageType);
	if (Key.IsNone())
	{
		return FText::GetEmpty();
	}

	return GetMessageByKey(Key);
}

FText UKCLobbyStringTable::GetMessageByKey(FName Key)
{
	EnsureStringTableLoaded();

	// 에셋 전체 경로(/Game/KC/SteamLobbySystem/Data/ST_LobbyMessage)로 조회
	FText Message = FText::FromStringTable(StringTablePath, Key.ToString());
	if (!Message.IsEmpty())
	{
		return Message;
	}

	// TableId(ST_LobbyMessage)로 조회
	Message = FText::FromStringTable(TableId, Key.ToString());
	return Message;
}
