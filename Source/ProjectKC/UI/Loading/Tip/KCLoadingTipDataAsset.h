#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KCLoadingTipDataAsset.generated.h"

/**
 * 팁 struct
 * 지금은 Text만 있고, 이미지/영상 등 필요시 추가
 */
USTRUCT(BlueprintType)
struct FKCLoadingTip
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "KC|UI")
	FText Text;

	// TODO : 이미지 추가 시
	// UPROPERTY(EditAnywhere, Category = "KC|UI")
	// TObjectPtr<UTexture2D> Image;
};

/**
 * 로딩화면에 표시할 팁 목록 Data Asset
 * UKCColorStyle과 같은 패턴으로 만들어봤습니다.
 */
UCLASS(BlueprintType)
class PROJECTKC_API UKCLoadingTipDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "KC|UI")
	TArray<FKCLoadingTip> Tips;
};