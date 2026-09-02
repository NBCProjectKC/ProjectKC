// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Widgets/ColorPickerTypes.h"
#include "HorizontalColorBarWidget.generated.h"

class SRuntimeMeshPaintColorSlider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHorizontalColorBarValueChanged, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHorizontalColorBarInteractionStarted);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Horizontal Color Bar Widget"))
class MESHPAINTINGCORE_API UHorizontalColorBarWidget : public UWidget
{
	GENERATED_BODY()

public:
	UHorizontalColorBarWidget();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Bar")
	float Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Bar")
	float MinValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Bar")
	float MaxValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Bar")
	FLinearColor StartColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Bar")
	FLinearColor EndColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Bar")
	EColorBarGradientMode GradientMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector2D DesiredBarSize;

	UPROPERTY(BlueprintAssignable, Category = "Color Bar|Events")
	FOnHorizontalColorBarValueChanged OnValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "Color Bar|Events")
	FOnHorizontalColorBarInteractionStarted OnInteractionStarted;

	UPROPERTY(BlueprintAssignable, Category = "Color Bar|Events")
	FOnHorizontalColorBarValueChanged OnValueCommitted;

	UFUNCTION(BlueprintCallable, Category = "Color Bar")
	void SetValue(float NewValue, bool bBroadcast = false);

	UFUNCTION(BlueprintPure, Category = "Color Bar")
	float GetValue() const { return Value; }

	UFUNCTION(BlueprintCallable, Category = "Color Bar")
	void SetRange(float NewMinValue, float NewMaxValue);

	UFUNCTION(BlueprintCallable, Category = "Color Bar")
	void SetGradientColors(FLinearColor NewStartColor, FLinearColor NewEndColor);

	UFUNCTION(BlueprintCallable, Category = "Color Bar")
	void SetGradientMode(EColorBarGradientMode NewGradientMode);

	UFUNCTION(BlueprintPure, Category = "Color Bar")
	float GetNormalizedValue() const;

	TArray<FLinearColor> GetGradientColors() const;
	bool HasAlphaBackground() const;

	void HandleSlateValueChanged(float NewValue);
	void HandleSlateInteractionStarted();
	void HandleSlateInteractionFinished(float NewValue);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

private:
	TSharedPtr<SRuntimeMeshPaintColorSlider> MyColorBar;
};
