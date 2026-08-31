#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "KCLevelTransitionSubsystem.generated.h"

/**
 * 레벨 전환을 감지하고 알리는 서브시스템입니다.
 * 
 */
UCLASS()
class PROJECTKC_API UKCLevelTransitionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void OnLevelLoaded(UWorld* LoadedWorld);

	FDelegateHandle PostLoadMapWithWorldHandle;
};