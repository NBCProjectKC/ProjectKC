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

TSharedPtr<FStreamableHandle> UKCAssetManager::PreloadAssetsByType(
	FPrimaryAssetType AssetType,
	TFunction<void(float)> OnProgress,
	TFunction<void()> OnComplete)
{
	// 1. 카탈로그 조회 (동기, 즉시 반환 - 실제 로드 아님)
	TArray<FPrimaryAssetId> AssetIds;
	GetPrimaryAssetIdList(AssetType, AssetIds);

	if (AssetIds.Num() == 0)
	{
		if (OnProgress)
		{
			OnProgress(1.0f);
		}
		OnComplete();
		return nullptr;
	}

	// 2. 비동기 로드 시작
	TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssets(
		AssetIds,
		TArray<FName>(),
		FStreamableDelegate::CreateLambda([OnComplete]()
		{
			OnComplete();
		})
	);

	// 3. progress callback
	if (Handle.IsValid())
	{
		if (OnProgress)
		{
			Handle->BindUpdateDelegate(FStreamableUpdateDelegate::CreateLambda(
				[OnProgress](TSharedRef<FStreamableHandle> InHandle)
				{
					OnProgress(InHandle->GetProgress());
				}));
		}
	}
	else if (OnProgress)
	{
		// 요청한 에셋이 이미 전부 메모리에 있는 등의 이유로 Handle이 nullptr로 오는 경우 방어
		OnProgress(1.0f);
	}

	return Handle;
}

TSharedPtr<FStreamableHandle> UKCAssetManager::PreloadAssetsByTypes(
	const TArray<FPrimaryAssetType>& AssetTypes,
	TFunction<void(float)> OnProgress,
	TFunction<void()> OnComplete)
{
	TArray<FPrimaryAssetId> AllAssetIds;
	for (const FPrimaryAssetType& AssetType : AssetTypes)
	{
		TArray<FPrimaryAssetId> AssetIds;
		GetPrimaryAssetIdList(AssetType, AssetIds);
	}

	if (AllAssetIds.Num() == 0)
	{
		if (OnProgress)
		{
			OnProgress(1.0f);
		}
		OnComplete();
		return nullptr;
	}

	TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssets(
		AllAssetIds,
		TArray<FName>(),
		FStreamableDelegate::CreateLambda([OnComplete]()
		{
			OnComplete();
		})
	);
	
	if (Handle.IsValid())
	{
		if (OnProgress)
		{
			Handle->BindUpdateDelegate(FStreamableUpdateDelegate::CreateLambda(
				[OnProgress](TSharedRef<FStreamableHandle> InHandle)
				{
					OnProgress(InHandle->GetProgress());
				}));
		}
	}
	else if (OnProgress)
	{
		OnProgress(1.0f);
	}

	return Handle;
}