#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "KCAssetManager.generated.h"

UCLASS()
class PROJECTKC_API UKCAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static UKCAssetManager& Get();

	/**
	 * @param AssetType   로드할 PrimaryAssetType (예: "Item")
	 * @param OnProgress  로딩 도중 스트리밍 매니저가 주기적으로 호출. 인자는 0.0~1.0 누적 진행률
	 * @param OnComplete  로딩이 전부 끝나면 1회 호출. AssetIds가 0개인 예외 상황에도 즉시 호출됨
	 * @return            로딩 작업을 나타내는 핸들
	 */
	TSharedPtr<FStreamableHandle> PreloadAssetsByType(
		FPrimaryAssetType AssetType,
		TFunction<void(float)> OnProgress,
		TFunction<void()> OnComplete);

	/**
	 * @param AssetTypes  로드할 PrimaryAssetType 목록 (예: {"Item", "Trap"}).
	 */
	TSharedPtr<FStreamableHandle> PreloadAssetsByTypes(
		const TArray<FPrimaryAssetType>& AssetTypes,
		TFunction<void(float)> OnProgress,
		TFunction<void()> OnComplete);

protected:
	virtual void StartInitialLoading() override;
};