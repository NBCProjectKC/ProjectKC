// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ColorOptionRowWidget.generated.h"

class UCheckBox;
class UHorizontalBox;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorOptionChanged, FName, OptionId, bool, bIsChecked);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Color Option Row Widget"))
class MESHPAINTINGCORE_API UColorOptionRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UColorOptionRowWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Option")
	FName OptionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Option")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Option")
	bool bIsChecked;

	UPROPERTY(BlueprintAssignable, Category = "Color Option|Events")
	FOnColorOptionChanged OnCheckStateChanged;

	UFUNCTION(BlueprintCallable, Category = "Color Option")
	void SetIsChecked(bool bNewChecked, bool bBroadcast = false);

	UFUNCTION(BlueprintCallable, Category = "Color Option")
	void SetLabel(FText NewLabel);

	UFUNCTION(BlueprintPure, Category = "Color Option")
	bool GetIsChecked() const { return bIsChecked; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

private:
	void BuildWidgetTree();
	void SynchronizeChildWidgets();

	UFUNCTION()
	void HandleCheckStateChanged(bool bNewChecked);

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> RootHorizontalBox;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> CheckBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LabelText;

	bool bIsSynchronizing;
	bool bNativeTreeBuilt;
};
