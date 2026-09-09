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
	UE_LOG(LogTemp, Warning, TEXT("[KC_DEBUG] DT 조회 성공. MapName=%s, AssetTypesToPreload 개수=%d"),
		*Row->MapName.ToString(), Row->AssetTypesToPreload.Num());
	WaitingForLevel = TargetLevel;
	bAssetsReady = false;
	bLevelReady = false;

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
				StrongThis->bAssetsReady = true;
				StrongThis->TryHide();
			}
		});
}

void UKCLoadingScreenSubsystem::OnLevelChangedMessage(FGameplayTag Channel, const FKCLevelChangedStruct& Message)
{
	if (WaitingForLevel == EKCLevelType::None || Message.NewLevelType != WaitingForLevel)
	{
		return;
	}

	bLevelReady = true;
	TryHide();
}

void UKCLoadingScreenSubsystem::TryHide()
{
	if (!bAssetsReady || !bLevelReady)
	{
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
	HideDelayTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UKCLoadingScreenSubsystem::HideWidgetDelayed), 0.3f);

	WaitingForLevel = EKCLevelType::None;
}

bool UKCLoadingScreenSubsystem::HideWidgetDelayed(float DeltaTime)
{
	if (ActiveLoadingWidget)
	{
		ActiveLoadingWidget->RemoveFromParent();
		ActiveLoadingWidget = nullptr;
	}
	// 로딩화면 Hide 후 알림 -> InGameHUD 세팅 호출
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(KCGameplayTags::Message_LoadingScreen_Hidden, FKCEmptyMessageStruct());
	
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
	UE_LOG(LogTemp, Warning, TEXT("UKCLoadingScreenSubsystem::CancelPreload() : 로딩화면 강제종료 요청됨"));
	
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
	LoadingViewModel = nullptr;
}

void UKCLoadingScreenSubsystem::HandleSessionJoinComplete(bool bWasSuccessful, const FString& ConnectString)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("[KCLoadingScreenSubsystem] JoinSession 실패 감지 -> 로딩화면 취소"));
		CancelPreload();
	}
}

void UKCLoadingScreenSubsystem::HandleSessionCreateComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("[KCLoadingScreenSubsystem] CreateSession 실패 감지 -> 로딩화면 취소"));
		CancelPreload();
	}
}

void UKCLoadingScreenSubsystem::RunAfterLoadingScreenHidden(UObject* WorldContextObject, FSimpleDelegate Callback)
{
	if (!ActiveLoadingWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[KC_DEBUG13] RunAfterLoadingScreenHidden - 이미 로딩화면 없음, 즉시 실행"));
		Callback.ExecuteIfBound();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[KC_DEBUG13] RunAfterLoadingScreenHidden - 로딩화면 대기, 리스너 등록"));

	UGameplayMessageSubsystem::Get(WorldContextObject).RegisterListener<FKCEmptyMessageStruct>(
		KCGameplayTags::Message_LoadingScreen_Hidden,
		[Callback](FGameplayTag, const FKCEmptyMessageStruct&)
		{
			UE_LOG(LogTemp, Warning, TEXT("[KC_DEBUG13] RunAfterLoadingScreenHidden - 로딩화면 종료 감지, 실행"));
			Callback.ExecuteIfBound();
		}
	);
}