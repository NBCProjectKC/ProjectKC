#include "KCAssetManager.h"

UKCAssetManager& UKCAssetManager::Get()
{
	UKCAssetManager* This = Cast<UKCAssetManager>(GEngine->AssetManager);
	check(This);
	return *This;
}

void UKCAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	TArray<FPrimaryAssetId> ItemIds;
	GetPrimaryAssetIdList(FPrimaryAssetType("Item"), ItemIds);
	UE_LOG(LogTemp, Warning, TEXT("[AssetManager] Item 타입 %d개 발견"), ItemIds.Num());
}

void UKCAssetManager::PreloadAssetsByType(FPrimaryAssetType AssetType, TFunction<void()> OnComplete)
{
	TArray<FPrimaryAssetId> AssetIds;
	GetPrimaryAssetIdList(AssetType, AssetIds);

	if (AssetIds.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PreloadAssetsByType: '%s' 타입의 에셋이 없습니다."), *AssetType.ToString());
		OnComplete();
		return;
	}

	LoadPrimaryAssets(AssetIds, TArray<FName>(), FStreamableDelegate::CreateLambda([OnComplete]()
	{
		OnComplete();
	}));
}