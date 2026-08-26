/**
 * @file KCPlayerInfoWidget.cpp
 * @brief UKCPlayerInfoWidget 구현부
 */

#include "ProjectKC/Lobby/UI/KCPlayerInfoWidget.h"
#include "Components/TextBlock.h"

void UKCPlayerInfoWidget::UpdatePlayerInfo(const FKCPlayerInfoStruct& InInfo)
{
	PlayerInfo = InInfo;

	// 1. 닉네임 설정
	if (Text_PlayerName)
	{
		Text_PlayerName->SetText(FText::FromString(InInfo.PlayerName.IsEmpty() ? TEXT("Waiting...") : InInfo.PlayerName));
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
			Text_ReadyStatus->SetText(InInfo.bReady ? FText::FromString(TEXT("READY")) : FText::FromString(TEXT("NOT READY")));
			const float Opacity = InInfo.bReady ? 1.0f : 0.35f;
			Text_ReadyStatus->SetOpacity(Opacity);
		}
	}
}
