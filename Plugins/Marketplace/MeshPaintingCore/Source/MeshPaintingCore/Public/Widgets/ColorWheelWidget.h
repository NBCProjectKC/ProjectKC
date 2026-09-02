// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "ColorWheelWidget.generated.h"

class SColorWheel;
class SColorSpectrum;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorWheelHueSaturationChanged, float, Hue, float, Saturation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnColorWheelHSVChanged, float, Hue, float, Saturation, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnColorWheelInteractionStarted);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Color Wheel Widget"))
class MESHPAINTINGCORE_API UColorWheelWidget : public UWidget
{
	GENERATED_BODY()

public:
	UColorWheelWidget();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Wheel", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float Hue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Wheel", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Saturation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Wheel", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector2D DesiredWheelSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Wheel")
	bool bUseSpectrumMode;

	UPROPERTY(BlueprintAssignable, Category = "Color Wheel|Events")
	FOnColorWheelHueSaturationChanged OnHueSaturationChanged;

	UPROPERTY(BlueprintAssignable, Category = "Color Wheel|Events")
	FOnColorWheelHSVChanged OnHSVChanged;

	UPROPERTY(BlueprintAssignable, Category = "Color Wheel|Events")
	FOnColorWheelInteractionStarted OnInteractionStarted;

	UPROPERTY(BlueprintAssignable, Category = "Color Wheel|Events")
	FOnColorWheelHueSaturationChanged OnHueSaturationCommitted;

	UPROPERTY(BlueprintAssignable, Category = "Color Wheel|Events")
	FOnColorWheelHSVChanged OnHSVCommitted;

	UFUNCTION(BlueprintCallable, Category = "Color Wheel")
	void SetHueSaturation(float NewHue, float NewSaturation, bool bBroadcast = false);

	UFUNCTION(BlueprintCallable, Category = "Color Wheel")
	void SetValueLevel(float NewValue);

	UFUNCTION(BlueprintCallable, Category = "Color Wheel")
	void SetUseSpectrumMode(bool bNewUseSpectrumMode);

	UFUNCTION(BlueprintPure, Category = "Color Wheel")
	float GetHue() const { return Hue; }

	UFUNCTION(BlueprintPure, Category = "Color Wheel")
	float GetSaturation() const { return Saturation; }

	UFUNCTION(BlueprintPure, Category = "Color Wheel")
	float GetValue() const { return Value; }

	UFUNCTION(BlueprintPure, Category = "Color Wheel")
	bool GetUseSpectrumMode() const { return bUseSpectrumMode; }

	FLinearColor GetSelectedColorHSV() const;
	void HandleSlateValueChanged(FLinearColor NewHSVColor);
	void HandleSlateInteractionStarted();
	void HandleSlateInteractionFinished();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

private:
	void InvalidateSlateWidgets();
	EVisibility GetWheelVisibility() const;
	EVisibility GetSpectrumVisibility() const;

	TSharedPtr<SColorWheel> MyColorWheel;
	TSharedPtr<SColorSpectrum> MyColorSpectrum;
};
