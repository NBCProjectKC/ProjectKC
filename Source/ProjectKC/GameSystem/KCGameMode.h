#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "KCGameMode.generated.h"

class AKCGameState;
struct FKCIngredientSubmittedStruct;

UCLASS()
class PROJECTKC_API AKCGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AKCGameMode();

	// Debuging Method
	UFUNCTION(Exec)
	void Debug_SubmitIngredient(int32 TeamId, int32 Count = 1);
	
protected:
	// GameMode Base Methods
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

	// Game Rule
	UPROPERTY(EditDefaultsOnly, Category = "KC|Rule")
	int32 PlayersPerTeam = 3;
	UPROPERTY(EditDefaultsOnly, Category = "KC|Rule")
	int32 TotalIngredientCount = 5;
	UPROPERTY(EditDefaultsOnly, Category = "KC|Rule")
	int32 TargetScore = 3;
	UPROPERTY(EditDefaultsOnly, Category = "KC|Rule")
	int32 TeamCount = 2;

private:
	int32 GetRequiredPlayerCount() const
	{
		return TeamCount * PlayersPerTeam;
	}

	// Judging Rule
	void OnIngredientSubmitted(FGameplayTag Channel, const FKCIngredientSubmittedStruct& Message);
	bool IsTargetScoreReached(int32& OutWinningTeamId) const;
	bool IsColdGameTriggered(int32& OutWinningTeamId) const;
	void CheckWinCondition();
	void EndGame(int32 WinningTeamId);

	FGameplayMessageListenerHandle IngredientSubmittedListenerHandle;
	int32 SubmittedIngredientCount = 0;

	UPROPERTY()
	TObjectPtr<AKCGameState> KCGameState;
};
