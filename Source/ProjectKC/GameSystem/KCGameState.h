#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameplayTagContainer.h"
#include "KCGamePhaseType.h"
#include "KCGameState.generated.h"

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

protected:
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

	UFUNCTION()
	void OnRep_CurrentPhase();

	UFUNCTION()
	void OnRep_TeamScores();

	UFUNCTION()
	void OnRep_PotIngredients();

	UFUNCTION()
	void OnRep_ActiveRecipes();
};
