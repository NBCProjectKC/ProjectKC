#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "ProjectKC/GameSystem/KCGamePhaseType.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "KCHUDViewModel.generated.h"

struct FKCGamePhaseChangedStruct;
struct FKCScoreChangedStruct;

UCLASS(BlueprintType)
class PROJECTKC_API UKCHUDViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void StartListening(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void StopListening();

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	EKCGamePhaseType GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	int32 GetTeamScore(int32 TeamId) const;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnScoreChanged(int32 TeamId, int32 NewScore);

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnGamePhaseChanged(EKCGamePhaseType NewPhase);

private:
	void HandleScoreChanged(FGameplayTag Channel, const FKCScoreChangedStruct& Message);
	void HandleGamePhaseChanged(FGameplayTag Channel, const FKCGamePhaseChangedStruct& Message);

	UPROPERTY(BlueprintReadOnly, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	EKCGamePhaseType CurrentPhase = EKCGamePhaseType::Waiting;

	UPROPERTY(BlueprintReadOnly, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TArray<int32> TeamScores;

	TWeakObjectPtr<UObject> ListeningWorldContext;
	FGameplayMessageListenerHandle ScoreChangedHandle;
	FGameplayMessageListenerHandle PhaseChangedHandle;
};
