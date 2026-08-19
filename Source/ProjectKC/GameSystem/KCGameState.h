#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "KCGamePhaseType.h"
#include "KCGameState.generated.h"


UCLASS()
class PROJECTKC_API AKCGameState : public AGameState
{
	GENERATED_BODY()

public:
	AKCGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// setter
	void InitializeTeamCount(int32 InTeamCount);
	void SetGamePhase(EKCGamePhaseType NewPhase);
	void SetTeamScore(int32 TeamId, int32 NewScore);

	// getter
	UFUNCTION(BlueprintPure, Category = "KC|GameState")
	EKCGamePhaseType GetGamePhase() const
	{
		return CurrentPhase;
	}
	UFUNCTION(BlueprintPure, Category = "KC|GameState")
	int32 GetTeamScore(int32 TeamId) const;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase)
	EKCGamePhaseType CurrentPhase = EKCGamePhaseType::Waiting;

	UPROPERTY(ReplicatedUsing = OnRep_TeamScores)
	TArray<int32> TeamScores;

	UFUNCTION()
	void OnRep_CurrentPhase();

	UFUNCTION()
	void OnRep_TeamScores();
};
