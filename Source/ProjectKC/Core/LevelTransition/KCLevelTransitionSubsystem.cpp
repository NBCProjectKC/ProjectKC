#include "KCLevelTransitionSubsystem.h"

#include "GameSystem/KCLevelTypeLibrary.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Messages/KCGameplayTags.h"
#include "Messages/Struct/KCLevelChangedStruct.h"

#include "ProjectKC/ProjectKC.h"

void UKCLevelTransitionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostLoadMapWithWorldHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UKCLevelTransitionSubsystem::OnLevelLoaded);
}

void UKCLevelTransitionSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapWithWorldHandle);

	Super::Deinitialize();
}

void UKCLevelTransitionSubsystem::OnLevelLoaded(UWorld* LoadedWorld)
{
	if (!LoadedWorld)
	{
		return;
	}
	
	const EKCLevelType NewLevelType = UKCLevelTypeLibrary::GetLevelTypeFromWorld(LoadedWorld);

	FKCLevelChangedStruct Message;
	Message.NewLevelType = NewLevelType;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(KCGameplayTags::Message_Level_Changed, Message);

	UE_LOG(LogKCGameSystem, Warning, TEXT("[LevelTransition] 레벨 전환 감지: %s (Type=%d) -> Message_Level_Changed 브로드캐스트"),
		*LoadedWorld->GetMapName(), static_cast<int32>(NewLevelType));
}