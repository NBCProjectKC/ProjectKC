/**
 * @file KCLobbyPlayerController.h
 * @brief 로비 전용 PlayerController 클래스 정의 (UI 생성, 입력 모드, 서버 RPC 통신)
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "KCLobbyPlayerController.generated.h"

class UKCLobbyWidget;

/**
 * @class AKCLobbyPlayerController
 * @brief 기존 BP_PC_Lobby 블루프린트를 1:1 매핑한 C++ PlayerController 클래스
 */
UCLASS()
class PROJECTKC_API AKCLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AKCLobbyPlayerController();

	/** @brief 클라이언트가 서버에 준비(Ready) 상태 토글을 요청하는 Server RPC */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "KC|Lobby")
	void ROS_ToggleReadyStatus();

	/** @brief 클라이언트가 서버에 플레이어 정보 동기화를 요청하는 Server RPC */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "KC|Lobby")
	void ROS_UpdatePlayerInfo();

	/** @brief 게임 시작 시 모든 클라이언트의 입력을 비활성화하고 시작 애니메이션을 재생하는 Client RPC */
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "KC|Lobby")
	void Client_OnMatchBegin();

	/** @brief 전원 준비 완료 여부에 따라 방장 UI의 StartGame 버튼 활성화 상태를 동기화하는 Client RPC */
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "KC|Lobby")
	void Client_SetStartGameButtonEnabled(bool bEnabled);

protected:
	//~APlayerController interface
	virtual void BeginPlay() override;
	//~End of APlayerController interface

	/** @brief 뷰포트에 생성할 로비 메인 UI 위젯 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|UI")
	TSubclassOf<UKCLobbyWidget> LobbyWidgetClass;

	/** @brief 생성된 로비 메인 UI 위젯 인스턴스 */
	UPROPERTY(BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UKCLobbyWidget> LobbyWidgetInstance;
};
