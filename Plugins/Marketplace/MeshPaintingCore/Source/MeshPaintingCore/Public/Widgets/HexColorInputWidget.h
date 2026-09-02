// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/ColorPickerTypes.h"
#include "HexColorInputWidget.generated.h"

class SEditableTextBox;
class UNativeWidgetHost;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnHexColorCommitted,
	const FString&, HexText,
	FLinearColor, Color,
	bool, bIsValid,
	bool, bHasAlpha);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHexColorValidityChanged, bool, bIsValid);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Hex Color Input Widget"))
class MESHPAINTINGCORE_API UHexColorInputWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UHexColorInputWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Color")
	EHexColorInputMode InputMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Color")
	FString HexText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Color")
	bool bIsValid;

	UPROPERTY(BlueprintAssignable, Category = "Hex Color|Events")
	FOnHexColorCommitted OnHexCommitted;

	UPROPERTY(BlueprintAssignable, Category = "Hex Color|Events")
	FOnHexColorValidityChanged OnValidityChanged;

	UFUNCTION(BlueprintCallable, Category = "Hex Color")
	void SetHexText(const FString& NewHexText, bool bBroadcastValidity = false);

	UFUNCTION(BlueprintCallable, Category = "Hex Color")
	void SetHexFromColor(FLinearColor Color, bool bIncludeAlpha = true);

	UFUNCTION(BlueprintCallable, Category = "Hex Color")
	void SetInputMode(EHexColorInputMode NewInputMode);

	UFUNCTION(BlueprintPure, Category = "Hex Color")
	bool ValidateHexText(const FString& TextToValidate, FLinearColor& OutColor, bool& bOutHasAlpha) const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;
	virtual void NativePreConstruct() override;

private:
	TSharedRef<SWidget> BuildSlateWidget();
	TSharedRef<SWidget> MakeHexModeMenu();
	FText GetModeButtonText() const;
	FText GetHexBoxText() const;
	void SynchronizeSlateWidget();
	void UpdateValidationState(bool bNewValid, bool bBroadcast);
	void HandleModeSelected(EHexColorInputMode NewInputMode);

	void HandleTextChanged(const FText& NewText);
	void HandleTextCommitted(const FText& NewText, ETextCommit::Type CommitMethod);

	UPROPERTY(Transient)
	TObjectPtr<UNativeWidgetHost> NativeWidgetHost;

	TSharedPtr<SEditableTextBox> TextBox;
	FLinearColor CachedColor;
	bool bIsSynchronizing;
};
