/**
 * @file KCLobbyWidget.cpp
 * @brief UKCLobbyWidget 구현부
 */

#include "ProjectKC/Lobby/UI/KCLobbyWidget.h"
#include "ProjectKC/Lobby/UI/KCFriendListWidget.h"
#include "ProjectKC/Lobby/KCLobbyPlayerController.h"
#include "ProjectKC/Lobby/KCLobbyPlayerState.h"
#include "ProjectKC/GameSystem/KCLobbyGameMode.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"
#include "Kismet/KismetSystemLibrary.h"

void UKCLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Socials)
	{
		Button_Socials->OnClicked.AddDynamic(this, &UKCLobbyWidget::OnSocialsClicked);
	}

	if (Button_Ready)
	{
		Button_Ready->OnClicked.AddDynamic(this, &UKCLobbyWidget::OnReadyClicked);
	}

	if (Button_StartGame)
	{
		Button_StartGame->OnClicked.AddDynamic(this, &UKCLobbyWidget::OnStartGameClicked);
		// 방장(서버)만 StartGame 버튼 노출 (블루프린트의 IsServer와 동일)
		const bool bIsServer = UKismetSystemLibrary::IsServer(this);
		Button_StartGame->SetVisibility(bIsServer ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		// 시작 시 기본 비활성화 (전원 레디 시 활성화)
		Button_StartGame->SetIsEnabled(false);
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AKCLobbyPlayerState* PS = PC->GetPlayerState<AKCLobbyPlayerState>())
		{
			PS->OnReadyStatusChanged.AddUniqueDynamic(this, &UKCLobbyWidget::OnReadyStatusUpdated);
			OnReadyStatusUpdated(PS->IsReady());
		}
	}
}

void UKCLobbyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 클라이언트에서 PlayerState가 뒤늦게 동기화되었을 때 바인딩 보장
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AKCLobbyPlayerState* PS = PC->GetPlayerState<AKCLobbyPlayerState>())
		{
			PS->OnReadyStatusChanged.AddUniqueDynamic(this, &UKCLobbyWidget::OnReadyStatusUpdated);
		}
	}
}

void UKCLobbyWidget::PlayMatchStartAnim()
{
	if (MatchStartAnim)
	{
		PlayAnimationForward(MatchStartAnim);
	}
	SetIsEnabled(false);
}

void UKCLobbyWidget::SetStartGameButtonEnabled(bool bEnabled)
{
	if (Button_StartGame)
	{
		Button_StartGame->SetIsEnabled(bEnabled);
	}
}

void UKCLobbyWidget::OnSocialsClicked()
{
	if (WBP_FriendList)
	{
		const ESlateVisibility CurrentVis = WBP_FriendList->GetVisibility();
		WBP_FriendList->SetVisibility(CurrentVis == ESlateVisibility::Visible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void UKCLobbyWidget::OnReadyClicked()
{
	if (AKCLobbyPlayerController* PC = Cast<AKCLobbyPlayerController>(GetOwningPlayer()))
	{
		PC->ROS_ToggleReadyStatus();
	}
}

void UKCLobbyWidget::OnStartGameClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (AKCLobbyGameMode* GM = World->GetAuthGameMode<AKCLobbyGameMode>())
		{
			GM->StartGame();
		}
	}
}

void UKCLobbyWidget::OnReadyStatusUpdated(bool bIsReady)
{
	if (Text_Ready)
	{
		Text_Ready->SetText(bIsReady ? FText::FromString(TEXT("CANCEL READY")) : FText::FromString(TEXT("READY")));
	}
}
