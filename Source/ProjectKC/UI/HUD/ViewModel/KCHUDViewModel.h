#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "ProjectKC/GameSystem/KCGamePhaseType.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "KCHUDViewModel.generated.h"

class UTexture2D;
class UDataTable;
struct FKCActiveRecipesChangedStruct;
struct FKCGamePhaseChangedStruct;
struct FKCPotIngredientsChangedStruct;
struct FKCScoreChangedStruct;

USTRUCT(BlueprintType)
struct FKCRecipeIngredientViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI", meta = (Categories = "Item.Id"))
	FGameplayTag IngredientId;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bSubmitted = false;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	int32 SubmittedTeamId = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FKCRecipeViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI", meta = (ClampMin = "0", ClampMax = "5"))
	int32 DifficultyStars = 0;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FName RecipeRowName;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	TSoftObjectPtr<UTexture2D> FoodIcon;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	TArray<FKCRecipeIngredientViewData> Ingredients;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	float Team0Progress = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	float Team1Progress = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bTeam0ProgressVisible = false;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bTeam1ProgressVisible = false;
};

DECLARE_MULTICAST_DELEGATE(FKCRecipesChangedNativeDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FKCTeamScoresChangedNativeDelegate, const TArray<int32>&);

UCLASS(BlueprintType, Blueprintable, meta = (MVVMAllowedContextCreationType = "Manual|CreateInstance"))
class PROJECTKC_API UKCHUDViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void StartListening(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void StopListening();

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetRecipeDataTable(UDataTable* InRecipeDataTable);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	EKCGamePhaseType GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetCurrentPhase(EKCGamePhaseType NewPhase);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	int32 GetTeamScore(int32 TeamId) const;

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetTeamScore(int32 TeamId, int32 NewScore);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	const TArray<int32>& GetTeamScores() const { return TeamScores; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetTeamScores(const TArray<int32>& NewTeamScores);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	const TArray<FKCRecipeViewData>& GetRecipes() const { return Recipes; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetRecipes(const TArray<FKCRecipeViewData>& NewRecipes);

	UFUNCTION(BlueprintCallable, Category = "KC|UI|Preview")
	void SetPreviewData(const TArray<int32>& NewTeamScores, const TArray<FKCRecipeViewData>& NewRecipes);

	FKCTeamScoresChangedNativeDelegate OnTeamScoresChangedNative;
	FKCRecipesChangedNativeDelegate OnRecipesChangedNative;

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
	void HandleScoreChanged(FGameplayTag Channel, const FKCScoreChangedStruct& Message);
	void HandleGamePhaseChanged(FGameplayTag Channel, const FKCGamePhaseChangedStruct& Message);
	void HandleActiveRecipesChanged(FGameplayTag Channel, const FKCActiveRecipesChangedStruct& Message);
	void HandlePotIngredientsChanged(FGameplayTag Channel, const FKCPotIngredientsChangedStruct& Message);
	void RebuildSubmittedStates();
	FKCRecipeViewData BuildRecipeViewData(FName RecipeRowName) const;
	static FText MakeDisplayNameFromTag(const FGameplayTag& Tag);

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	EKCGamePhaseType CurrentPhase = EKCGamePhaseType::Waiting;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TArray<int32> TeamScores;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TArray<FKCRecipeViewData> Recipes;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> RecipeDataTable;

	UPROPERTY(Transient)
	TArray<FGameplayTagContainer> TeamPotIngredients;

	TWeakObjectPtr<UObject> ListeningWorldContext;
	FGameplayMessageListenerHandle ScoreChangedHandle;
	FGameplayMessageListenerHandle PhaseChangedHandle;
	FGameplayMessageListenerHandle ActiveRecipesChangedHandle;
	FGameplayMessageListenerHandle PotIngredientsChangedHandle;
};
