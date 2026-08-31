#include "KCLevelTransitionSubsystem.h"

#include "GameSystem/KCLevelTypeLibrary.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Messages/KCGameplayTags.h"
#include "Messages/Struct/KCLevelChangedStruct.h"

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
	
	FString MapName = LoadedWorld->GetMapName();
    MapName.RemoveFromStart(LoadedWorld->StreamingLevelsPrefix);

	const EKCLevelType NewLevelType = UKCLevelTypeLibrary::GetLevelType(FName(*MapName));

	FKCLevelChangedStruct Message;
	Message.NewLevelType = NewLevelType;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(KCGameplayTags::Message_Level_Changed, Message);

	UE_LOG(LogTemp, Log, TEXT("[LevelTransition] 레벨 전환 감지: %s (Type=%d)"),
		*MapName, static_cast<int32>(NewLevelType));
}