// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "ProjectKC/UI/HUD/ViewModel/KCHUDRecipeTypes.h"
#include "UI/Common/Widget/KCUserWidget.h"
#include "KCRecipeEntryIngredient.generated.h"

class UImage;
class UBorder;
class UKCColorStyle;

DECLARE_MULTICAST_DELEGATE_OneParam(FKCRecipeIngredientListItemChangedNativeDelegate, const FKCRecipeIngredientViewData&);

UCLASS()
class PROJECTKC_API UKCRecipeIngredientListItem : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetIngredient(const FKCRecipeIngredientViewData& NewIngredient);

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FKCRecipeIngredientViewData Ingredient;

	FKCRecipeIngredientListItemChangedNativeDelegate OnIngredientChangedNative;
};

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCRecipeEntryIngredient : public UKCUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetIngredient(const FKCRecipeIngredientViewData& Ingredient);

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnEntryReleased() override;
	virtual void NativeApplyColorStyle(const UKCColorStyle* InColorStyle) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UImage> IngredientImage;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UBorder> BackBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UBorder> Team1CheckBoxBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UImage> Team1InnerImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UBorder> Team2CheckBoxBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UImage> Team2InnerImage;

private:
	void BindIngredientListItem(UKCRecipeIngredientListItem* IngredientItem);
	void UnbindIngredientListItem();
	void HandleIngredientListItemChanged(const FKCRecipeIngredientViewData& Ingredient);
	void RefreshSubmittedTeamImages();

	FKCRecipeIngredientViewData CurrentIngredient;
	TWeakObjectPtr<UKCRecipeIngredientListItem> BoundIngredientItem;
	FDelegateHandle IngredientChangedHandle;
};
