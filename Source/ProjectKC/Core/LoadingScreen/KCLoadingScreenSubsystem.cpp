#include "KCLoadingScreenSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "ProjectKC/Core/AssetManager/KCAssetManager.h"
#include "ProjectKC/Messages/KCGameplayTags.h"
#include "ProjectKC/Messages/Struct/KCLevelChangedStruct.h"
#include "ProjectKC/UI/Loading/Screen/KCLoadingScreen.h"
#include "ProjectKC/UI/Loading/ViewModel/KCLoadingViewModel.h"
#include "View/MVVMView.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Lobby/KCSessionSubsystem.h"
#include "Messages/Struct/KCEmptyMessageStruct.h"
#include "UObject/ConstructorHelpers.h"
#include "ProjectKC/UI/Loading/Tip/KCLoadingTipDataAsset.h"
#include "ProjectKC/GameSystem/KCLevelTypeLibrary.h"
#include "ProjectKC/GameSystem/KCLevelInfoRow.h"
#include "Engine/Engine.h"
#include "ProjectKC/ProjectKC.h"
#include "ProjectKC/Messages/Struct/KCGamePhaseChangedStruct.h"
#include "ProjectKC/Player/KCPlayerController.h"

static FString GetSubsystemNetPrefix(const UGameInstanceSubsystem* Subsystem)
{
	if (!Subsystem || !Subsystem->GetGameInstance() || !Subsystem->GetGameInstance()->GetWorld())
	{
		return TEXT("[Unknown]");
	}
	const UWorld* World = Subsystem->GetGameInstance()->GetWorld();
	const FWorldContext* Context = GEngine ? GEngine->GetWorldContextFromWorld(World) : nullptr;
	// PIE에서 세션 연결 전(NM_Standalone) 구간도 PIEInstance로 클라이언트를 정확히 식별한다.
	if (World->GetNetMode() == NM_Client || (Context && Context->PIEInstance > 0))
	{
		const int32 PieInstance = Context ? Context->PIEInstance : 1;
		return FString::Printf(TEXT("[Client PIE_%d]"), PieInstance);
	}
	return TEXT("[Server (Host)]");
}

UKCLoadingScreenSubsystem::UKCLoadingScreenSubsystem()
{
	static ConstructorHelpers::FClassFinder<UKCLoadingScreen> ScreenClassFinder(TEXT("/Game/KC/UI/Screens/WBP_Loading"));
	if (ScreenClassFinder.Succeeded())
	{
		DefaultLoadingScreenClass = ScreenClassFinder.Class;
	}
}

void UKCLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	Collection.InitializeDependency<UKCSessionSubsystem>();   // KCSessionSubsystem이 먼저 초기화되도록 강제
	
	if (UKCSessionSubsystem* SessionSubsystem = GetGameInstance()->GetSubsystem<UKCSessionSubsystem>())
	{
		UE_LOG(LogTemp, Warning, TEXT("[KC_DEBUG11] KCSessionSubsystem 참조 획득 성공, 델리게이트 바인딩 완료"));
		SessionSubsystem->OnJoinSessionComplete.AddDynamic(this,&UKCLoadingScreenSubsystem::HandleSessionJoinComplete);
		SessionSubsystem->OnCreateSessionComplete.AddDynamic(this,&UKCLoadingScreenSubsystem::HandleSessionCreateComplete);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[KC_DEBUG11] KCSessionSubsystem 참조 획득 실패! 델리게이트 바인딩 안 됨"));
	}
	
	DefaultTipsAsset = LoadObject<UKCLoadingTipDataAsset>(nullptr, TEXT("/Game/KC/UI/Screens/DA_LoadingTips.DA_LoadingTips"));
	UE_LOG(LogTemp, Warning, TEXT("[KC_DEBUG] DefaultTipsAsset 로드 결과: %s"), *GetNameSafe(DefaultTipsAsset));
	LevelChangedListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener<FKCLevelChangedStruct>(
		KCGameplayTags::Message_Level_Changed, this, &UKCLoadingScreenSubsystem::OnLevelChangedMessage);
	GamePhaseChangedListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener<FKCGamePhaseChangedStruct>(
		KCGameplayTags::Message_Game_PhaseChanged, this, &UKCLoadingScreenSubsystem::OnGamePhaseChangedMessage);
}

void UKCLoadingScreenSubsystem::Deinitialize()
{
	if (ProgressAnimTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ProgressAnimTickerHandle);
		ProgressAnimTickerHandle.Reset();
	}
	if (HideDelayTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HideDelayTickerHandle);
		HideDelayTickerHandle.Reset();
	}
	if (ControllerTimeoutTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ControllerTimeoutTickerHandle);
		ControllerTimeoutTickerHandle.Reset();
	}
	if (WarmupTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(WarmupTickerHandle);
		WarmupTickerHandle.Reset();
	}
	PendingHiddenCallbacks.Reset();

	if (UKCSessionSubsystem* SessionSubsystem = GetGameInstance()->GetSubsystem<UKCSessionSubsystem>())
	{
		SessionSubsystem->OnJoinSessionComplete.RemoveDynamic(this, &UKCLoadingScreenSubsystem::HandleSessionJoinComplete);
		SessionSubsystem->OnCreateSessionComplete.RemoveDynamic(this, &UKCLoadingScreenSubsystem::HandleSessionCreateComplete);
	}
	
	UGameplayMessageSubsystem::Get(this).UnregisterListener(LevelChangedListenerHandle);
	UGameplayMessageSubsystem::Get(this).UnregisterListener(GamePhaseChangedListenerHandle);
	Super::Deinitialize();
}

void UKCLoadingScreenSubsystem::BeginPreload(EKCLevelType TargetLevel)
{
	if (WaitingForLevel != EKCLevelType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("KCLoadingScreenSubsystem::BeginPreload - 이미 다른 전환(%d)을 기다리는 중에 다시 호출됨. 무시합니다."),
			static_cast<uint8>(WaitingForLevel));
		return;
	}

	const FKCLevelInfoRow* Row = UKCLevelTypeLibrary::GetLevelInfoRow(TargetLevel);
	if (!Row)
	{
		UE_LOG(LogTemp, Warning, TEXT("KCLoadingScreenSubsystem::BeginPreload - DT_LevelInfo에서 레벨 정보를 찾지 못했습니다."));
		return;
	}
	UE_LOG(LogKCGameSystem, Warning, TEXT("[Preload] DT 조회 성공. MapName=%s, AssetTypesToPreload 개수=%d"),
		*Row->MapName.ToString(), Row->AssetTypesToPreload.Num());
	WaitingForLevel = TargetLevel;
	bAssetsReady = false;
	bLevelReady = false;
	bControllerReady = false;
	bReadyReportSent = false;
	PendingHiddenCallbacks.Reset();
	if (ControllerTimeoutTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ControllerTimeoutTickerHandle);
		ControllerTimeoutTickerHandle.Reset();
	}

	const ULocalPlayer* LocalPlayer = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;

	// ============ 1. 뷰모델 준비 ============
	if (!LoadingViewModel)
	{
		LoadingViewModel = NewObject<UKCLoadingViewModel>(this);
	}
	LoadingViewModel->SetProgress(0.0f);
	LoadingViewModel->PickRandomTip(DefaultTipsAsset);
	UpdateLoadingText();
	
	PreloadStartTimeSeconds = FPlatformTime::Seconds();
	ProgressAnimTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UKCLoadingScreenSubsystem::TickProgressAnimation), 0.05f);

	// ============ 2. 위젯 표시 + 뷰모델 연결 ============
	if (LocalPlayer)
	{
		if (APlayerController* PC = LocalPlayer->GetPlayerController(GetGameInstance()->GetWorld()))
		{
			ActiveLoadingWidget = CreateWidget<UKCUserWidget>(PC, DefaultLoadingScreenClass);
			if (ActiveLoadingWidget)
			{
				if (UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get())
				{
					FGameViewportWidgetSlot Slot;
					Slot.bAutoRemoveOnWorldRemoved = false;
					ViewportSubsystem->AddWidgetForPlayer(ActiveLoadingWidget, const_cast<ULocalPlayer*>(LocalPlayer), Slot);
				}

				if (UMVVMView* View = ActiveLoadingWidget->GetExtension<UMVVMView>())
				{
					View->SetViewModel(TEXT("LoadingViewModel"), LoadingViewModel);
				}

				if (UKCLoadingScreen* LoadingScreen = Cast<UKCLoadingScreen>(ActiveLoadingWidget))
				{
					LoadingScreen->SetLoadingViewModel(LoadingViewModel);
				}
			}
		}
	}

	// ============ 3. 에셋 프리로드 시작 ============
	TWeakObjectPtr<UKCLoadingScreenSubsystem> WeakThis(this);

	UKCAssetManager::Get().PreloadAssetsByTypes(
		Row->AssetTypesToPreload,
		[WeakThis](float NewProgress)
		{
			// TODO : 현재 에셋매니저의 부하가 적어 가짜 진행률로 대체. 추후 수정할 예정
			/*if (UKCLoadingScreenSubsystem* StrongThis = WeakThis.Get())
			{
				if (StrongThis->LoadingViewModel)
				{
					const float CappedProgress = FMath::Min(NewProgress * 0.97f, 0.97f);
					StrongThis->LoadingViewModel->SetProgress(CappedProgress);
				}
			}*/
		},
		[WeakThis]()
		{
			if (UKCLoadingScreenSubsystem* StrongThis = WeakThis.Get())
			{
				UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] 에셋 프리로드 완료 (bAssetsReady = true)"));
				StrongThis->bAssetsReady = true;
				StrongThis->TryReportPlayerReadyIfFullyPrepared();
				StrongThis->TryHide();
			}
		});
}

void UKCLoadingScreenSubsystem::OnLevelChangedMessage(FGameplayTag Channel, const FKCLevelChangedStruct& Message)
{
	const FString NetPrefix = GetSubsystemNetPrefix(this);
	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] Message_Level_Changed 수신: NewLevelType=%d, WaitingForLevel=%d"),
		*NetPrefix, static_cast<int32>(Message.NewLevelType), static_cast<int32>(WaitingForLevel));

	if (WaitingForLevel == EKCLevelType::None)
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 대기 중인 레벨이 없어 무시됨"), *NetPrefix);
		return;
	}

	if (Message.NewLevelType != WaitingForLevel)
	{
		// 재접속/난입 시: 대기 레벨(예: 로비)과 다르더라도 유효한 플레이 레벨이면 동적으로 수용
		if (UKCLevelTypeLibrary::IsPlayableLevel(Message.NewLevelType))
		{
			UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 재접속/난입 감지: 대기 레벨(%d) -> 실제 수신 레벨(%d)로 자동 동기화"),
				*NetPrefix, static_cast<int32>(WaitingForLevel), static_cast<int32>(Message.NewLevelType));
			WaitingForLevel = Message.NewLevelType;
		}
		else
		{
			UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 대기 중인 레벨과 불일치하여 무시됨"), *NetPrefix);
			return;
		}
	}

	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 목표 레벨 도착 확인 완료 (bLevelReady = true)"), *NetPrefix);
	bLevelReady = true;
	TryHide();
}

void UKCLoadingScreenSubsystem::TryHide()
{
	const FString NetPrefix = GetSubsystemNetPrefix(this);
	const bool bIsInGame = UKCLevelTypeLibrary::IsInGameLevel(WaitingForLevel);

	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] TryHide 판정: AssetsReady=%s, LevelReady=%s, ControllerReady=%s, WarmupComplete=%s, AllPlayersReady=%s, IsInGame=%s, WaitingLevel=%d"),
		*NetPrefix,
		bAssetsReady ? TEXT("TRUE") : TEXT("FALSE"),
		bLevelReady ? TEXT("TRUE") : TEXT("FALSE"),
		bControllerReady ? TEXT("TRUE") : TEXT("FALSE"),
		bWarmupComplete ? TEXT("TRUE") : TEXT("FALSE"),
		bAllPlayersReadyReceived ? TEXT("TRUE") : TEXT("FALSE"),
		bIsInGame ? TEXT("TRUE") : TEXT("FALSE"),
		static_cast<int32>(WaitingForLevel));

	if (!bAssetsReady || !bLevelReady)
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] TryHide 대기 - 미완료 항목: %s%s"),
			*NetPrefix,
			!bAssetsReady ? TEXT("[에셋 로드] ") : TEXT(""),
			!bLevelReady ? TEXT("[레벨 로드] ") : TEXT(""));
		return;
	}

	// =========================================================================
	// [분기 1] 로비 등 비-전투 레벨: 컨트롤러 도착 -> 3프레임 렌더 웜업 -> 0.3초 후 오픈
	// =========================================================================
	if (!bIsInGame)
	{
		// 1. 에셋과 맵은 준비되었으나 컨트롤러가 아직 도착하지 않은 경우 (네트워크 복제 지연 방어 락)
		if (!bControllerReady)
		{
			if (!ControllerTimeoutTickerHandle.IsValid())
			{
				UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 에셋/맵 준비 완료. 컨트롤러 락 대기 시작 (1.5초 비상 타이머 가동)"), *NetPrefix);
				ControllerTimeoutTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
					FTickerDelegate::CreateUObject(this, &UKCLoadingScreenSubsystem::OnControllerTimeout), 1.5f);
			}
			return;
		}

		if (ControllerTimeoutTickerHandle.IsValid())
		{
			UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 컨트롤러 도착 확인 완료 -> 1.5초 비상 타이머 해제"), *NetPrefix);
			FTSTicker::GetCoreTicker().RemoveTicker(ControllerTimeoutTickerHandle);
			ControllerTimeoutTickerHandle.Reset();
		}

		// 2. 로비 3프레임 렌더 웜업 (캐릭터 커스터마이징 머티리얼/텍스처 히치 흡수)
		if (!bWarmupComplete)
		{
			if (!WarmupTickerHandle.IsValid())
			{
				RemainingWarmupFrames = 3;
				UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] [로비] 컨트롤러 확인 완료 -> 로딩 화면 뒤 3프레임 렌더링 웜업 시작 (남은 프레임: %d)"),
					*NetPrefix, RemainingWarmupFrames);
				WarmupTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
					FTickerDelegate::CreateUObject(this, &UKCLoadingScreenSubsystem::TickRenderWarmup));
			}
			return;
		}

		// 3. 3프레임 웜업 완료 후 0.3초 시각 인지 지연 후 화면 오픈
		if (HideDelayTickerHandle.IsValid())
		{
			return;
		}

		FTSTicker::GetCoreTicker().RemoveTicker(ProgressAnimTickerHandle);

		if (LoadingViewModel)
		{
			LoadingViewModel->SetProgress(1.0f);
			UpdateLoadingText();
			RefreshActiveLoadingWidget();
		}

		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] [로비] 3프레임 웜업 완료! 0.3초 지연 후 화면 오픈"), *NetPrefix);
		HideDelayTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UKCLoadingScreenSubsystem::HideWidgetDelayed), 0.3f);
		return;
	}

	// =========================================================================
	// [분기 2] 인게임 전투 레벨: 3프레임 렌더 웜업 및 전원 동기화
	// =========================================================================
	if (!bWarmupComplete)
	{
		if (!ControllerTimeoutTickerHandle.IsValid())
		{
			UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 인게임 에셋/맵 준비 완료. 폰 시점 안착 및 3프레임 웜업 대기 시작 (10.0초 비상 타이머 가동)"), *NetPrefix);
			ControllerTimeoutTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateUObject(this, &UKCLoadingScreenSubsystem::OnControllerTimeout), 10.0f);
		}
		return;
	}

	if (ControllerTimeoutTickerHandle.IsValid())
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 3프레임 웜업 완료 확인 -> 비상 타이머 해제"), *NetPrefix);
		FTSTicker::GetCoreTicker().RemoveTicker(ControllerTimeoutTickerHandle);
		ControllerTimeoutTickerHandle.Reset();
	}

	// 전원 준비 완료(All Players Ready) 신호 수신 여부 확인
	if (!bAllPlayersReadyReceived)
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ProgressAnimTickerHandle);
		if (LoadingViewModel)
		{
			LoadingViewModel->SetProgress(1.0f);
			LoadingViewModel->SetLoadingText(FText::FromString(TEXT("다른 요리사들을 기다리는 중...")));
			RefreshActiveLoadingWidget();
		}
		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 로컬 3프레임 웜업 완료 -> '다른 요리사들을 기다리는 중...' 표출 및 서버 전원 준비 신호 대기"), *NetPrefix);
		return;
	}

	if (HideDelayTickerHandle.IsValid())
	{
		return;
	}

	FTSTicker::GetCoreTicker().RemoveTicker(ProgressAnimTickerHandle);

	if (LoadingViewModel)
	{
		LoadingViewModel->SetProgress(1.0f);
		LoadingViewModel->SetLoadingText(FText::FromString(TEXT("준비 완료!")));
		RefreshActiveLoadingWidget();
	}

	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 전원 준비 완료! '준비 완료!' 노출 후 %.2f초 시각 인지 지연 티커 가동"), *NetPrefix, PendingHideDelayDuration);
	HideDelayTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UKCLoadingScreenSubsystem::HideWidgetDelayed), PendingHideDelayDuration);
}

bool UKCLoadingScreenSubsystem::OnControllerTimeout(float DeltaTime)
{
	const FString NetPrefix = GetSubsystemNetPrefix(this);
	ControllerTimeoutTickerHandle.Reset();

	const bool bIsInGame = UKCLevelTypeLibrary::IsInGameLevel(WaitingForLevel);
	if (bIsInGame)
	{
		if (!bWarmupComplete)
		{
			UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 플레이어 웜업 비상 타임아웃(10초) 만료! 강제로 웜업을 완료 처리하고 진행합니다."), *NetPrefix);
			bWarmupComplete = true;
			TryReportPlayerReadyIfFullyPrepared();
			TryHide();
		}
	}
	else
	{
		if (!bControllerReady)
		{
			UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 컨트롤러 락 타임아웃(1.5초) 만료! 컨트롤러 미도착 상태로 로딩화면을 닫습니다."), *NetPrefix);
			bControllerReady = true;
			TryHide();
		}
	}

	return false;
}

void UKCLoadingScreenSubsystem::FinishAndHideLoadingScreen()
{
	const FString NetPrefix = GetSubsystemNetPrefix(this);

	if (ProgressAnimTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ProgressAnimTickerHandle);
		ProgressAnimTickerHandle.Reset();
	}
	if (HideDelayTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HideDelayTickerHandle);
		HideDelayTickerHandle.Reset();
	}
	if (ControllerTimeoutTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ControllerTimeoutTickerHandle);
		ControllerTimeoutTickerHandle.Reset();
	}
	if (WarmupTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(WarmupTickerHandle);
		WarmupTickerHandle.Reset();
	}

	if (ActiveLoadingWidget)
	{
		ActiveLoadingWidget->RemoveFromParent();
		ActiveLoadingWidget = nullptr;
	}

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(KCGameplayTags::Message_LoadingScreen_Hidden, FKCEmptyMessageStruct());

	TArray<FSimpleDelegate> CallbacksToExecute = MoveTemp(PendingHiddenCallbacks);
	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 대기 중이던 콜백(%d개) 순차 실행 시작"), *NetPrefix, CallbacksToExecute.Num());
	for (const FSimpleDelegate& Callback : CallbacksToExecute)
	{
		Callback.ExecuteIfBound();
	}

	WaitingForLevel = EKCLevelType::None;
	bAssetsReady = false;
	bLevelReady = false;
	bControllerReady = false;
	bWarmupComplete = false;
	bAllPlayersReadyReceived = false;
	bReadyReportSent = false;
	RemainingWarmupFrames = 0;
	RegisteredController.Reset();
	LoadingViewModel = nullptr;

	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 로딩화면 종료 및 모든 상태 플래그/티커 완벽 초기화 완료"), *NetPrefix);
}

bool UKCLoadingScreenSubsystem::HideWidgetDelayed(float DeltaTime)
{
	const FString NetPrefix = GetSubsystemNetPrefix(this);
	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 시각 인지 지연 만료 -> 로딩 화면 위젯 제거 및 GMR 브로드캐스트"), *NetPrefix);
	FinishAndHideLoadingScreen();
	return false;
}
bool UKCLoadingScreenSubsystem::TickProgressAnimation(float DeltaTime)
{
	if (!LoadingViewModel)
	{
		return true;
	}

	const double Elapsed = FPlatformTime::Seconds() - PreloadStartTimeSeconds;
	const float FakeProgress = FMath::Min(static_cast<float>(Elapsed / MinDisplayDurationSeconds) * 0.97f, 0.97f);
	LoadingViewModel->SetProgress(FakeProgress);
	UpdateLoadingText(); // 진행률 오를 때마다 체크해서 조건 맞으면 갱신
	RefreshActiveLoadingWidget();

	return true;
}

void UKCLoadingScreenSubsystem::UpdateLoadingText()
{
	if (!LoadingViewModel)
	{
		return;
	}
	
	if (bAssetsReady && bLevelReady)
	{
		const bool bIsInGame = UKCLevelTypeLibrary::IsInGameLevel(WaitingForLevel);
		if (bIsInGame && bWarmupComplete && !bAllPlayersReadyReceived)
		{
			LoadingViewModel->SetLoadingText(FText::FromString(TEXT("다른 요리사들을 기다리는 중...")));
			return;
		}
		else if (!bIsInGame || (bWarmupComplete && bAllPlayersReadyReceived))
		{
			LoadingViewModel->SetLoadingText(FText::FromString(TEXT("준비 완료!")));
			return;
		}
	}

	const float CurrentProgress = LoadingViewModel->GetProgress();
	
	if (WaitingForLevel == EKCLevelType::LobbyLevel)
	{
		LoadingViewModel->SetLoadingText(FText::FromString(TEXT("로비로 이동하는 중...")));
		return;
	}
	
	if (CurrentProgress >= 0.97f)
	{
		LoadingViewModel->SetLoadingText(FText::FromString(TEXT("맵 불러오는 중...")));
		return;
	}

	// 97% 미만 구간을 에셋 종류별 문구로 3등분
	if (CurrentProgress < 0.32f)
	{
		LoadingViewModel->SetLoadingText(FText::FromString(TEXT("아이템 준비 중...")));
	}
	else if (CurrentProgress < 0.64f)
	{
		LoadingViewModel->SetLoadingText(FText::FromString(TEXT("이펙트 준비 중...")));
	}
	else
	{
		LoadingViewModel->SetLoadingText(FText::FromString(TEXT("사운드 준비 중...")));
	}
}

void UKCLoadingScreenSubsystem::RefreshActiveLoadingWidget()
{
	if (UKCLoadingScreen* LoadingScreen = Cast<UKCLoadingScreen>(ActiveLoadingWidget))
	{
		LoadingScreen->RefreshFromViewModel();
	}
}

void UKCLoadingScreenSubsystem::CancelPreload()
{
	const FString NetPrefix = GetSubsystemNetPrefix(this);
	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] CancelPreload 호출됨 -> 로딩화면 강제종료 및 상태 초기화"), *NetPrefix);
	
	// 1. 진행 중인 애니메이션/지연 티커 제거
	if (ProgressAnimTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ProgressAnimTickerHandle);
		ProgressAnimTickerHandle.Reset();
	}
	if (HideDelayTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HideDelayTickerHandle);
		HideDelayTickerHandle.Reset();
	}
	if (ControllerTimeoutTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ControllerTimeoutTickerHandle);
		ControllerTimeoutTickerHandle.Reset();
	}
	if (WarmupTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(WarmupTickerHandle);
		WarmupTickerHandle.Reset();
	}
	
	// 2. 화면의 로딩위젯 즉시 제거
	if (ActiveLoadingWidget)
	{
		ActiveLoadingWidget->RemoveFromParent();
		ActiveLoadingWidget = nullptr;
	}
	
	// 3. 뷰모델 및 서브시스템 상태 초기화
	WaitingForLevel = EKCLevelType::None;
	bAssetsReady = false;
	bLevelReady = false;
	bControllerReady = false;
	bWarmupComplete = false;
	bAllPlayersReadyReceived = false;
	bReadyReportSent = false;
	RemainingWarmupFrames = 0;
	RegisteredController.Reset();
	PendingHiddenCallbacks.Reset();
	LoadingViewModel = nullptr;
}

void UKCLoadingScreenSubsystem::HandleSessionJoinComplete(bool bWasSuccessful, const FString& ConnectString)
{
	const FString NetPrefix = GetSubsystemNetPrefix(this);
	if (!bWasSuccessful)
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [Preload] JoinSession 실패 감지 -> 로딩화면 취소"), *NetPrefix);
		CancelPreload();
	}
}

void UKCLoadingScreenSubsystem::HandleSessionCreateComplete(bool bWasSuccessful)
{
	const FString NetPrefix = GetSubsystemNetPrefix(this);
	if (!bWasSuccessful)
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [Preload] CreateSession 실패 감지 -> 로딩화면 취소"), *NetPrefix);
		CancelPreload();
	}
}

void UKCLoadingScreenSubsystem::RunAfterLoadingScreenHidden(UObject* WorldContextObject, FSimpleDelegate Callback)
{
	const FString NetPrefix = GetSubsystemNetPrefix(this);
	if (!ActiveLoadingWidget)
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] RunAfterLoadingScreenHidden - 이미 로딩화면 없음, 즉시 실행"), *NetPrefix);
		Callback.ExecuteIfBound();
		return;
	}

	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] RunAfterLoadingScreenHidden - 컨트롤러 도착, 대기열 등록 및 락(Lock) 해제"), *NetPrefix);
	PendingHiddenCallbacks.Add(Callback);
	bControllerReady = true;

	// 컨트롤러가 유효한 플레이 월드에서 호출한 경우, 레벨 로드 완료 보장 (재접속/난입 상황 자동 치유)
	// 에셋 준비 여부(bAssetsReady)는 오직 BeginPreload()의 실제 프리로드 완료 콜백만이 결정한다 (SSOT).
	if (WorldContextObject)
	{
		if (const UWorld* World = WorldContextObject->GetWorld())
		{
			const EKCLevelType CurrentLevelType = UKCLevelTypeLibrary::GetLevelTypeFromWorld(World);
			if (UKCLevelTypeLibrary::IsPlayableLevel(CurrentLevelType))
			{
				if (!bLevelReady || WaitingForLevel != CurrentLevelType)
				{
					UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] RunAfterLoadingScreenHidden - 컨트롤러 월드 감지: %d (기존 대기: %d) -> 레벨 준비 완료 처리"),
						*NetPrefix, static_cast<int32>(CurrentLevelType), static_cast<int32>(WaitingForLevel));
					WaitingForLevel = CurrentLevelType;
					bLevelReady = true;
				}
			}
		}
	}

	if (ControllerTimeoutTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ControllerTimeoutTickerHandle);
		ControllerTimeoutTickerHandle.Reset();
	}

	TryHide();
}

void UKCLoadingScreenSubsystem::NotifyPlayerReady(APlayerController* Controller)
{
	const FString NetPrefix = GetSubsystemNetPrefix(this);
	if (!ActiveLoadingWidget)
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] NotifyPlayerReady 호출됨 - 활성 로딩 화면 위젯이 없습니다. 즉시 로컬 완료 보고."), *NetPrefix);
		if (AKCPlayerController* KCController = Cast<AKCPlayerController>(Controller))
		{
			KCController->NotifyLocalLoadingAndWarmupComplete();
		}
		return;
	}

	if (bWarmupComplete)
	{
		// 비상 타임아웃 등으로 웜업이 이미 강제 완료 처리된 뒤에 진짜 폰 빙의가 일어난 경우.
		// RegisteredController를 지금이라도 등록하지 않으면 이 플레이어의 서버 보고가 영원히 나가지 않는다.
		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] NotifyPlayerReady 호출됨 - 이미 웜업 완료 상태, 지금 컨트롤러 등록 후 보고 시도"), *NetPrefix);
		RegisteredController = Controller;
		TryReportPlayerReadyIfFullyPrepared();
		return;
	}

	if (WarmupTickerHandle.IsValid())
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] NotifyPlayerReady 호출됨 - 이미 웜업 티커가 동작 중입니다."), *NetPrefix);
		return;
	}

	RegisteredController = Controller;
	RemainingWarmupFrames = 3;

	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 로컬 컨트롤러 폰 빙의 완료 확인 -> 로딩 화면 뒤 3프레임 렌더링 웜업 시작 (남은 프레임: %d)"),
		*NetPrefix, RemainingWarmupFrames);

	WarmupTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UKCLoadingScreenSubsystem::TickRenderWarmup));
}

bool UKCLoadingScreenSubsystem::TickRenderWarmup(float DeltaTime)
{
	const FString NetPrefix = GetSubsystemNetPrefix(this);
	RemainingWarmupFrames--;
	UE_LOG(LogKCGameSystem, Log, TEXT("%s [LoadingScreen] 렌더링 웜업 틱 진행 중... (남은 프레임: %d)"), *NetPrefix, RemainingWarmupFrames);

	if (RemainingWarmupFrames <= 0)
	{
		WarmupTickerHandle.Reset();
		bWarmupComplete = true;

		UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 3프레임 렌더링 웜업 완료! (셰이더 컴파일/외형 텍스처 히치 소화 완료)"), *NetPrefix);

		TryReportPlayerReadyIfFullyPrepared();
		TryHide();
		return false;
	}

	return true;
}

void UKCLoadingScreenSubsystem::TryReportPlayerReadyIfFullyPrepared()
{
	// 로비 흐름(RegisteredController 미등록)은 대상이 아니며, 렌더 웜업과 에셋 프리로드가
	// 둘 다 끝났을 때 딱 한 번만 서버에 보고한다. 어느 쪽이 먼저 끝나든 이 함수가
	// 양쪽 이벤트 지점(TickRenderWarmup, 에셋 프리로드 완료 콜백)에서 호출되므로 안전하다.
	if (!RegisteredController.IsValid() || bReadyReportSent || !bWarmupComplete || !bAssetsReady)
	{
		return;
	}

	AKCPlayerController* KCController = Cast<AKCPlayerController>(RegisteredController.Get());
	if (!KCController)
	{
		return;
	}

	// 실제로 보고를 보낼 수 있을 때만 "보냈다"고 표시한다 (조기 세팅 시 캐스팅 실패하면 영구히 재시도 불가능해짐).
	bReadyReportSent = true;

	const FString NetPrefix = GetSubsystemNetPrefix(this);
	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 렌더 웜업 + 에셋 프리로드 모두 완료 -> 서버에 로딩 완료 보고 진행"), *NetPrefix);
	KCController->NotifyLocalLoadingAndWarmupComplete();
}

void UKCLoadingScreenSubsystem::NotifyAllPlayersReady(float DisplayDuration)
{
	const FString NetPrefix = GetSubsystemNetPrefix(this);
	PendingHideDelayDuration = FMath::Max(0.05f, DisplayDuration);
	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] NotifyAllPlayersReady 수신! (전원 웜업 완료, 노출 시간: %.2f초) -> '준비 완료!' 노출 및 화면 닫기 진행"),
		*NetPrefix, PendingHideDelayDuration);
	bAllPlayersReadyReceived = true;
	TryHide();
}

void UKCLoadingScreenSubsystem::OnGamePhaseChangedMessage(FGameplayTag Channel, const FKCGamePhaseChangedStruct& Message)
{
	const FString NetPrefix = GetSubsystemNetPrefix(this);
	UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] GMR GamePhaseChanged 수신: %d"), *NetPrefix, static_cast<int32>(Message.NewPhase));

	if (Message.NewPhase == EKCGamePhaseType::Countdown || Message.NewPhase == EKCGamePhaseType::Playing)
	{
		// [보완 1] 위젯이 아직 남아있는 비상 상황에서만 단 1회 실행하여 중복 발화 및 stale 누수 원천 차단
		if (ActiveLoadingWidget)
		{
			UE_LOG(LogKCGameSystem, Warning, TEXT("%s [LoadingScreen] 카운트다운/게임시작 단계 진입 감지 -> 남은 로딩 화면 강제 완벽 정리"), *NetPrefix);
			FinishAndHideLoadingScreen();
		}
	}
}