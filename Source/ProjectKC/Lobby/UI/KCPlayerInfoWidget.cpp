/**
 * @file KCPlayerInfoWidget.cpp
 * @brief UKCPlayerInfoWidget 구현부
 */

#include "ProjectKC/Lobby/UI/KCPlayerInfoWidget.h"
#include "ProjectKC/ProjectKC.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "KCPlayerInfoWidget"

void UKCPlayerInfoWidget::UpdatePlayerInfo(const FKCPlayerInfoStruct& InInfo)
{
	PlayerInfo = InInfo;

	UE_LOG(LogKCLobby, Verbose, TEXT("[KCPlayerInfoWidget] UpdatePlayerInfo: Name='%s', Ready=%s"),
		*InInfo.PlayerName, InInfo.bReady ? TEXT("TRUE") : TEXT("FALSE"));

	// 1. 닉네임 설정
	if (Text_PlayerName)
	{
		Text_PlayerName->SetText(InInfo.PlayerName.IsEmpty() ? LOCTEXT("WaitingPlayer", "Waiting...") : FText::FromString(InInfo.PlayerName));
	}

	// 2. 머리 위 레디 상태 텍스트 및 투명도 설정
	if (Text_ReadyStatus)
	{
		if (InInfo.PlayerName.IsEmpty())
		{
			Text_ReadyStatus->SetText(FText::GetEmpty());
			Text_ReadyStatus->SetOpacity(0.0f);
		}
		else
		{
			// 준비 완료: 밝은 READY (1.0f), 준비 전: 흐린 NOT READY (0.35f)
			Text_ReadyStatus->SetText(InInfo.bReady ? LOCTEXT("PlayerReady", "READY") : LOCTEXT("PlayerNotReady", "NOT READY"));
			const float Opacity = InInfo.bReady ? 1.0f : 0.35f;
			Text_ReadyStatus->SetOpacity(Opacity);
		}
	}
}

#undef LOCTEXT_NAMESPACE

