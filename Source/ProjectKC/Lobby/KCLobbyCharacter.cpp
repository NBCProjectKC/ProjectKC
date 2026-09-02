/**
 * @file KCLobbyCharacter.cpp
 * @brief AKCLobbyCharacter 구현부
 */

#include "ProjectKC/Lobby/KCLobbyCharacter.h"
#include "ProjectKC/Lobby/UI/KCPlayerInfoWidget.h"
#include "ProjectKC/Player/Component/KCPlayerCustomizationComponent.h"
#include "ProjectKC/Player/KCPlayerState.h"
#include "ProjectKC/ProjectKC.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AKCLobbyCharacter::AKCLobbyCharacter()
{
	bReplicates = true;
}

void AKCLobbyCharacter::BeginPlay()
{
	Super::BeginPlay();

	CachedWidgetComp = FindComponentByClass<UWidgetComponent>();
	RefreshPlayerInfoWidget();
	RefreshCustomizationPresentation();
}

void AKCLobbyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKCLobbyCharacter, PlayerInfo);
}

void AKCLobbyCharacter::UpdatePlayerInfo(const FKCPlayerInfoStruct& InNewInfo)
{
	if (HasAuthority())
	{
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyCharacter] UpdatePlayerInfo for '%s' (Ready: %s)"),
			*InNewInfo.PlayerName, InNewInfo.bReady ? TEXT("TRUE") : TEXT("FALSE"));
		PlayerInfo = InNewInfo;
		OnRep_PlayerInfo();
	}
}

void AKCLobbyCharacter::OnRep_PlayerInfo()
{
	UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyCharacter] OnRep_PlayerInfo: '%s' (Ready: %s)"),
		*PlayerInfo.PlayerName, PlayerInfo.bReady ? TEXT("TRUE") : TEXT("FALSE"));
	OnPlayerInfoUpdated.Broadcast(PlayerInfo);
	RefreshPlayerInfoWidget();
	RefreshCustomizationPresentation();
}

void AKCLobbyCharacter::RefreshCustomizationPresentation()
{
	UKCPlayerCustomizationComponent* CustomizationComponent =
		GetPlayerCustomizationComponent();
	AKCPlayerState* PresentationPlayerState =
		Cast<AKCPlayerState>(PlayerInfo.PlayerState.Get());
	if (!CustomizationComponent)
	{
		return;
	}

	APlayerController* LocalPlayerController =
		UGameplayStatics::GetPlayerController(this, 0);
	if (!LocalPlayerController ||
		LocalPlayerController->PlayerState != PresentationPlayerState)
	{
		LocalPlayerController = nullptr;
	}

	CustomizationComponent->InitializeForPresentation(
		PresentationPlayerState,
		LocalPlayerController);
}

void AKCLobbyCharacter::RefreshPlayerInfoWidget()
{
	if (!CachedWidgetComp)
	{
		CachedWidgetComp = FindComponentByClass<UWidgetComponent>();
	}

	if (CachedWidgetComp)
	{
		if (UKCPlayerInfoWidget* InfoWidget = Cast<UKCPlayerInfoWidget>(CachedWidgetComp->GetUserWidgetObject()))
		{
			// 1. 정보 갱신
			InfoWidget->UpdatePlayerInfo(PlayerInfo);

			// 2. 머리 위 위젯 가시성 켜기 (블루프린트와 1:1 동일)
			CachedWidgetComp->SetVisibility(true);

			// 기존 재시도 타이머가 있다면 해제
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(WidgetRefreshRetryTimerHandle);
			}
		}
		else
		{
			// 3. 위젯이 아직 생성되지 않은 경우 0.1초 뒤 재시도
			if (UWorld* World = GetWorld())
			{
				if (!World->GetTimerManager().IsTimerActive(WidgetRefreshRetryTimerHandle))
				{
					UE_LOG(LogKCLobby, Verbose, TEXT("[KCLobbyCharacter] UserWidgetObject not ready for '%s', retrying in 0.1s"), *PlayerInfo.PlayerName);
					World->GetTimerManager().SetTimer(
						WidgetRefreshRetryTimerHandle,
						this,
						&AKCLobbyCharacter::RefreshPlayerInfoWidget,
						0.1f,
						false
					);
				}
			}
		}
	}
}

