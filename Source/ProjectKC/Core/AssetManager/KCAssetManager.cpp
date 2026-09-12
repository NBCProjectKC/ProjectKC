#include "KCAssetManager.h"

#include "Item/Definition/KCItemDefinition.h"
#include "ProjectKC/ProjectKC.h"

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

	UE_LOG(LogKCGameSystem, Warning, TEXT("[AssetManager] PreloadAssetsByType - Type=%s, 발견된 에셋: %d개"),
		*AssetType.ToString(), AssetIds.Num());

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
	else
	{
		// 요청한 에셋이 이미 전부 메모리에 있는 등의 이유로 Handle이 nullptr로 오는 경우,
		// 스트리밍할 게 없어 완료 델리게이트가 실행되지 않으므로 여기서 직접 완료 처리해야 한다.
		if (OnProgress)
		{
			OnProgress(1.0f);
		}
		OnComplete();
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
		UE_LOG(LogKCGameSystem, Warning, TEXT("[AssetManager] 프리로드 대상 감지: Type=%s, 개수=%d개 (누적 %d개)"),
			*AssetType.ToString(), AssetIds.Num(), AllAssetIds.Num() + AssetIds.Num());
		AllAssetIds.Append(AssetIds);
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
	else
	{
		// 요청한 에셋이 이미 전부 메모리에 있는 등의 이유로 Handle이 nullptr로 오는 경우,
		// 스트리밍할 게 없어 완료 델리게이트가 실행되지 않으므로 여기서 직접 완료 처리해야 한다.
		if (OnProgress)
		{
			OnProgress(1.0f);
		}
		OnComplete();
	}

	return Handle;
}