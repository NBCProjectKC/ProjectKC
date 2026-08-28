#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "KCGameMode.generated.h"

class AKCGameState;
class UDataTable;
struct FKCIngredientSubmittedStruct;
struct FKCDishFinishedStruct;
struct FKCRecipeStruct;

/**
 * L_GasRange 레벨 전용 GameMode
 - 매치 시작 시 레시피 3종 랜덤 배정
 - 재료 투입 판정 (요리시작 / 대기 / 망)
 - 요리 완성 시 점수 반영, 승리 조건 판정
 */

UCLASS()
class PROJECTKC_API AKCGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AKCGameMode();

	// Debuging Method : "Debug_SubmitIngredient 0 Item.Id.xxx"
	UFUNCTION(Exec)
	void Debug_SubmitIngredient(int32 TeamId, FString IngredientTagName);
	// Debuging Method : "Debug_FinishDish 0 Recipe_Simple"
	UFUNCTION(Exec)
	void Debug_FinishDish(int32 TeamId, FName RecipeRowName);
	// 즉시 승리 디버깅 함수
	UFUNCTION(Exec)
	void Debug_WinMatch(int32 WinningTeamId);
		
	// 이탈/재접속
	virtual void Logout(AController* Exiting) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
protected:
	// GameMode Methods
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

	// Game Rule
	// 전체 레시피 목록
	UPROPERTY(EditDefaultsOnly, Category = "KC|Recipe")
	TObjectPtr<UDataTable> RecipeDataTable;
	// 매칭 시 레시피 랜덤 선정
	virtual TArray<FName> SelectActiveRecipes() const;
	// 한 매치에 배정할 레시피 개수
	UPROPERTY(EditDefaultsOnly, Category = "KC|Recipe")
	int32 ActiveRecipeCount = 3;
	// 팀 수
	UPROPERTY(EditDefaultsOnly, Category = "KC|Rule")
	int32 TeamCount = 2;
	// 팀당 인원수
	UPROPERTY(EditDefaultsOnly, Category = "KC|Rule")
	int32 PlayersPerTeam = 3;
	// 승리점수
	UPROPERTY(EditDefaultsOnly, Category = "KC|Rule")
	int32 TargetScore = 10;
	// 한 판 종료 후 결과화면 보는 시간(= Ending Phase 시작부터 SeverTravel 하기까지의 시간)
	UPROPERTY(EditDefaultsOnly, Category = "KC|Rule")
	float ResultScreenDuration = 10.f;


	// 시드 고정
	UPROPERTY(EditDefaultsOnly, Category = "KC|Recipe|Debug")
	bool bUseFixedRecipeSeed = false;
	UPROPERTY(EditDefaultsOnly, Category = "KC|Recipe|Debug", meta = (EditCondition = "bUseFixedRecipeSeed"))
	int32 FixedRecipeSeed = 404;
	
private:
	int32 GetRequiredPlayerCount() const
	{
		return TeamCount * PlayersPerTeam;
	}
	
	// Multicast RPC로 클라이언트 알림 : 요리가 망할 때, 요리가 시작할 때
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyDishRuined(int32 TeamId);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_NotifyRecipeCompleted(int32 TeamId, FName RecipeRowName);
	
	// Callback
	void OnIngredientSubmitted(FGameplayTag Channel, const FKCIngredientSubmittedStruct& Message);
	void OnDishFinished(FGameplayTag Channel, const FKCDishFinishedStruct& Message);

	// 재료 투입 시 판단 -> 요리시작/대기/망
	void ProcessIngredientSubmission(int32 TeamId, const FGameplayTag& IngredientId);

	// 레시피 완성 판단
	bool FindCompletedRecipe(const FGameplayTagContainer& CurrentIngredients, FName& OutRecipeRowName) const;

	// 재료 잘못 투입했나 판단
	bool HasAnyViableRecipe(const FGameplayTagContainer& CurrentIngredients) const;

	// 레시피 데이터 조회
	const FKCRecipeStruct* FindRecipeByRowName(FName RowName) const;

	bool IsTargetScoreReached(int32& OutWinningTeamId) const;
	void CheckWinCondition();
	void EndGame(int32 WinningTeamId);
	
	// 로비 세션으로 이동(EndGame() 내부에서 타이머 끝나면 실행됨)
	void TravelBackToLobby();
	FTimerHandle ResultScreenTimerHandle;
	
	FGameplayMessageListenerHandle IngredientSubmittedListenerHandle;
	FGameplayMessageListenerHandle DishFinishedListenerHandle;

	UPROPERTY()
	TObjectPtr<AKCGameState> KCGameState;
};
