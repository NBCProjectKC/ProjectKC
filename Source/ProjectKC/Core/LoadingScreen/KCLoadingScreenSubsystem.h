#pragma once
 
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectKC/GameSystem/Enum/KCLevelType.h"
#include "KCLoadingScreenSubsystem.generated.h"
 
class UKCLoadingScreen;
class UKCLoadingViewModel;
struct FKCLevelChangedStruct;
struct FKCGamePhaseChangedStruct;
class UKCLoadingTipDataAsset;
class UKCUserWidget;
class APlayerController;

UCLASS(Blueprintable)
class PROJECTKC_API UKCLoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
 
public:
	UKCLoadingScreenSubsystem();
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
 
	/**
	 * @param TargetLevel   타겟 레벨 타입 (예: EKCLevelType::PortableGasStove).
	 *                      나중에 "GasRange → 결과화면" 같은 다른 전환에도 이 함수를
	 *                      재사용할 수 있도록 함
	 */
	UFUNCTION(BlueprintCallable, Category = "KC|Loading")
	void BeginPreload(EKCLevelType TargetLevel);
	
	/** 세션 참가 실패나 네트워크 에러 발생 시 로딩 화면을 즉시 정리 */
	UFUNCTION(BlueprintCallable, Category = "KC|Loading")
	void CancelPreload();

	/**
	 * 로컬 컨트롤러가 폰 빙의 및 외형 세팅 완료 후 호출 (인게임 전투 레벨 전용).
	 * 로딩 화면 뒤에서 3프레임 렌더링 웜업을 수행하고, 완료 시 서버에 보고하도록 합니다.
	 */
	void NotifyPlayerReady(APlayerController* Controller);

	/**
	 * 서버에서 전원 웜업 완료 신호를 수신했을 때 호출.
	 * 문구를 "준비 완료!"로 변경하고 서버가 지정한 DisplayDuration 초 후 로딩 화면을 내리도록 지시합니다.
	 */
	void NotifyAllPlayersReady(float DisplayDuration = 0.3f);
 
	UPROPERTY(Transient)
	TObjectPtr<UKCUserWidget> ActiveLoadingWidget;
	
	/** 로딩화면에 항상 쓰이는 기본 위젯 클래스 (에디터에서 지정) */
	UPROPERTY(EditDefaultsOnly, Category = "KC|Loading")
	TSubclassOf<UKCLoadingScreen> DefaultLoadingScreenClass;

	/** 로딩화면에 항상 쓰이는 기본 팁 데이터 애셋 (에디터에서 지정, 없으면 nullptr) */
	UPROPERTY(EditDefaultsOnly, Category = "KC|Loading")
	TObjectPtr<UKCLoadingTipDataAsset> DefaultTipsAsset;
	
	/**
	* 로딩화면이 이미 끝나있으면 즉시 Callback을 실행하고,
	* 아직 떠있으면 로딩화면이 끝나는 시점에 Callback을 실행하도록 예약한다.
	* 호스트/클라이언트의 BeginPlay 타이밍 차이까지 커버
	*/
	void RunAfterLoadingScreenHidden(UObject* WorldContextObject, FSimpleDelegate Callback);
private:
	//Message_Level_Changed 받는 콜백
	void OnLevelChangedMessage(FGameplayTag Channel, const FKCLevelChangedStruct& Message);
 
	
	//bAssetsReady&&bLevelReady==true 시 위젯 hide
	void TryHide();
	
	//대기중인 레벨, None : 대기 중이 아님
	EKCLevelType WaitingForLevel = EKCLevelType::None;
 
	// 에셋 프리로드 준비여부 (AssetManager의 OnComplete 콜백->true)
	bool bAssetsReady = false;
 
	// 목표 레벨에 진입여부 (OnLevelChangedMessage->true)
	bool bLevelReady = false;

	// 플레이어 컨트롤러가 스폰되어 BeginPlay를 탔는지 여부 (네트워크 복제 지연 방어 락)
	bool bControllerReady = false;

	// 인게임 전투 레벨: 로컬 폰 시점 안착 및 3프레임 렌더링 웜업 완료 여부
	bool bWarmupComplete = false;

	// 인게임 전투 레벨: 서버로부터 전원 준비 완료(All Players Ready) 수신 여부
	bool bAllPlayersReadyReceived = false;

	// 3프레임 렌더링 웜업 티커
	bool TickRenderWarmup(float DeltaTime);
	FTSTicker::FDelegateHandle WarmupTickerHandle;
	int32 RemainingWarmupFrames = 0;
	TWeakObjectPtr<APlayerController> RegisteredController;

	// 인게임 전투 레벨: 서버에 로딩 완료를 이미 보고했는지 여부 (중복 보고 방지)
	bool bReadyReportSent = false;

	/**
	 * 렌더 웜업과 에셋 프리로드는 서로 다른 타이밍에 끝나는 별개의 비동기 작업이다.
	 * 둘 중 어느 쪽이 나중에 끝나든, 이 함수가 호출된 시점에 둘 다 완료됐는지 확인해서
	 * 그때 딱 한 번만 서버에 로딩 완료를 보고한다 (웜업만 끝났다고 조기 보고하지 않도록 방지).
	 */
	void TryReportPlayerReadyIfFullyPrepared();

	// 로딩화면이 내려간 후 실행 대기 중인 콜백 목록
	TArray<FSimpleDelegate> PendingHiddenCallbacks;
 
	/**
	 * 로딩 진행률(0.0~1.0)과 텍스트를 화면에 뿌리기 위한 뷰모델.
	 * UMVVMViewModelBase를 상속한 클래스 -> UMG 쪽에서 FieldNotify 바인딩
	 * 값이 바뀔 때마다 자동으로 화면 갱신 (MVVM 패턴)
	 */
	UPROPERTY(Transient)
	TObjectPtr<UKCLoadingViewModel> LoadingViewModel;
	
	// 프로그레스바 최소 노출시간
	UPROPERTY(EditDefaultsOnly, Category = "KC|Loading")
	float MinDisplayDurationSeconds = 1.5f;

	double PreloadStartTimeSeconds = 0.0;
	
	FTSTicker::FDelegateHandle ProgressAnimTickerHandle;
	bool TickProgressAnimation(float DeltaTime);
	
	// 100% 노출을 위한 파괴 지연 티커
	FTSTicker::FDelegateHandle HideDelayTickerHandle;
	float PendingHideDelayDuration = 0.3f;
	bool HideWidgetDelayed(float DeltaTime);

	/** 로딩화면 정상 종료 및 비상 안전장치 공용 완전 정리 함수 */
	void FinishAndHideLoadingScreen();

	// 컨트롤러 미도착 시 무한 로딩 방지용 비상 탈출 티커
	FTSTicker::FDelegateHandle ControllerTimeoutTickerHandle;
	bool OnControllerTimeout(float DeltaTime);
	
	// GMS 핸들
	FGameplayMessageListenerHandle LevelChangedListenerHandle;
	FGameplayMessageListenerHandle GamePhaseChangedListenerHandle;
	void OnGamePhaseChangedMessage(FGameplayTag Channel, const FKCGamePhaseChangedStruct& Message);
	
	/** bAssetsReady/bLevelReady 상태에 맞춰 LoadingText를 갱신 
	* 97% 미만 : 아이템, 사운드 효과 준비 중... 문구
	* 97%~ : 맵 불러오는 중...
	* bAssetsReady/bLevelReady 둘 다 true일 때 : 준비 완료!
	*/
    void UpdateLoadingText();
	void RefreshActiveLoadingWidget();
	
	// 클라이언트의 세션 Join 실패 델리게이트 핸들러
	UFUNCTION()
	void HandleSessionJoinComplete(bool bWasSuccessful, const FString& ConnectString);
	UFUNCTION()
	void HandleSessionCreateComplete(bool bWasSuccessful);
};
 
