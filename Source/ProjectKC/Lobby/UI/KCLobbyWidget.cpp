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
		const bool bIsServer = UKismetSystemLibrary::IsServer(this);
		Button_StartGame->SetVisibility(bIsServer ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button_StartGame->SetIsEnabled(false);
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AKCLobbyPlayerState* PS = PC->GetPlayerState<AKCLobbyPlayerState>())
		{
			PS->OnReadyStatusChanged.AddUniqueDynamic(this, &UKCLobbyWidget::OnReadyStatusUpdated);
			PS->OnTeamIdChanged.AddUniqueDynamic(this, &UKCLobbyWidget::OnTeamIdUpdated);
			OnReadyStatusUpdated(PS->IsReady());
			OnTeamIdUpdated(PS->GetTeamId());
			bPlayerStateBound = true;
		}
	}
}

void UKCLobbyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 클라이언트에서 PlayerState 복제 완료 시 즉시 바인딩 및 초기 UI 텍스트 갱신
	if (!bPlayerStateBound)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (AKCLobbyPlayerState* PS = PC->GetPlayerState<AKCLobbyPlayerState>())
			{
				PS->OnReadyStatusChanged.AddUniqueDynamic(this, &UKCLobbyWidget::OnReadyStatusUpdated);
				PS->OnTeamIdChanged.AddUniqueDynamic(this, &UKCLobbyWidget::OnTeamIdUpdated);
				OnReadyStatusUpdated(PS->IsReady());
				OnTeamIdUpdated(PS->GetTeamId());
				bPlayerStateBound = true;
			}
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
		// 준비 완료 시: CANCEL READY (준비 취소), 미준비 시: READY (준비하기)
		Text_Ready->SetText(bIsReady ? FText::FromString(TEXT("CANCEL READY")) : FText::FromString(TEXT("READY")));
	}
}

void UKCLobbyWidget::OnTeamIdUpdated(int32 NewTeamId)
{
	if (Text_TeamName)
	{
		const FString TeamStr = (NewTeamId == 0) ? TEXT("TEAM RED") : TEXT("TEAM BLUE");
		Text_TeamName->SetText(FText::FromString(TeamStr));
	}
}
