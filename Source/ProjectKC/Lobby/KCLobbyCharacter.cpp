/**
 * @file KCLobbyCharacter.cpp
 * @brief AKCLobbyCharacter 구현부
 */

#include "ProjectKC/Lobby/KCLobbyCharacter.h"
#include "ProjectKC/Lobby/UI/KCPlayerInfoWidget.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AKCLobbyCharacter::AKCLobbyCharacter()
{
	bReplicates = true;
}

void AKCLobbyCharacter::BeginPlay()
{
	Super::BeginPlay();

	RefreshPlayerInfoWidget();
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
		PlayerInfo = InNewInfo;
		OnRep_PlayerInfo();
	}
}

void AKCLobbyCharacter::OnRep_PlayerInfo()
{
	OnPlayerInfoUpdated.Broadcast(PlayerInfo);
	RefreshPlayerInfoWidget();
}

void AKCLobbyCharacter::RefreshPlayerInfoWidget()
{
	if (UWidgetComponent* WidgetComp = FindComponentByClass<UWidgetComponent>())
	{
		if (UKCPlayerInfoWidget* InfoWidget = Cast<UKCPlayerInfoWidget>(WidgetComp->GetUserWidgetObject()))
		{
			// 1. 정보 갱신
			InfoWidget->UpdatePlayerInfo(PlayerInfo);

			// 2. 머리 위 위젯 가시성 켜기 (블루프린트와 1:1 동일)
			WidgetComp->SetVisibility(true);
		}
		else
		{
			// 3. 위젯이 아직 생성되지 않은 경우 0.1초 뒤 재시도
			if (UWorld* World = GetWorld())
			{
				FTimerHandle RetryTimerHandle;
				World->GetTimerManager().SetTimer(
					RetryTimerHandle,
					this,
					&AKCLobbyCharacter::RefreshPlayerInfoWidget,
					0.1f,
					false
				);
			}
		}
	}
}
