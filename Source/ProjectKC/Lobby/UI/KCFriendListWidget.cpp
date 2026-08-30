/**
 * @file KCFriendListWidget.cpp
 * @brief UKCFriendListWidget 구현부
 */

#include "ProjectKC/Lobby/UI/KCFriendListWidget.h"
#include "ProjectKC/Lobby/UI/KCFriendWidget.h"
#include "ProjectKC/ProjectKC.h"
#include "AdvancedFriendsLibrary.h"
#include "Components/PanelWidget.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "TimerManager.h"

UKCFriendListWidget::UKCFriendListWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UKCFriendListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!FriendEntryWidgetClass)
	{
		static UClass* LoadedClass = StaticLoadClass(UKCFriendWidget::StaticClass(), nullptr, TEXT("/Game/KC/SteamLobbySystem/Blueprints/UI/WBP_Friend.WBP_Friend_C"));
		FriendEntryWidgetClass = LoadedClass;
	}

	// 1. 즉시 1회 친구 목록 갱신
	RefreshFriendList();

	// 2. 5초마다 자동 갱신 타이머 시작
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FriendListRefreshTimerHandle,
			this,
			&UKCFriendListWidget::RefreshFriendList,
			5.0f,
			true
		);
	}
}

void UKCFriendListWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FriendListRefreshTimerHandle);
	}

	Super::NativeDestruct();
}

void UKCFriendListWidget::RefreshFriendList()
{
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS)
	{
		UE_LOG(LogKCSession, Warning, TEXT("[KCFriendListWidget] RefreshFriendList Failed: OnlineSubsystem is null"));
		return;
	}

	IOnlineFriendsPtr FriendsInterface = OSS->GetFriendsInterface();
	if (!FriendsInterface.IsValid())
	{
		UE_LOG(LogKCSession, Warning, TEXT("[KCFriendListWidget] RefreshFriendList Failed: FriendsInterface is invalid"));
		return;
	}

	UE_LOG(LogKCSession, Verbose, TEXT("[KCFriendListWidget] Requesting ReadFriendsList..."));
	FriendsInterface->ReadFriendsList(
		0,
		EFriendsLists::ToString(EFriendsLists::Default),
		FOnReadFriendsListComplete::CreateUObject(this, &UKCFriendListWidget::HandleReadFriendsListComplete)
	);
}

void UKCFriendListWidget::HandleReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogKCSession, Warning, TEXT("[KCFriendListWidget] ReadFriendsList failed: %s"), *ErrorStr);
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	TArray<FBPFriendInfo> FriendsList;
	UAdvancedFriendsLibrary::GetStoredFriendsList(PC, FriendsList);

	int32 OnlineCount = 0;
	for (const FBPFriendInfo& Friend : FriendsList)
	{
		if (Friend.OnlineState != EBPOnlinePresenceState::Offline)
		{
			OnlineCount++;
		}
	}

	UE_LOG(LogKCSession, Verbose, TEXT("[KCFriendListWidget] ReadFriendsList Complete: Found %d friends (%d online)"),
		FriendsList.Num(), OnlineCount);

	// 온라인 친구가 위로 오도록 정렬
	FriendsList.Sort([](const FBPFriendInfo& A, const FBPFriendInfo& B)
	{
		const bool bAOnline = (A.OnlineState != EBPOnlinePresenceState::Offline);
		const bool bBOnline = (B.OnlineState != EBPOnlinePresenceState::Offline);
		return bAOnline > bBOnline;
	});

	if (!FriendList || !FriendEntryWidgetClass)
	{
		return;
	}

	// 위젯 풀링: 부족하면 생성, 남으면 재사용
	AddedFriendWidgets.Reserve(FriendsList.Num());
	for (int32 i = 0; i < FriendsList.Num(); ++i)
	{
		UKCFriendWidget* EntryWidget = nullptr;
		if (AddedFriendWidgets.IsValidIndex(i))
		{
			EntryWidget = AddedFriendWidgets[i];
			if (EntryWidget)
			{
				EntryWidget->SetVisibility(ESlateVisibility::Visible);
			}
		}
		else
		{
			EntryWidget = CreateWidget<UKCFriendWidget>(this, FriendEntryWidgetClass);
			if (EntryWidget)
			{
				FriendList->AddChild(EntryWidget);
				AddedFriendWidgets.Add(EntryWidget);
			}
		}

		if (EntryWidget)
		{
			EntryWidget->Update(FriendsList[i]);
		}
	}

	// 친구 수보다 많은 기존 풀링 위젯은 숨김 처리
	for (int32 i = FriendsList.Num(); i < AddedFriendWidgets.Num(); ++i)
	{
		if (AddedFriendWidgets[i])
		{
			AddedFriendWidgets[i]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

