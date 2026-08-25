/**
 * @file KCFriendWidget.cpp
 * @brief UKCFriendWidget 구현부
 */

#include "ProjectKC/Lobby/UI/KCFriendWidget.h"
#include "ProjectKC/Lobby/KCSessionSubsystem.h"
#include "AdvancedSteamFriendsLibrary.h"
#include "AdvancedFriendsLibrary.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UKCFriendWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Invite)
	{
		Button_Invite->OnClicked.AddDynamic(this, &UKCFriendWidget::OnInviteClicked);
	}
}

void UKCFriendWidget::Update(const FBPFriendInfo& FriendData)
{
	CachedFriendData = FriendData;
	if (FriendData.UniqueNetId.IsValid() && FriendData.UniqueNetId.GetUniqueNetId())
	{
		FriendUniqueNetId = FriendData.UniqueNetId.GetUniqueNetId()->ToString();
	}
	else
	{
		FriendUniqueNetId.Empty();
	}

	// 1. 친구 이름 텍스트 설정
	if (Text_PlayerName)
	{
		Text_PlayerName->SetText(FText::FromString(FriendData.DisplayName));
	}

	// 2. 스팀 아바타 이미지 로드 및 Image_Avatar 브러시 적용
	if (Image_Avatar)
	{
		EBlueprintAsyncResultSwitch Result;
		UTexture2D* AvatarTexture = UAdvancedSteamFriendsLibrary::GetSteamFriendAvatar(FriendData.UniqueNetId, Result, SteamAvatarSize::SteamAvatar_Medium);
		if (Result == EBlueprintAsyncResultSwitch::OnSuccess && AvatarTexture)
		{
			Image_Avatar->SetBrushFromTexture(AvatarTexture, false);
		}
	}

	// 3. 오프라인 상태에 따른 투명도 설정 (Offline: 0.25, Online: 1.0)
	const float Opacity = (FriendData.OnlineState == EBPOnlinePresenceState::Offline) ? 0.25f : 1.0f;
	SetRenderOpacity(Opacity);
}

void UKCFriendWidget::SetupFriend(const FString& InFriendName, const FString& InFriendUniqueNetId)
{
	FriendUniqueNetId = InFriendUniqueNetId;

	if (Text_PlayerName)
	{
		Text_PlayerName->SetText(FText::FromString(InFriendName));
	}
}

void UKCFriendWidget::OnInviteClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(this, 0);
	}

	if (PC && CachedFriendData.UniqueNetId.IsValid())
	{
		EBlueprintResultSwitch Result;
		UAdvancedFriendsLibrary::SendSessionInviteToFriend(PC, CachedFriendData.UniqueNetId, Result);
		if (Result == EBlueprintResultSwitch::OnSuccess)
		{
			UKismetSystemLibrary::PrintString(this, TEXT("Invited!"), true, true, FLinearColor(0.0f, 0.66f, 1.0f, 1.0f), 2.0f);
		}
		else
		{
			UKismetSystemLibrary::PrintString(this, TEXT("Failed to invite.."), true, true, FLinearColor(0.0f, 0.66f, 1.0f, 1.0f), 2.0f);
		}
	}
	else if (UGameInstance* GI = GetGameInstance())
	{
		if (UKCSessionSubsystem* SessionSubsystem = GI->GetSubsystem<UKCSessionSubsystem>())
		{
			SessionSubsystem->SendSessionInviteToFriend(FriendUniqueNetId);
		}
	}
}
