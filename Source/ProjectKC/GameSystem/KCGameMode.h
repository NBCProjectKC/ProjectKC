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
class AKCPlayerState;

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
	// 테스트용 레시피 고정 구현 -> 로직에 영향x 삭제예정
	UPROPERTY(EditDefaultsOnly, Category = "KC|Recipe|Debug")
	TObjectPtr<UDataTable> DebugRecipeDataTable;
	UPROPERTY(EditDefaultsOnly, Category = "KC|Recipe|Debug", meta = (EditCondition = "bUseFixedRecipeList", GetOptions = "GetRecipeRowNameOptions"))
	TArray<FName> FixedRecipeList;
	UFUNCTION()
	TArray<FName> GetRecipeRowNameOptions() const;	
	
	// 이탈 시 슬롯 정보 저장
	virtual void Logout(AController* Exiting) override;
	
	// 결과화면 조기 스킵 요청 (클라이언트 RPC가 호출함)
	void RequestEarlyTravelToLobby(AKCPlayerState* RequestingPlayer);
	
protected:
	// GameMode Methods
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	
	// 로비 슬롯 번호에 대응하는 인게임 시작점을 선택합니다.
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	// 심리스 트래블 시 타는 경로
    virtual void InitSeamlessTravelPlayer(AController* NewController) override;
	virtual bool UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage) override;
	
	// Game Rule
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

	UPROPERTY(EditDefaultsOnly, Category = "KC|Rule")
	float MatchDurationSeconds = 300.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "KC|Recipe|Debug")
	bool bUseFixedRecipeList = true;
	
private:
	int32 GetRequiredPlayerCount() const;
	// Callback
	void OnIngredientSubmitted(FGameplayTag Channel, const FKCIngredientSubmittedStruct& Message);
	void OnDishFinished(FGameplayTag Channel, const FKCDishFinishedStruct& Message);

	// 재료 투입 시 판단 -> 요리시작/대기/망
	void ProcessIngredientSubmission(int32 TeamId, const FGameplayTag& IngredientId);

	// 레시피 완성 판단
	bool FindCompletedRecipe(const FGameplayTagContainer& CurrentIngredients, FName& OutRecipeRowName) const;

	// 재료 잘못 투입했나 판단
	bool HasAnyViableRecipe(const FGameplayTagContainer& CurrentIngredients) const;

	bool IsTargetScoreReached(int32& OutWinningTeamId) const;
	void CheckWinCondition();
	void EndGame(int32 WinningTeamId);
	void HandleMatchTimeExpired();
	int32 GetLeadingTeamId() const;
	
	// 로비 세션으로 이동(EndGame() 내부에서 타이머 끝나면 실행됨)
	void TravelBackToLobby();
	FTimerHandle ResultScreenTimerHandle;
	FTimerHandle MatchTimerHandle;
	
	FGameplayMessageListenerHandle IngredientSubmittedListenerHandle;
	FGameplayMessageListenerHandle DishFinishedListenerHandle;

	UPROPERTY()
	TObjectPtr<AKCGameState> KCGameState;
	
	/** 결과화면에서 스킵을 누른 플레이어 목록 (UniqueNetId 또는 PlayerState 포인터로 추적) */
	UPROPERTY()
	TSet<TWeakObjectPtr<AKCPlayerState>> SkippedResultScreenPlayers;
	
	// 두 경로 공통으로 쓰는 슬롯/팀 복원 로직 (위치 계산과 무관, 순수 데이터 세팅만 담당)
	// 신규/재접속, seamless travel
	void RestoreSlotDataForController(AController* Controller);
};

