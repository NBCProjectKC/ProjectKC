#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "KCSoundSettings.generated.h"

class USoundClass;
class USoundMix;

UENUM(BlueprintType)
enum class EKCSoundCategory : uint8
{
	Master UMETA(DisplayName = "Master"),
	BGM UMETA(DisplayName = "BGM"),
	SFX UMETA(DisplayName = "SFX"),
	UI UMETA(DisplayName = "UI")
};

USTRUCT(BlueprintType)
struct PROJECTKC_API FKCSoundClassSetting
{
	GENERATED_BODY()

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Audio")
	TSoftObjectPtr<USoundClass> SoundClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Volume = 1.0f;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ProjectKC Sound"))
class PROJECTKC_API UKCSoundSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Audio")
	TSoftObjectPtr<USoundMix> DefaultSoundMix;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Audio|Class")
	FKCSoundClassSetting Master;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Audio|Class")
	FKCSoundClassSetting BGM;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Audio|Class")
	FKCSoundClassSetting SFX;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Audio|Class")
	FKCSoundClassSetting UI;
};
