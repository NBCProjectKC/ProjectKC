/**
 * @file KCPlayerInfoWidget.cpp
 * @brief UKCPlayerInfoWidget 구현부
 */

#include "ProjectKC/Lobby/UI/KCPlayerInfoWidget.h"
#include "Components/TextBlock.h"

void UKCPlayerInfoWidget::UpdatePlayerInfo(const FKCPlayerInfoStruct& InInfo)
{
	PlayerInfo = InInfo;

	// 1. 닉네임 설정 (비어있으면 Waiting...)
	if (Text_PlayerName)
	{
		Text_PlayerName->SetText(FText::FromString(InInfo.PlayerName.IsEmpty() ? TEXT("Waiting...") : InInfo.PlayerName));
	}

	// 2. 레디 상태 텍스트 및 투명도 설정
	if (Text_ReadyStatus)
	{
		if (InInfo.PlayerName.IsEmpty())
		{
			Text_ReadyStatus->SetText(FText::GetEmpty());
			Text_ReadyStatus->SetOpacity(0.0f);
		}
		else
		{
			Text_ReadyStatus->SetText(InInfo.bReady ? FText::FromString(TEXT("READY")) : FText::FromString(TEXT("NOT READY")));

			// 레디 상태에 따른 투명도 조절 (READY: 1.0, NOT READY: 0.2)
			const float Opacity = InInfo.bReady ? 1.0f : 0.2f;
			Text_ReadyStatus->SetOpacity(Opacity);
		}
	}
}
