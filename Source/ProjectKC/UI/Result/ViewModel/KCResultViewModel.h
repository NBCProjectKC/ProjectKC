#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "KCResultViewModel.generated.h"

USTRUCT(BlueprintType)
struct FKCResultTeamViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	int32 Score = 0;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	int32 Rank = 0;
};

UCLASS(BlueprintType, Blueprintable, meta = (MVVMAllowedContextCreationType = "Manual|CreateInstance"))
class PROJECTKC_API UKCResultViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "KC|UI")
	const TArray<FKCResultTeamViewData>& GetTeams() const { return Teams; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetTeams(const TArray<FKCResultTeamViewData>& NewTeams);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	int32 GetRemainingBackToLobbySeconds() const { return RemainingBackToLobbySeconds; }

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	FText GetRemainingBackToLobbySecondText() const { return RemainingBackToLobbySecondText; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetRemainingBackToLobbySeconds(int32 NewRemainingSeconds);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	int32 GetWinningTeamId() const { return WinningTeamId; }

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	FText GetWinningTeamText() const { return WinningTeamText; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetWinningTeamId(int32 NewWinningTeamId);

	UFUNCTION(BlueprintCallable, Category = "KC|UI|Preview")
	void SetPreviewData(const TArray<FKCResultTeamViewData>& NewTeams);

private:
	static FText MakeRemainingSecondText(int32 RemainingSeconds);
	static FText MakeWinningTeamText(int32 InWinningTeamId);

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TArray<FKCResultTeamViewData> Teams;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	int32 RemainingBackToLobbySeconds = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FText RemainingBackToLobbySecondText;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	int32 WinningTeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FText WinningTeamText;
};
