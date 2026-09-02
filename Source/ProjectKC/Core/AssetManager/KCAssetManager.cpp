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
		UE_LOG(LogTemp, Warning, TEXT("PreloadAssetsByType: '%s' 타입의 에셋이 없습니다."), *AssetType.ToString());
		if (OnProgress)
		{
			OnProgress(1.0f);
		}
		OnComplete();
		return nullptr;
	}

	// 2. 실제 비동기 로드 시작. 반환된 Handle이 이 로딩 작업 자체를 가리킵니다.
	TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssets(
		AssetIds,
		TArray<FName>(),
		FStreamableDelegate::CreateLambda([OnComplete]()
		{
			OnComplete();
		})
	);

	// 3. Handle이 유효할 때만 진행률 콜백을 걸어줍니다.
	//    BindUpdateDelegate는 스트리밍 매니저가 로딩 도중 주기적으로 호출해주는 델리게이트이며,
	//    Handle 하나당 하나만 바인딩됩니다 (여러 번 걸면 마지막 것으로 덮어써짐 - 이 함수를
	//    같은 AssetType에 대해 동시에 여러 번 호출하지 않도록 주의).
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
		// 요청한 에셋이 이미 전부 메모리에 있는 등의 이유로 Handle이 nullptr로 오는 경우 방어.
		// 이 경우 OnComplete는 위 델리게이트를 통해 이미 동기적으로 호출됐을 수 있으므로
		// Progress만 100%로 맞춰줍니다.
		OnProgress(1.0f);
	}

	return Handle;
}

TSharedPtr<FStreamableHandle> UKCAssetManager::PreloadAssetsByTypes(
	const TArray<FPrimaryAssetType>& AssetTypes,
	TFunction<void(float)> OnProgress,
	TFunction<void()> OnComplete)
{
	// [임시 디버그 로그] "[KC_LOADING_DEBUG]" 붙은 줄들은 호스트 GasRange 진입 멈춤
	// 현상 원인 추적용. 원인 찾으면 지우면 됨.
	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] PreloadAssetsByTypes 진입. AssetTypes.Num()=%d"), AssetTypes.Num());
	
	
	// 여러 타입의 AssetId를 하나의 배열로 합친다 - LoadPrimaryAssets를 타입별로 따로 부르지 않고
	// 딱 한 번만 호출해서, 진행률/완료 콜백이 하나의 Handle 기준으로 통합되게 한다.
	// (타입마다 따로 호출했다면 핸들이 여러 개 생겨서, 전체 진행률을 우리가 직접
	//  평균 내야 하는 번거로움이 생겼을 것 - 그래서 로드 "요청" 단계에서부터 합침)
	TArray<FPrimaryAssetId> AllAssetIds;
	for (const FPrimaryAssetType& AssetType : AssetTypes)
	{
		// GetPrimaryAssetIdList는 동기 함수라 여기서 루프를 돌아도 실제 로딩이
		// 여러 번 발생하는 게 아니다 - 그냥 카탈로그에서 ID만 여러 번 조회하는 것뿐.
		TArray<FPrimaryAssetId> AssetIds;
		GetPrimaryAssetIdList(AssetType, AssetIds);
		UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] 타입 '%s'에서 AssetId %d개 발견"), *AssetType.ToString(), AssetIds.Num());
		AllAssetIds.Append(AssetIds);
	}

	if (AllAssetIds.Num() == 0)
	{
		// AssetTypes가 비어있거나(호출부에서 프리로드할 게 없는 경우), 넘긴 타입들이
		// 전부 PrimaryAsset 시스템에 편입 안 돼 있는 경우(예: 아직 Pot/Trap을 넣은 경우)
		// 여기로 빠진다. 즉시 완료 처리해서 호출부가 영원히 기다리는 사고를 막는다.
		UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG]PreloadAssetsByTypes: 요청한 타입들에 해당하는 에셋이 없습니다."));
		if (OnProgress)
		{
			OnProgress(1.0f);
		}
		OnComplete();
		UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] AssetId 0개 경로로 즉시 OnComplete 호출 완료"));
		return nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] LoadPrimaryAssets 호출 직전 (여기서 멈추면 스트리밍 매니저/에셋 매니저 초기화 쪽 의심)"));
	
	TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssets(
		AllAssetIds,
		TArray<FName>(),
		FStreamableDelegate::CreateLambda([OnComplete]()
		{
			OnComplete();
		})
	);
	
	UE_LOG(LogTemp, Warning, TEXT("[KC_LOADING_DEBUG] LoadPrimaryAssets 리턴함. Handle.IsValid()=%s"), Handle.IsValid() ? TEXT("true") : TEXT("false"));

	// 아래 로직은 PreloadAssetsByType(단일 타입 버전)과 완전히 동일한 이유로 존재한다 -
	// Handle이 유효할 때만 BindUpdateDelegate로 진행률을 걸고, nullptr이면(이미 다
	// 로드돼 있던 경우 등) Progress를 그냥 100%로 맞춰준다.
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