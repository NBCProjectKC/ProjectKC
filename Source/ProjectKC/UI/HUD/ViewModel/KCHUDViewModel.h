#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "ProjectKC/GameSystem/KCGamePhaseType.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDRecipeTypes.h"
#include "TimerManager.h"
#include "KCHUDViewModel.generated.h"

class UTexture2D;
class UKCItemDefinition;
class AKCPlayerState;
class UKCHUDRecipeViewModel;
struct FKCActiveRecipesChangedStruct;
struct FKCDishRuinedStruct;
struct FKCGamePhaseChangedStruct;
struct FKCPotIngredientsChangedStruct;
struct FKCPotProgressChangedStruct;
struct FKCScoreChangedStruct;

DECLARE_MULTICAST_DELEGATE(FKCRecipesChangedNativeDelegate);
DECLARE_MULTICAST_DELEGATE(FKCLocalDishRuinedNativeDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FKCTeamScoresChangedNativeDelegate, const TArray<int32>&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FKCTeamScoreAddedNativeDelegate, int32, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FKCMatchTimerChangedNativeDelegate, int32);

USTRUCT(BlueprintType)
struct FKCPotProgressViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	float ProgressPercent = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	int32 RemainingSeconds = 0;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FName RecipeRowName = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bVisible = false;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bCompleted = false;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FKCPotProgressChangedNativeDelegate, int32, const FKCPotProgressViewData&);

UCLASS(BlueprintType, Blueprintable, meta = (MVVMAllowedContextCreationType = "Manual|CreateInstance"))
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

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetCurrentPhase(EKCGamePhaseType NewPhase);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	int32 GetTeamScore(int32 TeamId) const;

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	FText GetTeamScoreText(int32 TeamId) const;

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetTeamScore(int32 TeamId, int32 NewScore);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	const TArray<int32>& GetTeamScores() const { return TeamScores; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetTeamScores(const TArray<int32>& NewTeamScores);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	int32 GetRemainingMatchSeconds() const { return RemainingMatchSeconds; }

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	FText GetRemainingMatchTimeText() const { return RemainingMatchTimeText; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetRemainingMatchSeconds(int32 NewRemainingSeconds);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	const TArray<FKCRecipeViewData>& GetRecipes() const { return Recipes; }

	const TArray<TObjectPtr<UKCHUDRecipeViewModel>>& GetRecipeViewModels() const { return RecipeViewModels; }

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	int32 GetLocalTeamId() const { return LocalTeamId; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetLocalTeamId(int32 NewTeamId);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetRecipes(const TArray<FKCRecipeViewData>& NewRecipes);

	UFUNCTION(BlueprintCallable, Category = "KC|UI|Preview")
	void SetPreviewData(const TArray<int32>& NewTeamScores, const TArray<FKCRecipeViewData>& NewRecipes);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	FKCPotProgressViewData GetPotProgress(int32 TeamId) const;

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	const TArray<FKCPotProgressViewData>& GetPotProgresses() const { return PotProgresses; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetPotProgress(
		int32 TeamId,
		float ProgressPercent,
		int32 RemainingSeconds,
		FName RecipeRowName,
		bool bVisible,
		bool bCompleted);

	FKCTeamScoresChangedNativeDelegate OnTeamScoresChangedNative;
	FKCTeamScoreAddedNativeDelegate OnTeamScoreAddedNative;
	FKCRecipesChangedNativeDelegate OnRecipesChangedNative;
	FKCMatchTimerChangedNativeDelegate OnMatchTimerChangedNative;
	FKCPotProgressChangedNativeDelegate OnPotProgressChangedNative;
	FKCLocalDishRuinedNativeDelegate OnLocalDishRuinedNative;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnScoreChanged(int32 TeamId, int32 NewScore);

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnGamePhaseChanged(EKCGamePhaseType NewPhase);

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnRecipesChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnPotIngredientsChanged(int32 TeamId);

private:
	UFUNCTION()
	void HandleLocalTeamIdChanged(int32 NewTeamId);

	void HandleScoreChanged(FGameplayTag Channel, const FKCScoreChangedStruct& Message);
	void HandleGamePhaseChanged(FGameplayTag Channel, const FKCGamePhaseChangedStruct& Message);
	void HandleActiveRecipesChanged(FGameplayTag Channel, const FKCActiveRecipesChangedStruct& Message);
	void HandleDishRuined(FGameplayTag Channel, const FKCDishRuinedStruct& Message);
	void HandlePotIngredientsChanged(FGameplayTag Channel, const FKCPotIngredientsChangedStruct& Message);
	void HandlePotProgressChanged(FGameplayTag Channel, const FKCPotProgressChangedStruct& Message);
	void SyncFromGameState();
	void RefreshMatchTimer();
	void StartMatchTimerRefresh();
	void StopMatchTimerRefresh();
	void SyncLocalTeamIdFromContext();
	void BindLocalTeamPlayerState(AKCPlayerState* PlayerState);
	void UnbindLocalTeamPlayerState();
	void RebuildSubmittedStates();
	void RebuildCookingStates();
	void RebuildRecipeViewModels();
	void RefreshRecipeViewModelLocalTeams();
	FKCRecipeViewData BuildRecipeViewData(FName RecipeRowName) const;
	TSoftObjectPtr<UTexture2D> FindIngredientIcon(const FGameplayTag& IngredientTag) const;
	static FText MakeDisplayNameFromTag(const FGameplayTag& Tag);
	static FText MakeMatchTimeText(int32 Seconds);

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	EKCGamePhaseType CurrentPhase = EKCGamePhaseType::Waiting;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TArray<int32> TeamScores;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	int32 RemainingMatchSeconds = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FText RemainingMatchTimeText;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TArray<FKCRecipeViewData> Recipes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UKCHUDRecipeViewModel>> RecipeViewModels;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	int32 LocalTeamId = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TArray<FKCPotProgressViewData> PotProgresses;

	UPROPERTY(Transient)
	TArray<FGameplayTagContainer> TeamPotIngredients;

	mutable TMap<FGameplayTag, TSoftObjectPtr<UTexture2D>> IngredientIconCache;

	TWeakObjectPtr<UObject> ListeningWorldContext;
	TWeakObjectPtr<AKCPlayerState> BoundLocalTeamPlayerState;
	FGameplayMessageListenerHandle ScoreChangedHandle;
	FGameplayMessageListenerHandle PhaseChangedHandle;
	FGameplayMessageListenerHandle ActiveRecipesChangedHandle;
	FGameplayMessageListenerHandle DishRuinedHandle;
	FGameplayMessageListenerHandle PotIngredientsChangedHandle;
	FGameplayMessageListenerHandle PotProgressChangedHandle;
	FTimerHandle MatchTimerRefreshHandle;
};
