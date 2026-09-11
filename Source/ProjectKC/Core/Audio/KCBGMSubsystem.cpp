#include "ProjectKC/Core/Audio/KCBGMSubsystem.h"

#include "Components/AudioComponent.h"
#include "GameSystem/KCLevelInfoRow.h"
#include "GameSystem/KCLevelTypeLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Messages/KCGameplayTags.h"
#include "Messages/Struct/KCLevelChangedStruct.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

void UKCBGMSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LevelChangedListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener<FKCLevelChangedStruct>(
		KCGameplayTags::Message_Level_Changed, this, &ThisClass::HandleLevelChanged);

	if (const UWorld* World = GetWorld())
	{
		ApplySoundSettings();
		PlayBGMForLevel(UKCLevelTypeLibrary::GetLevelTypeFromWorld(World));
	}
}

void UKCBGMSubsystem::Deinitialize()
{
	UGameplayMessageSubsystem::Get(this).UnregisterListener(LevelChangedListenerHandle);
	StopCurrentBGM();

	Super::Deinitialize();
}

void UKCBGMSubsystem::PlayBGMForLevel(EKCLevelType LevelType)
{
	if (!ShouldPlayAudio())
	{
		return;
	}

	const FKCLevelInfoRow* LevelInfo = UKCLevelTypeLibrary::GetLevelInfoRow(LevelType);
	USoundBase* NewBGM = LevelInfo ? LevelInfo->BGM : nullptr;
	if (!NewBGM)
	{
		StopCurrentBGM();
		CurrentLevelType = LevelType;
		return;
	}

	if (CurrentLevelType == LevelType && IsSameBGM(NewBGM))
	{
		return;
	}

	StopCurrentBGM();

	ActiveBGMComponent = UGameplayStatics::SpawnSound2D(
		this,
		NewBGM,
		GetSoundCategoryVolume(EKCSoundCategory::BGM),
		1.0f,
		0.0f,
		nullptr,
		true,
		false);
	if (!ActiveBGMComponent)
	{
		CurrentBGM = nullptr;
		CurrentLevelType = EKCLevelType::None;
		return;
	}

	CurrentBGM = NewBGM;
	CurrentLevelType = LevelType;

	if (FadeInDuration > 0.0f)
	{
		ActiveBGMComponent->FadeIn(FadeInDuration);
	}
}

void UKCBGMSubsystem::StopCurrentBGM()
{
	if (ActiveBGMComponent)
	{
		if (FadeOutDuration > 0.0f && ActiveBGMComponent->IsPlaying())
		{
			ActiveBGMComponent->FadeOut(FadeOutDuration, 0.0f);
		}
		else
		{
			ActiveBGMComponent->Stop();
		}

		ActiveBGMComponent = nullptr;
	}

	CurrentBGM = nullptr;
	CurrentLevelType = EKCLevelType::None;
}

void UKCBGMSubsystem::ApplySoundSettings()
{
	if (!ShouldPlayAudio())
	{
		return;
	}

	ApplySoundClassVolume(EKCSoundCategory::Master, GetSoundCategoryVolume(EKCSoundCategory::Master));
	ApplySoundClassVolume(EKCSoundCategory::BGM, GetSoundCategoryVolume(EKCSoundCategory::BGM));
	ApplySoundClassVolume(EKCSoundCategory::SFX, GetSoundCategoryVolume(EKCSoundCategory::SFX));
	ApplySoundClassVolume(EKCSoundCategory::UI, GetSoundCategoryVolume(EKCSoundCategory::UI));
}

void UKCBGMSubsystem::SetSoundCategoryVolume(EKCSoundCategory Category, float Volume)
{
	const float ClampedVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	RuntimeVolumes.FindOrAdd(Category) = ClampedVolume;

	ApplySoundClassVolume(Category, ClampedVolume);
}

float UKCBGMSubsystem::GetSoundCategoryVolume(EKCSoundCategory Category) const
{
	if (const float* RuntimeVolume = RuntimeVolumes.Find(Category))
	{
		return *RuntimeVolume;
	}

	const FKCSoundClassSetting* Setting = GetSoundClassSetting(Category);
	return Setting ? Setting->Volume : 1.0f;
}

void UKCBGMSubsystem::HandleLevelChanged(FGameplayTag Channel, const FKCLevelChangedStruct& Message)
{
	PlayBGMForLevel(Message.NewLevelType);
}

bool UKCBGMSubsystem::ShouldPlayAudio() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_DedicatedServer;
}

bool UKCBGMSubsystem::IsSameBGM(const USoundBase* NewBGM) const
{
	return CurrentBGM == NewBGM;
}

const FKCSoundClassSetting* UKCBGMSubsystem::GetSoundClassSetting(EKCSoundCategory Category) const
{
	const UKCSoundSettings* SoundSettings = GetDefault<UKCSoundSettings>();
	if (!SoundSettings)
	{
		return nullptr;
	}

	switch (Category)
	{
	case EKCSoundCategory::Master:
		return &SoundSettings->Master;
	case EKCSoundCategory::BGM:
		return &SoundSettings->BGM;
	case EKCSoundCategory::SFX:
		return &SoundSettings->SFX;
	case EKCSoundCategory::UI:
		return &SoundSettings->UI;
	default:
		return nullptr;
	}
}

void UKCBGMSubsystem::ApplySoundClassVolume(EKCSoundCategory Category, float Volume)
{
	if (!ShouldPlayAudio())
	{
		return;
	}

	if (Category == EKCSoundCategory::BGM && ActiveBGMComponent)
	{
		ActiveBGMComponent->SetVolumeMultiplier(Volume);
	}

	const UKCSoundSettings* SoundSettings = GetDefault<UKCSoundSettings>();
	const FKCSoundClassSetting* ClassSetting = GetSoundClassSetting(Category);
	if (!SoundSettings || !ClassSetting)
	{
		return;
	}

	USoundMix* SoundMix = SoundSettings->DefaultSoundMix.LoadSynchronous();
	USoundClass* SoundClass = ClassSetting->SoundClass.LoadSynchronous();
	if (!SoundMix || !SoundClass)
	{
		return;
	}

	UGameplayStatics::PushSoundMixModifier(this, SoundMix);
	UGameplayStatics::SetSoundMixClassOverride(this, SoundMix, SoundClass, Volume, 1.0f, 0.0f, true);
}
