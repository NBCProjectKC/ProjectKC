#pragma once

#include "CoreMinimal.h"
#include "Core/Audio/KCSoundSettings.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectKC/GameSystem/Enum/KCLevelType.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "KCBGMSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
struct FKCLevelChangedStruct;

UCLASS()
class PROJECTKC_API UKCBGMSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "KC|Audio")
	void PlayBGMForLevel(EKCLevelType LevelType);

	UFUNCTION(BlueprintCallable, Category = "KC|Audio")
	void StopCurrentBGM();

	UFUNCTION(BlueprintCallable, Category = "KC|Audio")
	void ApplySoundSettings();

	UFUNCTION(BlueprintCallable, Category = "KC|Audio")
	void SetSoundCategoryVolume(EKCSoundCategory Category, float Volume);

	UFUNCTION(BlueprintPure, Category = "KC|Audio")
	float GetSoundCategoryVolume(EKCSoundCategory Category) const;

private:
	void HandleLevelChanged(FGameplayTag Channel, const FKCLevelChangedStruct& Message);
	bool ShouldPlayAudio() const;
	bool IsSameBGM(const USoundBase* NewBGM) const;
	const FKCSoundClassSetting* GetSoundClassSetting(EKCSoundCategory Category) const;
	void ApplySoundClassVolume(EKCSoundCategory Category, float Volume);

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveBGMComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CurrentBGM;

	EKCLevelType CurrentLevelType = EKCLevelType::None;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Audio")
	float FadeInDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Audio")
	float FadeOutDuration = 0.5f;

	UPROPERTY(Transient)
	TMap<EKCSoundCategory, float> RuntimeVolumes;

	FGameplayMessageListenerHandle LevelChangedListenerHandle;
};
