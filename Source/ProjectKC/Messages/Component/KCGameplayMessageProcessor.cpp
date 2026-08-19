/**
 * @file KCGameplayMessageProcessor.cpp
 * @brief UKCGameplayMessageProcessor 구현부
 */

#include "ProjectKC/Messages/Component/KCGameplayMessageProcessor.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(KCGameplayMessageProcessor)

UKCGameplayMessageProcessor::UKCGameplayMessageProcessor()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UKCGameplayMessageProcessor::BeginPlay()
{
	Super::BeginPlay();
	StartListening();
}

void UKCGameplayMessageProcessor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	StopListening();

	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		for (FGameplayMessageListenerHandle& Handle : ListenerHandles)
		{
			MessageSubsystem.UnregisterListener(Handle);
		}
	}
	ListenerHandles.Empty();
}

void UKCGameplayMessageProcessor::StartListening()
{
}

void UKCGameplayMessageProcessor::StopListening()
{
}

void UKCGameplayMessageProcessor::AddListenerHandle(FGameplayMessageListenerHandle&& Handle)
{
	ListenerHandles.Add(MoveTemp(Handle));
}

double UKCGameplayMessageProcessor::GetServerTime() const
{
	if (UWorld* World = GetWorld())
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			return GameState->GetServerWorldTimeSeconds();
		}
	}
	return 0.0;
}
