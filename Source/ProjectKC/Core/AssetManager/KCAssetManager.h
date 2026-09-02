#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "KCAssetManager.generated.h"

UCLASS()
class PROJECTKC_API UKCAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	// UKCAssetManager::Get()으로 접근 - Cast<UKCAssetManager>(GEngine->AssetManager)의 래퍼.
	// AssetManager는 엔진(GEngine) 소속 싱글톤이라 GameInstance/레벨과 무관하게
	// 프로세스 하나에 항상 하나만 존재한다 (그래서 "서버 프로세스의 AssetManager"와
	// "클라이언트 프로세스의 AssetManager"는 완전히 별개의 메모리 - 한쪽에서
	// 뭘 로드해도 다른 쪽엔 절대 영향이 없다는 뜻이기도 하다).
	static UKCAssetManager& Get();

	/**
	 * 특정 PrimaryAssetType에 속한 모든 에셋을 비동기로 프리로드합니다.
	 *
	 * [내부적으로 두 단계로 나뉜다]
	 *  1) GetPrimaryAssetIdList(AssetType, ...) - 동기, 즉시 반환. 실제 로드가 아니라
	 *     "이 타입에 등록된 에셋 ID 목록"을 엔진이 미리 스캔해둔 카탈로그에서 꺼내오는
	 *     것뿐이다. 이게 되려면 대상 클래스(예: KCItemDefinition)가 UPrimaryDataAsset을
	 *     상속하고 GetPrimaryAssetId()를 오버라이드해서 "나는 Item 타입이다"라고
	 *     선언해뒀어야 한다.
	 *  2) LoadPrimaryAssets(AssetIds, ...) - 진짜 비동기 로드. 반환되는
	 *     TSharedPtr<FStreamableHandle>이 "지금 돌고 있는 로딩 작업" 그 자체를
	 *     가리키는 객체다. 이 핸들에 BindUpdateDelegate로 진행률 콜백을 걸 수 있다.
	 *
	 * @param AssetType   로드할 PrimaryAssetType (예: "Item")
	 * @param OnProgress  로딩 도중 스트리밍 매니저가 주기적으로 호출. 인자는 0.0~1.0 누적 진행률.
	 *                    비워도(nullptr) 무방합니다.
	 * @param OnComplete  로딩이 전부 끝나면 1회 호출. AssetIds가 0개인 예외 상황에도 즉시 호출됨.
	 * @return            로딩 작업을 나타내는 핸들. 호출부에서 굳이 들고 있지 않아도 로드는 진행되지만,
	 *                    취소하거나 별도로 GetProgress()를 폴링하고 싶다면 이 반환값을 보관하면 됩니다.
	 *                    nullptr이 반환될 수도 있음 (요청한 에셋이 이미 전부 메모리에 있는 경우 등 -
	 *                    이건 버그가 아니라 엔진 공식 문서에 명시된 정상 동작).
	 */
	TSharedPtr<FStreamableHandle> PreloadAssetsByType(
		FPrimaryAssetType AssetType,
		TFunction<void(float)> OnProgress,
		TFunction<void()> OnComplete);

	/**
	 * 여러 PrimaryAssetType을 한 번에 프리로드합니다.
	 *
	 * [왜 타입마다 따로 안 부르고 하나로 합쳤는가]
	 * PreloadAssetsByType을 타입 개수만큼 여러 번 부르면, LoadPrimaryAssets도 여러 번
	 * 호출돼서 FStreamableHandle이 타입 개수만큼 따로 생긴다. 그러면 "전체 진행률"을
	 * 알고 싶을 때 여러 핸들의 진행률을 우리가 직접 평균 내는 등의 번거로운 작업이
	 * 필요해진다. 대신 여기서는 각 타입의 AssetId를 GetPrimaryAssetIdList로 미리
	 * 다 뽑아서 하나의 배열로 합친 다음, LoadPrimaryAssets를 딱 한 번만 호출한다.
	 * 그러면 핸들도 하나, 진행률도 자동으로 "요청한 전체 에셋 기준"으로 통합되어 나온다.
	 *
	 * @param AssetTypes  로드할 PrimaryAssetType 목록 (예: {"Item", "Trap"}).
	 *                    단, 목록에 넣는 타입은 전부 이미 PrimaryAsset 시스템에
	 *                    편입돼 있어야 한다 (지금 이 프로젝트에서는 Item만 해당됨 -
	 *                    Pot/Trap은 아직 그냥 AActor라 여기 넣어도 아무 것도 안 잡힘).
	 */
	TSharedPtr<FStreamableHandle> PreloadAssetsByTypes(
		const TArray<FPrimaryAssetType>& AssetTypes,
		TFunction<void(float)> OnProgress,
		TFunction<void()> OnComplete);

protected:
	virtual void StartInitialLoading() override;
};