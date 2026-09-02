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

void UKCLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LevelChangedListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener<FKCLevelChangedStruct>(
		KCGameplayTags::Message_Level_Changed, this, &UKCLoadingScreenSubsystem::OnLevelChangedMessage);

	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] KCLoadingScreenSubsystem::Initialize 완료, 리스너 등록됨 (IsValid=%s)"),
		LevelChangedListenerHandle.IsValid() ? TEXT("true") : TEXT("false"));
}

void UKCLoadingScreenSubsystem::Deinitialize()
{
	UGameplayMessageSubsystem::Get(this).UnregisterListener(LevelChangedListenerHandle);
	Super::Deinitialize();
}

void UKCLoadingScreenSubsystem::BeginPreload(EKCLevelType TargetLevel, const TArray<FPrimaryAssetType>& AssetTypes,
	TSubclassOf<UKCLoadingScreen> ScreenClass, const UKCLoadingTipDataAsset* TipsAsset)
{
	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] BeginPreload 진입. TargetLevel=%d, AssetTypes.Num()=%d, ScreenClass=%s"),
		static_cast<uint8>(TargetLevel), AssetTypes.Num(), *GetNameSafe(ScreenClass));

	if (WaitingForLevel != EKCLevelType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("KCLoadingScreenSubsystem::BeginPreload - 이미 다른 전환(%d)을 기다리는 중에 다시 호출됨. 무시합니다."),
			static_cast<uint8>(WaitingForLevel));
		return;
	}

	WaitingForLevel = TargetLevel;
	bAssetsReady = false;
	bLevelReady = false;

	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] 방어 코드 통과, 상태 초기화 완료"));

	const ULocalPlayer* LocalPlayer = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;
	UKCLocalPlayerUISubsystem* UISubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UKCLocalPlayerUISubsystem>() : nullptr;

	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] LocalPlayer=%s, UISubsystem=%s"),
		LocalPlayer ? TEXT("Valid") : TEXT("NULL"), UISubsystem ? TEXT("Valid") : TEXT("NULL"));

	// ============ 1. 뷰모델 준비 ============
	if (!LoadingViewModel)
	{
		LoadingViewModel = NewObject<UKCLoadingViewModel>(this);
		UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] LoadingViewModel 새로 생성함"));
	}
	LoadingViewModel->SetProgress(0.0f);
	LoadingViewModel->PickRandomTip(TipsAsset);
		
	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] 뷰모델 준비 완료, 위젯 표시 단계 진입 직전"));

	// ============ 2. 위젯 표시 + 뷰모델 연결 ============
	if (UISubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] SetHUDWidget 호출 직전"));
		
		if (UKCUserWidget* CreatedWidget = UISubsystem->SetHUDWidget(ScreenClass,/*bPersistAcrossLevelTravel=*/true))
		{
			UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] SetHUDWidget 성공, CreatedWidget=%s"), *GetNameSafe(CreatedWidget));
			
			if (UMVVMView* View = CreatedWidget->GetExtension<UMVVMView>())
			{
				UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] MVVMView 획득 성공, SetViewModel 호출 직전"));
				const bool bSetOk = View->SetViewModel(TEXT("LoadingViewModel"), LoadingViewModel);
				UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] SetViewModel 결과=%s"), bSetOk ? TEXT("true") : TEXT("false"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] GetExtension<UMVVMView>() 실패 - WBP_Loading에 뷰모델이 아직 안 등록된 것으로 보임"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] SetHUDWidget이 nullptr 반환함"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] UISubsystem이 NULL이라 위젯 표시 단계 스킵됨"));
	}

	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] 위젯 단계 끝, 에셋 프리로드 시작 직전"));

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
					const float CappedProgress = FMath::Min(NewProgress * 0.99f, 0.99f);
					StrongThis->LoadingViewModel->SetProgress(CappedProgress);
				}
			}
		},
		[WeakThis]()
		{
			UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] AssetManager OnComplete 콜백 진입"));
			if (UKCLoadingScreenSubsystem* StrongThis = WeakThis.Get())
			{
				StrongThis->bAssetsReady = true;
				StrongThis->TryHide();
			}
		});

	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] BeginPreload 함수 끝까지 도달함"));
}

void UKCLoadingScreenSubsystem::OnLevelChangedMessage(FGameplayTag Channel, const FKCLevelChangedStruct& Message)
{
	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] Message_Level_Changed 수신. NewLevelType=%d, WaitingForLevel=%d"),
		static_cast<uint8>(Message.NewLevelType), static_cast<uint8>(WaitingForLevel));

	if (WaitingForLevel == EKCLevelType::None || Message.NewLevelType != WaitingForLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] 우리가 기다리는 레벨이 아니라서 무시함"));
		return;
	}

	bLevelReady = true;
	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] bLevelReady = true 설정됨, TryHide 호출"));
	TryHide();
}

void UKCLoadingScreenSubsystem::TryHide()
{
	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] TryHide 진입. bAssetsReady=%s, bLevelReady=%s"),
		bAssetsReady ? TEXT("true") : TEXT("false"), bLevelReady ? TEXT("true") : TEXT("false"));

	if (!bAssetsReady || !bLevelReady)
	{
		return;
	}

	// 이때 progressbar 100% 채움
	if (LoadingViewModel)
	{
		LoadingViewModel->SetProgress(1.0f);
	}

	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] 두 조건 다 충족, 위젯 내리는 중"));

	const ULocalPlayer* LocalPlayer = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;
	if (UKCLocalPlayerUISubsystem* UISubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UKCLocalPlayerUISubsystem>() : nullptr)
	{
		UISubsystem->ClearHUDWidget();
	}

	WaitingForLevel = EKCLevelType::None;

	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] TryHide 완료"));
}