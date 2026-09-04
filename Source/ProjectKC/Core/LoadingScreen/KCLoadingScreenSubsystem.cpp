#include "KCLoadingScreenSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "ProjectKC/Core/AssetManager/KCAssetManager.h"
#include "ProjectKC/Messages/KCGameplayTags.h"
#include "ProjectKC/Messages/Struct/KCLevelChangedStruct.h"
#include "ProjectKC/UI/Common/Core/KCLocalPlayerUISubsystem.h"
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
			if (UKCLoadingScreenSubsystem* StrongThis = WeakThis.Get())
			{
				if (StrongThis->LoadingViewModel)
				{
					const float CappedProgress = FMath::Min(NewProgress * 0.97f, 0.97f);
					StrongThis->LoadingViewModel->SetProgress(CappedProgress);
				}
			}
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
	}

	// 100%가 화면에 실제로 그려질 시간을 준 다음에 위젯을 지운다.
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

	return true;
}