#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameplayTagContainer.h"
#include "GameSystem/KCGamePhaseType.h"
#include "KCGameState.generated.h"

class UDataTable;
struct FKCRecipeStruct;

/**
 * 공용 데이터 보관소
 * - 점수, 게임 페이즈, 냄비에 투입된 재료 등
 */
UCLASS()
class PROJECTKC_API AKCGameState : public AGameState
{
	GENERATED_BODY()

public:
	AKCGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void InitializeTeamCount(int32 InTeamCount);
	
	// 매치 시작 시각 저장
	UPROPERTY(Replicated)
	float MatchStartServerTime = 0.0f;
	void SetMatchStartServerTime(float InTime) { MatchStartServerTime = InTime; }
	float GetMatchStartServerTime() const { return MatchStartServerTime; }

	UPROPERTY(Replicated)
	float MatchEndServerTime = 0.0f;
	void SetMatchEndServerTime(float InTime) { MatchEndServerTime = InTime; }
	float GetMatchEndServerTime() const { return MatchEndServerTime; }
	bool HasMatchTimerStarted() const { return MatchEndServerTime > MatchStartServerTime; }
	int32 GetRemainingMatchSeconds(float CurrentServerTime) const;

	UPROPERTY(Replicated)
	float CountdownEndServerTime = 0.0f;
	void SetCountdownEndServerTime(float InTime) { CountdownEndServerTime = InTime; }
	UFUNCTION(BlueprintPure, Category = "KC|GameState")
	float GetCountdownEndServerTime() const { return CountdownEndServerTime; }
	UFUNCTION(BlueprintPure, Category = "KC|GameState")
	int32 GetRemainingCountdownSeconds(float CurrentServerTime) const;

	UPROPERTY(Replicated)
	float ResultScreenEndServerTime = 0.0f;
	void SetResultScreenEndServerTime(float InTime) { ResultScreenEndServerTime = InTime; }
	float GetResultScreenEndServerTime() const { return ResultScreenEndServerTime; }
	int32 GetRemainingResultScreenSeconds(float CurrentServerTime) const;
	void SetWinningTeamId(int32 InWinningTeamId);
	int32 GetWinningTeamId() const { return WinningTeamId; }
	
	// setter
	void SetGamePhase(EKCGamePhaseType NewPhase);
	void SetTeamScore(int32 TeamId, int32 NewScore);
	void SetPotIngredients(int32 TeamId, const FGameplayTagContainer& NewIngredients);
	void SetActiveRecipes(const TArray<FName>& InRecipeRowNames);

	// getter
	UFUNCTION(BlueprintPure, Category = "KC|GameState")
	EKCGamePhaseType GetGamePhase() const
	{
		return CurrentPhase;
	}
	UFUNCTION(BlueprintPure, Category = "KC|GameState")
	int32 GetTeamScore(int32 TeamId) const;
	
	// UI : 냄비에 투입된 재료 조회 -> 재료 아이콘 회색/컬러 표시
	UFUNCTION(BlueprintPure, Category = "KC|GameState")
	FGameplayTagContainer GetPotIngredients(int32 TeamId) const;

	// UI : 이번 판의 레시피 3종 조회 -> 레시피 표시
	UFUNCTION(BlueprintPure, Category = "KC|GameState")
	const TArray<FName>& GetActiveRecipes() const { return ActiveRecipeRowNames; }
	
	// UI, GameMode : 레시피 이름으로 그 레시피의 정보 조회 함수
	const FKCRecipeStruct* FindRecipeByRowName(FName RowName) const;

	// GameMode : 레시피 랜덤 선정용 
	TArray<FName> GetAllRecipeRowNames() const;
	// 클라이언트 알림: 요리가 망할 때, 요리가 시작될 때
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyDishRuined(int32 TeamId);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyRecipeCompleted(int32 TeamId, FName RecipeRowName);
	
	// 접시 덮개 오픈 전 재료 습득 방어
	void SetFarmingOpen(bool bOpen);
    UFUNCTION(BlueprintPure, Category = "KC|GameState")
    bool IsFarmingOpen() const { return bIsFarmingOpen; }

protected:
	// Recipe DT ptr
	UPROPERTY(EditDefaultsOnly, Category = "KC|Recipe")
	TObjectPtr<UDataTable> RecipeDataTable;
	
	// default phase : waiting
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase)
	EKCGamePhaseType CurrentPhase = EKCGamePhaseType::Waiting;

	// Index : TeamId
	UPROPERTY(ReplicatedUsing = OnRep_TeamScores)
	TArray<int32> TeamScores;

	// Index : TeamId. 팀별 현재 냄비에 투입된 재료
	UPROPERTY(ReplicatedUsing = OnRep_PotIngredients)
	TArray<FGameplayTagContainer> PotIngredients;

	// 이번 매치의 레시피 3종 (DataTable RowName)
	UPROPERTY(ReplicatedUsing = OnRep_ActiveRecipes)
	TArray<FName> ActiveRecipeRowNames;

	UPROPERTY(Replicated)
	int32 WinningTeamId = INDEX_NONE;
	
	// 덮개 오픈 여부
	UPROPERTY(Replicated)
	bool bIsFarmingOpen = true;

	UFUNCTION()
	void OnRep_CurrentPhase();

	UFUNCTION()
	void OnRep_TeamScores();

	UFUNCTION()
	void OnRep_PotIngredients();

	UFUNCTION()
	void OnRep_ActiveRecipes();
};
