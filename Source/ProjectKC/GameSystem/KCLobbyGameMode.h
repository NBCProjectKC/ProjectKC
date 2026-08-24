#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "KCLobbyGameMode.generated.h"


//로비 레벨 전용 GameMode
// 인원이 다 차면 전투 레벨로 이동

UCLASS()
class PROJECTKC_API AKCLobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AKCLobbyGameMode();
	
	// To.고은님 세션 생성 시점에 호출해서 (팀수x팀당인원) 값을 넘겨주세요
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void SetRequiredPlayerCount(int32 InCount);

protected:
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleMatchHasStarted() override;

	// TODO: 실제 전투 레벨 이름으로 교체 필요
	UPROPERTY(EditDefaultsOnly, Category = "KC|Lobby")
	FString BattleLevelName = TEXT("Lvl_Battle");
	
private:
	// 기본값
	int32 RequiredPlayerCount = 6;
};