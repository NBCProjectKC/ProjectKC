// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Styling/SlateBrush.h"
#include "ColorPickerToolStripWidget.generated.h"

class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorPickerToolStripButtonClicked, FName, ToolId);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Color Picker Tool Strip Widget"))
class MESHPAINTINGCORE_API UColorPickerToolStripWidget : public UWidget
{
	GENERATED_BODY()

public:
	UColorPickerToolStripWidget();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tool Strip")
	bool bUseSpectrumMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tool Strip")
	bool bIsEyedropperActive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tool Strip")
	bool bIsThemePanelVisible;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tool Strip")
	bool bIsEraserActive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tool Strip")
	TObjectPtr<UTexture2D> EraserIconTexture;

	UPROPERTY(BlueprintAssignable, Category = "Tool Strip|Events")
	FOnColorPickerToolStripButtonClicked OnButtonClicked;

	UFUNCTION(BlueprintCallable, Category = "Tool Strip")
	void SetUseSpectrumMode(bool bNewUseSpectrumMode);

	UFUNCTION(BlueprintPure, Category = "Tool Strip")
	bool GetUseSpectrumMode() const { return bUseSpectrumMode; }

	UFUNCTION(BlueprintCallable, Category = "Tool Strip")
	void SetEyedropperActive(bool bNewIsActive);

	UFUNCTION(BlueprintCallable, Category = "Tool Strip")
	void SetThemePanelVisible(bool bNewIsVisible);

	UFUNCTION(BlueprintCallable, Category = "Tool Strip")
	void SetEraserActive(bool bNewIsActive);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

private:
	FReply HandleColorPickerModeClicked();
	FReply HandleEyeDropperClicked();
	FReply HandleThemePanelClicked();
	FReply HandleEraserClicked();

	EVisibility GetModeWheelVisibility() const;
	EVisibility GetModeSpectrumVisibility() const;
	EVisibility GetEyeDropperEscapeTextVisibility() const;
	FSlateColor GetEyeDropperImageColor() const;
	FSlateColor GetEraserImageColor() const;
	const FSlateBrush* GetThemePanelButtonImageBrush() const;
	const FSlateBrush* GetEraserImageBrush() const;
	void UpdateEraserIconBrush();
	void InvalidateToolStrip();

	FSlateBrush EraserIconBrush;
	TSharedPtr<SWidget> MyToolStrip;
};
