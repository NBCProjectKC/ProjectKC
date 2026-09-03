#pragma once
 
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectKC/GameSystem/Enum/KCLevelType.h"
#include "KCLoadingScreenSubsystem.generated.h"
 
class UKCLoadingScreen;
class UKCLoadingViewModel;
struct FKCLevelChangedStruct;
class UKCLoadingTipDataAsset;
class UKCUserWidget;

UCLASS()
class PROJECTKC_API UKCLoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
 
public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
 
	/**
	 * @param TargetLevel   타겟 레벨 타입 (예: EKCLevelType::GasRange).
	 *                      나중에 "GasRange → 결과화면" 같은 다른 전환에도 이 함수를
	 *                      재사용할 수 있도록 함
	 * @param AssetTypes    pre-load할 PrimaryAssetType 목록 (예: {"Item"})
	 * @param ScreenClass   표시할 로딩화면 위젯 클래스
	 */
	UFUNCTION(BlueprintCallable, Category = "KC|Loading")
	void BeginPreload(EKCLevelType TargetLevel, const TArray<FPrimaryAssetType>& AssetTypes,
	TSubclassOf<UKCLoadingScreen> ScreenClass, const UKCLoadingTipDataAsset* TipsAsset);
 
	UPROPERTY(Transient)
	TObjectPtr<UKCUserWidget> ActiveLoadingWidget;
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
	bool HideWidgetDelayed(float DeltaTime);
	
	// GMS 핸들
	FGameplayMessageListenerHandle LevelChangedListenerHandle;
};
 
