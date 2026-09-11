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
#include "ProjectKC/ProjectKC.h"

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
	PendingHiddenCallbacks.Reset();

	if (UKCSessionSubsystem* SessionSubsystem = GetGameInstance()->GetSubsystem<UKCSessionSubsystem>())
	{
		SessionSubsystem->OnJoinSessionComplete.RemoveDynamic(this, &UKCLoadingScreenSubsystem::HandleSessionJoinComplete);
		SessionSubsystem->OnCreateSessionComplete.RemoveDynamic(this, &UKCLoadingScreenSubsystem::HandleSessionCreateComplete);
	}
	
	UGameplayMessageSubsystem::Get(this).UnregisterListener(LevelChangedListenerHandle);
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
				StrongThis->TryHide();
			}
		});
}

void UKCLoadingScreenSubsystem::OnLevelChangedMessage(FGameplayTag Channel, const FKCLevelChangedStruct& Message)
{
	UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] Message_Level_Changed 수신: NewLevelType=%d, WaitingForLevel=%d"),
		static_cast<int32>(Message.NewLevelType), static_cast<int32>(WaitingForLevel));

	if (WaitingForLevel == EKCLevelType::None)
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] 대기 중인 레벨이 없어 무시됨"));
		return;
	}

	if (Message.NewLevelType != WaitingForLevel)
	{
		// 재접속/난입 시: 대기 레벨(예: 로비)과 다르더라도 유효한 플레이 레벨이면 동적으로 수용
		if (UKCLevelTypeLibrary::IsPlayableLevel(Message.NewLevelType))
		{
			UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] 재접속/난입 감지: 대기 레벨(%d) -> 실제 수신 레벨(%d)로 자동 동기화"),
				static_cast<int32>(WaitingForLevel), static_cast<int32>(Message.NewLevelType));
			WaitingForLevel = Message.NewLevelType;
		}
		else
		{
			UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] 대기 중인 레벨과 불일치하여 무시됨"));
			return;
		}
	}

	UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] 목표 레벨 도착 확인 완료 (bLevelReady = true)"));
	bLevelReady = true;
	TryHide();
}

void UKCLoadingScreenSubsystem::TryHide()
{
	UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] TryHide 판정: AssetsReady=%s, LevelReady=%s, ControllerReady=%s"),
		bAssetsReady ? TEXT("TRUE") : TEXT("FALSE"),
		bLevelReady ? TEXT("TRUE") : TEXT("FALSE"),
		bControllerReady ? TEXT("TRUE") : TEXT("FALSE"));

	if (!bAssetsReady || !bLevelReady)
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] TryHide 대기 - 미완료 항목: %s%s"),
			!bAssetsReady ? TEXT("[에셋 로드] ") : TEXT(""),
			!bLevelReady ? TEXT("[레벨 로드] ") : TEXT(""));
		return;
	}

	// 에셋과 맵은 준비되었으나 컨트롤러가 아직 도착하지 않은 경우 (네트워크 복제 지연 방어 락)
	if (!bControllerReady)
	{
		if (!ControllerTimeoutTickerHandle.IsValid())
		{
			UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] 에셋/맵 준비 완료. 컨트롤러 락 대기 시작 (1.5초 비상 타이머 가동)"));
			ControllerTimeoutTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateUObject(this, &UKCLoadingScreenSubsystem::OnControllerTimeout), 1.5f);
		}
		return;
	}

	// 컨트롤러까지 도착 확인 완료 -> 비상 타이머 정리
	if (ControllerTimeoutTickerHandle.IsValid())
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] 컨트롤러 도착 확인 완료 -> 1.5초 비상 타이머 해제"));
		FTSTicker::GetCoreTicker().RemoveTicker(ControllerTimeoutTickerHandle);
		ControllerTimeoutTickerHandle.Reset();
	}

	if (HideDelayTickerHandle.IsValid())
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] 이미 0.3초 지연 티커 가동 중"));
		return;
	}

	FTSTicker::GetCoreTicker().RemoveTicker(ProgressAnimTickerHandle);

	if (LoadingViewModel)
	{
		LoadingViewModel->SetProgress(1.0f);
		UpdateLoadingText(); // Loading Text : "준비 완료!"
		RefreshActiveLoadingWidget();
	}

	// 100%가 화면에 실제로 그려질 시간을 준 다음에 위젯 떼어냄
	UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] 3가지 조건 모두 충족! 100%% 노출 후 0.3초 지연 티커 가동"));
	HideDelayTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UKCLoadingScreenSubsystem::HideWidgetDelayed), 0.3f);

	WaitingForLevel = EKCLevelType::None;
}

bool UKCLoadingScreenSubsystem::OnControllerTimeout(float DeltaTime)
{
	ControllerTimeoutTickerHandle.Reset();

	if (!bControllerReady)
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] 컨트롤러 1.5초 타임아웃 만료! 강제로 락을 해제하고 로딩화면을 닫습니다."));
		bControllerReady = true;
		TryHide();
	}

	return false;
}

bool UKCLoadingScreenSubsystem::HideWidgetDelayed(float DeltaTime)
{
	HideDelayTickerHandle.Reset();

	UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] 0.3초 지연 만료 -> 로딩 화면 위젯 제거 및 GMR 브로드캐스트"));

	if (ActiveLoadingWidget)
	{
		ActiveLoadingWidget->RemoveFromParent();
		ActiveLoadingWidget = nullptr;
	}
	// 로딩화면 Hide 후 알림 -> 외부 리스너용
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(KCGameplayTags::Message_LoadingScreen_Hidden, FKCEmptyMessageStruct());
	
	// 대기 중인 UI 생성 콜백 일괄 실행 및 비우기
	TArray<FSimpleDelegate> CallbacksToExecute = MoveTemp(PendingHiddenCallbacks);
	UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] 대기 중이던 콜백(%d개) 순차 실행 시작"), CallbacksToExecute.Num());
	for (const FSimpleDelegate& Callback : CallbacksToExecute)
	{
		Callback.ExecuteIfBound();
	}

	bAssetsReady = false;
	bLevelReady = false;
	bControllerReady = false;

	UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] 로딩화면 종료 및 모든 상태 플래그 초기화 완료"));

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
		LoadingViewModel->SetLoadingText(FText::FromString(TEXT("준비 완료!")));
		return;
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
	UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] CancelPreload 호출됨 -> 로딩화면 강제종료 및 상태 초기화"));
	
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
	PendingHiddenCallbacks.Reset();
	LoadingViewModel = nullptr;
}

void UKCLoadingScreenSubsystem::HandleSessionJoinComplete(bool bWasSuccessful, const FString& ConnectString)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("[Preload] JoinSession 실패 감지 -> 로딩화면 취소"));
		CancelPreload();
	}
}

void UKCLoadingScreenSubsystem::HandleSessionCreateComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("[Preload] CreateSession 실패 감지 -> 로딩화면 취소"));
		CancelPreload();
	}
}

void UKCLoadingScreenSubsystem::RunAfterLoadingScreenHidden(UObject* WorldContextObject, FSimpleDelegate Callback)
{
	if (!ActiveLoadingWidget)
	{
		UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] RunAfterLoadingScreenHidden - 이미 로딩화면 없음, 즉시 실행"));
		Callback.ExecuteIfBound();
		return;
	}

	UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] RunAfterLoadingScreenHidden - 컨트롤러 도착, 대기열 등록 및 락(Lock) 해제"));
	PendingHiddenCallbacks.Add(Callback);
	bControllerReady = true;

	// 컨트롤러가 유효한 플레이 월드에서 호출한 경우, 레벨 로드 완료 보장 (재접속/난입 상황 자동 치유)
	if (WorldContextObject)
	{
		if (const UWorld* World = WorldContextObject->GetWorld())
		{
			const EKCLevelType CurrentLevelType = UKCLevelTypeLibrary::GetLevelTypeFromWorld(World);
			if (UKCLevelTypeLibrary::IsPlayableLevel(CurrentLevelType))
			{
				if (!bLevelReady || WaitingForLevel != CurrentLevelType)
				{
					UE_LOG(LogKCGameSystem, Warning, TEXT("[LoadingScreen] RunAfterLoadingScreenHidden - 컨트롤러 월드 감지: %d (기존 대기: %d) -> 레벨 준비 완료 처리"),
						static_cast<int32>(CurrentLevelType), static_cast<int32>(WaitingForLevel));
					WaitingForLevel = CurrentLevelType;
					bLevelReady = true;
				}
				bAssetsReady = true;
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