#include "KCAssetManager.h"

#include "Item/Definition/KCItemDefinition.h"

UKCAssetManager& UKCAssetManager::Get()
{
	UKCAssetManager* This = Cast<UKCAssetManager>(GEngine->AssetManager);
	check(This);
	return *This;
}

void UKCAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
}

void UKCAssetManager::PreloadAssetsByType(FPrimaryAssetType AssetType, TFunction<void()> OnComplete)
{
	// PrimaryAssetId 저장
	TArray<FPrimaryAssetId> AssetIds;
	GetPrimaryAssetIdList(AssetType, AssetIds);

	if (AssetIds.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PreloadAssetsByType: '%s' 타입의 에셋이 없습니다."), *AssetType.ToString());
		OnComplete();
		return;
	}
	// 비동기 일괄 스트리밍 로드
	LoadPrimaryAssets(AssetIds, TArray<FName>(), FStreamableDelegate::CreateLambda([OnComplete]()
	{
		OnComplete();
	}));
}