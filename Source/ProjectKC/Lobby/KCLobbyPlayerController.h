/**
 * @file KCLobbyPlayerController.h
 * @brief 로비 전용 PlayerController 클래스 정의 (UI 생성, 입력 모드, 슬롯 이동 및 레디 Server RPC)
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "KCLobbyPlayerController.generated.h"

class UKCLobbyWidget;
class UKCCustomizationNetworkComponent;

/**
 * @brief 채팅 메시지 수신 시 UI(위젯)에 브로드캐스트할 다이나믹 멀티캐스트 델리게이트
 * @param SenderName 메시지를 보낸 플레이어의 닉네임 (스팀 닉네임)
 * @param Message 채팅 내용
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKCChatMessageReceivedDelegate, const FString&, SenderName, const FString&, Message);

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

	/** @brief 클라이언트가 서버에 특정 슬롯(TargetSlotIndex: 0~5)으로 1:1 자리 이동을 요청하는 Server RPC */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "KC|Lobby")
	void ROS_RequestMoveToSlot(int32 TargetSlotIndex);

	/** @brief 클라이언트가 서버에 플레이어 정보 동기화를 요청하는 Server RPC */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "KC|Lobby")
	void ROS_UpdatePlayerInfo();

	/** @brief 게임 시작 시 모든 클라이언트의 입력을 비활성화하고 시작 애니메이션을 재생하는 Client RPC */
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "KC|Lobby")
	void Client_OnMatchBegin();

	/** @brief 전원 준비 완료 여부에 따라 방장 UI의 StartGame 버튼 활성화 상태를 동기화하는 Client RPC */
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "KC|Lobby")
	void Client_SetStartGameButtonEnabled(bool bEnabled);

	UKCCustomizationNetworkComponent* GetCustomizationNetworkComponent() const
	{
		return CustomizationNetworkComponent;
	}
	/* =========================================================================
	 *  로비 채팅 시스템 (Lobby Chat System)
	 * ========================================================================= */

	/**
	 * @brief 로컬에서 채팅 메시지를 전송하는 블루프린트 호출용 진입 함수
	 * @param Message 보낼 메시지 문자열
	 */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Chat")
	void SendChatMessage(const FString& Message);

	/**
	 * @brief 개발용 콘솔 명령어로 채팅을 즉시 보낼 수 있는 치트/Exec 함수
	 * @note 콘솔 창(`)에 'SendChat 안녕하세요' 형식으로 입력하여 UI 없이 즉시 테스트 가능
	 */
	UFUNCTION(Exec, Category = "KC|Lobby|Chat")
	void SendChat(const FString& Message);

	/** @brief 클라이언트 -> 서버: 채팅 메시지 전송 요청 Server RPC */
	UFUNCTION(Server, Reliable, WithValidation, Category = "KC|Lobby|Chat")
	void Server_SendChatMessage(const FString& Message);

	/** @brief 서버 -> 클라이언트: 전파된 채팅 메시지 수신 Client RPC */
	UFUNCTION(Client, Reliable, Category = "KC|Lobby|Chat")
	void Client_ReceiveChatMessage(const FString& SenderName, const FString& Message);

	/** @brief 채팅 메시지가 수신되었을 때 승재님의 UI 위젯이 감지할 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "KC|Lobby|Chat|Events")
	FOnKCChatMessageReceivedDelegate OnChatMessageReceived;

protected:
	//~APlayerController interface
	virtual void BeginPlay() override;
	virtual void PostSeamlessTravel() override;
	virtual void OnRep_PlayerState() override;
	//~End of APlayerController interface

	/** @brief 로컬 플레이어 대상 로비 UI 위젯 생성 및 마우스/입력 모드 설정 */
	void SetupLobbyUI();

	/** @brief 뷰포트에 생성할 로비 메인 UI 위젯 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|UI")
	TSubclassOf<UKCLobbyWidget> LobbyWidgetClass;

	/** @brief 생성된 로비 메인 UI 위젯 인스턴스 */
	UPROPERTY(BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UKCLobbyWidget> LobbyWidgetInstance;

	/** 로비에서도 인게임과 동일한 외형 업로드/다운로드 경로를 사용합니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Customization|Network")
	TObjectPtr<UKCCustomizationNetworkComponent> CustomizationNetworkComponent;
private:
	/** @brief 도배 방지(쿨타임)를 위한 마지막 메시지 전송 시각 (초 단위) */
	double LastChatMessageTimeSeconds = 0.0;

	/** @brief 채팅 전송 최소 간격 (초 단위, 도배 방지용) */
	static constexpr double ChatCooldownSeconds = 0.5;

	/** @brief 1회 전송 가능한 최대 글자 수 */
	static constexpr int32 MaxChatMessageLength = 100;
};
