#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KCCustomizationAssetSet.generated.h"

class UMaterialInterface;
class UStaticMesh;

/** 캐릭터 커스터마이징 외형에 필요한 프로젝트 에셋을 한 곳에서 소유합니다. */
UCLASS(BlueprintType)
class PROJECTKC_API UKCCustomizationAssetSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Customization|Assets")
	TObjectPtr<UStaticMesh> EyeMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Customization|Assets")
	TObjectPtr<UStaticMesh> ApronMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Customization|Assets")
	TObjectPtr<UStaticMesh> ChefHatMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Customization|Assets")
	TObjectPtr<UMaterialInterface> PaintMaterial;
};
