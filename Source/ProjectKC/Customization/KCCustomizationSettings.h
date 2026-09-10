#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "KCCustomizationSettings.generated.h"

class UKCCustomizationAssetSet;

/** 프로젝트 전체에서 공유하는 캐릭터 커스터마이징 에셋 설정입니다. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ProjectKC Customization"))
class PROJECTKC_API UKCCustomizationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Customization|Assets")
	TSoftObjectPtr<UKCCustomizationAssetSet> AssetSet;
};
