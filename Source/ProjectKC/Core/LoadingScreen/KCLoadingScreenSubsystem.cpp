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
#include "Messages/Struct/KCEmptyMessageStruct.h"

void UKCLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LevelChangedListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener<FKCLevelChangedStruct>(
		KCGameplayTags::Message_Level_Changed, this, &UKCLoadingScreenSubsystem::OnLevelChangedMessage);
}

void UKCLoadingScreenSubsystem::Deinitialize()
{
	UGameplayMessageSubsystem::Get(this).UnregisterListener(LevelChangedListenerHandle);
	Super::Deinitialize();
}

void UKCLoadingScreenSubsystem::BeginPreload(EKCLevelType TargetLevel, const TArray<FPrimaryAssetType>& AssetTypes,
	TSubclassOf<UKCLoadingScreen> ScreenClass, const UKCLoadingTipDataAsset* TipsAsset)
{
	if (WaitingForLevel != EKCLevelType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("KCLoadingScreenSubsystem::BeginPreload - 이미 다른 전환(%d)을 기다리는 중에 다시 호출됨. 무시합니다."),
			static_cast<uint8>(WaitingForLevel));
		return;
	}

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
	LoadingViewModel->PickRandomTip(TipsAsset);
	UpdateLoadingText();
	
	PreloadStartTimeSeconds = FPlatformTime::Seconds();
	ProgressAnimTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UKCLoadingScreenSubsystem::TickProgressAnimation), 0.05f);

	// ============ 2. 위젯 표시 + 뷰모델 연결 ============
	if (LocalPlayer)
	{
		if (APlayerController* PC = LocalPlayer->GetPlayerController(GetGameInstance()->GetWorld()))
		{
			ActiveLoadingWidget = CreateWidget<UKCUserWidget>(PC, ScreenClass);
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
			}
		}
	}

	// ============ 3. 에셋 프리로드 시작 ============
	TWeakObjectPtr<UKCLoadingScreenSubsystem> WeakThis(this);

	UKCAssetManager::Get().PreloadAssetsByTypes(
		AssetTypes,
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