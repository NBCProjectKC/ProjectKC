/**
 * @file KCLobbyPlayerController.h
 * @brief 로비 전용 PlayerController 클래스 정의 (UI 생성, 입력 모드, 슬롯 이동 및 레디 Server RPC)
 */

#pragma once

#include "CoreMinimal.h"
#include "Customization/KCCustomizationSaveGame.h"
#include "GameFramework/PlayerController.h"
#include "KCLobbyPlayerController.generated.h"

class UKCLobbyWidget;
class ACameraActor;
class AKCLobbyCharacter;
class UKCCustomizationNetworkComponent;
class UKCLoadingScreen;
class UKCLoadingTipDataAsset;
class UKCPlayerCustomizationComponent;
class UPaintingModeControllerComponent;
class URuntimeMeshPaintTargetComponent;

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

	/** 로비의 내 캐릭터를 로컬 페인트 편집 대상으로 엽니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Customization")
	bool BeginCustomizationEditing();

	/** 현재 편집 결과를 저장하고 내 로비 외형과 서버에 반영합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Customization")
	bool SaveCustomizationEditing();

	/** 저장 파일은 유지하고 현재 편집 화면만 기본 외형으로 초기화합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Customization")
	bool ResetCustomizationEditing();

	/** 저장하지 않은 변경을 폐기하고 기존 저장 외형을 복원합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Customization")
	bool CancelCustomizationEditing();

	UFUNCTION(BlueprintPure, Category = "KC|Lobby|Customization")
	bool IsCustomizationEditing() const { return bCustomizationEditing; }

	/** 커스터마이징 카메라를 캐릭터 중심으로 회전시킵니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Customization|Camera")
	void OrbitCustomizationCamera(float DeltaYaw, float DeltaPitch);

	/** 양수 입력은 확대, 음수 입력은 축소합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Customization|Camera")
	void ZoomCustomizationCamera(float ZoomDelta);

	/** 커스터마이징 카메라를 최초 구도로 되돌립니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby|Customization|Camera")
	void ResetCustomizationCamera();

	UFUNCTION(BlueprintPure, Category = "KC|Lobby|Customization")
	URuntimeMeshPaintTargetComponent* GetCustomizationEditingPaintTarget() const
	{
		return CustomizationEditingPaintTarget;
	}

	/** UI의 색상·브러시 크기 컨트롤을 연결할 플러그인 컴포넌트입니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby|Customization")
	UPaintingModeControllerComponent* GetCustomizationPaintingController() const
	{
		return CustomizationPaintingController;
	}

	UPROPERTY(BlueprintReadOnly, Category = "KC|Lobby|Customization")
	EKCCustomizationSaveResult LastCustomizationEditingResult =
		EKCCustomizationSaveResult::NoSaveFound;
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

	/** @brief 채팅 메시지가 수신되었을 때 UI 위젯이 감지할 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "KC|Lobby|Chat|Events")
	FOnKCChatMessageReceivedDelegate OnChatMessageReceived;

	/* =========================================================================
	 *  세션 종료 및 퇴장 (Session Termination)
	 * ========================================================================= */

	/** @brief 서버 -> 클라이언트: 방장이 세션을 종료했음을 알리는 Client RPC */
	UFUNCTION(Client, Reliable, Category = "KC|Lobby|Session")
	void Client_NotifySessionTerminated(const FString& Reason);

	/** @brief 방장 권한으로 세션을 종료하고 전원 메인 메뉴로 복귀 (UI 호출 및 콘솔 명령어 겸용) */
	UFUNCTION(Exec, BlueprintCallable, Category = "KC|Lobby|Session")
	void EndSession();

protected:
	//~APlayerController interface
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostSeamlessTravel() override;
	virtual void OnRep_PlayerState() override;
	//~End of APlayerController interface

	/** @brief 로컬 플레이어 대상 로비 UI 위젯 생성 및 마우스/입력 모드 설정 */
	void SetupLobbyUI();

	/** PlayerState 복제 완료 후 기존 로비 표시 캐릭터들의 외형 연결을 갱신합니다. */
	void RefreshLobbyCustomizationPresentations();

	/** @brief 뷰포트에 생성할 로비 메인 UI 위젯 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|UI")
	TSubclassOf<UKCLobbyWidget> LobbyWidgetClass;

	/** @brief 생성된 로비 메인 UI 위젯 인스턴스 */
	UPROPERTY(BlueprintReadOnly, Category = "KC|Lobby|UI")
	TObjectPtr<UKCLobbyWidget> LobbyWidgetInstance;

	/** @brief GasRange 진입 전 표시할 로딩화면 위젯 클래스
	 * Client_OnMatchBegin에서 UKCLoadingScreenSubsystem::BeginPreload()에 전달
	 *  BP_PC_Lobby 디테일 -> WBP_Loading 지정 필요 */
	UPROPERTY(EditDefaultsOnly, Category = "KC|Loading")
	TSubclassOf<UKCLoadingScreen> GasRangeLoadingScreenClass;

	/** @brief GasRange 진입 전 pre-load 할 PrimaryAssetType 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "KC|Loading")
	TArray<FPrimaryAssetType> GasRangePreloadAssetTypes;

	/** @brief 로딩화면에 표시할 팁 문구 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "KC|Loading")
	TObjectPtr<UKCLoadingTipDataAsset> LoadingTipsAsset;

	/** 로비에서도 인게임과 동일한 외형 업로드/다운로드 경로를 사용합니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Customization|Network")
	TObjectPtr<UKCCustomizationNetworkComponent> CustomizationNetworkComponent;

	/** 로비 편집 중 마우스 입력과 브러시 설정을 담당합니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Lobby|Customization")
	TObjectPtr<UPaintingModeControllerComponent> CustomizationPaintingController;

	/** 캐릭터 바운드 중심에서 카메라가 바라볼 위치의 월드 오프셋입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|Customization|Camera")
	FVector CustomizationCameraFocusOffset = FVector(0.0f, 0.0f, 10.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|Customization|Camera",
		meta = (ClampMin = "50.0"))
	float CustomizationCameraInitialDistance = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|Customization|Camera",
		meta = (ClampMin = "10.0"))
	float CustomizationCameraMinimumDistance = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|Customization|Camera",
		meta = (ClampMin = "50.0"))
	float CustomizationCameraMaximumDistance = 650.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|Customization|Camera",
		meta = (ClampMin = "0.0"))
	float CustomizationCameraOrbitSensitivity = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|Customization|Camera",
		meta = (ClampMin = "0.0"))
	float CustomizationCameraZoomSensitivity = 35.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|Customization|Camera")
	float CustomizationCameraInitialPitch = -5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|Customization|Camera")
	float CustomizationCameraMinimumPitch = -55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|Customization|Camera")
	float CustomizationCameraMaximumPitch = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|Customization|Camera",
		meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float CustomizationCameraFieldOfView = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Lobby|Customization|Camera",
		meta = (ClampMin = "0.0"))
	float CustomizationCameraBlendTime = 0.2f;
private:
	AKCLobbyCharacter* ResolveLocalCustomizationCharacter() const;
	class UKCCustomizationSaveSubsystem* GetCustomizationSaveSubsystem() const;
	bool OpenCustomizationCamera(AKCLobbyCharacter* TargetCharacter);
	void UpdateCustomizationCameraTransform();
	void CloseCustomizationCamera();
	void CloseCustomizationEditingSession();

	UPROPERTY(Transient)
	TObjectPtr<UKCPlayerCustomizationComponent> CustomizationEditingComponent;

	UPROPERTY(Transient)
	TObjectPtr<URuntimeMeshPaintTargetComponent> CustomizationEditingPaintTarget;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> CustomizationCameraActor;

	UPROPERTY(Transient)
	TObjectPtr<AKCLobbyCharacter> CustomizationCameraTarget;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PreviousCustomizationViewTarget;

	bool bCustomizationEditing = false;
	float CustomizationCameraYaw = 0.0f;
	float CustomizationCameraPitch = 0.0f;
	float CustomizationCameraDistance = 0.0f;

	/** @brief 도배 방지(쿨타임)를 위한 마지막 메시지 전송 시각 (초 단위) */
	double LastChatMessageTimeSeconds = 0.0;

	/** @brief 채팅 전송 최소 간격 (초 단위, 도배 방지용) */
	static constexpr double ChatCooldownSeconds = 0.5;

	/** @brief 1회 전송 가능한 최대 글자 수 */
	static constexpr int32 MaxChatMessageLength = 100;
};
