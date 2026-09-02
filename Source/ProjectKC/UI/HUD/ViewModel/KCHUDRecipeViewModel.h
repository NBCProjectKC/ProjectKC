#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDRecipeTypes.h"
#include "KCHUDRecipeViewModel.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FKCRecipeViewModelChangedNativeDelegate, const FKCRecipeViewData&);

UCLASS(BlueprintType, Blueprintable, meta = (MVVMAllowedContextCreationType = "Manual|CreateInstance"))
class PROJECTKC_API UKCHUDRecipeViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "KC|UI")
	const FKCRecipeViewData& GetRecipe() const { return Recipe; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetRecipe(const FKCRecipeViewData& NewRecipe);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	int32 GetLocalTeamId() const { return LocalTeamId; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetLocalTeamId(int32 NewLocalTeamId);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	bool IsDisabledForLocalTeam() const { return bDisabledForLocalTeam; }

	FKCRecipeViewModelChangedNativeDelegate OnRecipeChangedNative;

private:
	void RefreshLocalState();

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FKCRecipeViewData Recipe;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	int32 LocalTeamId = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter = "IsDisabledForLocalTeam", Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	bool bDisabledForLocalTeam = false;
};
