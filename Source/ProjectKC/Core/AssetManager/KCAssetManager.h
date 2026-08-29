#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "KCAssetManager.generated.h"

UCLASS()
class PROJECTKC_API UKCAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	// UKCAssetManager::Get()으로 접근
	static UKCAssetManager& Get();

	// 비동기 로드
	void PreloadAssetsByType(FPrimaryAssetType AssetType, TFunction<void()> OnComplete);

protected:
	virtual void StartInitialLoading() override;
};