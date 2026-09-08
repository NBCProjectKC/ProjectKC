#include "ProjectKC/Core/Audio/KCBGMSubsystem.h"

#include "Components/AudioComponent.h"
#include "GameSystem/KCLevelInfoRow.h"
#include "GameSystem/KCLevelTypeLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Messages/KCGameplayTags.h"
#include "Messages/Struct/KCLevelChangedStruct.h"

void UKCBGMSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LevelChangedListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener<FKCLevelChangedStruct>(
		KCGameplayTags::Message_Level_Changed, this, &ThisClass::HandleLevelChanged);

	if (const UWorld* World = GetWorld())
	{
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

	ActiveBGMComponent = UGameplayStatics::SpawnSound2D(this, NewBGM, 1.0f, 1.0f, 0.0f, nullptr, true, false);
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
