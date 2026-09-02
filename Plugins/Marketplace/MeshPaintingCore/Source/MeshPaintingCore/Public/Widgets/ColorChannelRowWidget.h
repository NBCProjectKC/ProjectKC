// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/ColorPickerTypes.h"
#include "ColorChannelRowWidget.generated.h"

class SRuntimeMeshPaintColorSlider;
class UNativeWidgetHost;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorChannelValueChanged, FName, ChannelId, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorChannelValueCommitted, FName, ChannelId, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorChannelInteractionStarted, FName, ChannelId, float, InitialValue);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Color Channel Row Widget"))
class MESHPAINTINGCORE_API UColorChannelRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UColorChannelRowWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Channel")
	FName ChannelId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Channel")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Channel")
	float Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Channel")
	float MinValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Channel")
	float MaxValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Channel", meta = (ClampMin = "0", ClampMax = "6"))
	int32 DecimalPlaces;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Channel")
	FLinearColor GradientStartColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Channel")
	FLinearColor GradientEndColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Channel")
	EColorBarGradientMode GradientMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Channel|Layout", meta = (ClampMin = "1.0"))
	float HorizontalLabelWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Channel|Layout", meta = (ClampMin = "1.0"))
	float HorizontalSliderLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Channel|Layout", meta = (ClampMin = "1.0"))
	float HorizontalSliderHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Channel|Layout", meta = (ClampMin = "0.0"))
	float HorizontalTrackThickness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Channel|Layout", meta = (ClampMin = "1.0"))
	float HorizontalSpinBoxWidth;

	UPROPERTY(BlueprintAssignable, Category = "Color Channel|Events")
	FOnColorChannelValueChanged OnValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "Color Channel|Events")
	FOnColorChannelInteractionStarted OnInteractionStarted;

	UPROPERTY(BlueprintAssignable, Category = "Color Channel|Events")
	FOnColorChannelValueCommitted OnValueCommitted;

	UFUNCTION(BlueprintCallable, Category = "Color Channel")
	void SetValue(float NewValue, bool bBroadcast = false);

	UFUNCTION(BlueprintCallable, Category = "Color Channel")
	void SetRange(float NewMinValue, float NewMaxValue);

	UFUNCTION(BlueprintCallable, Category = "Color Channel")
	void SetLabel(FText NewLabel);

	UFUNCTION(BlueprintCallable, Category = "Color Channel")
	void SetDecimalPlaces(int32 NewDecimalPlaces);

	UFUNCTION(BlueprintCallable, Category = "Color Channel")
	void SetGradient(FLinearColor NewStartColor, FLinearColor NewEndColor, EColorBarGradientMode NewGradientMode);

	UFUNCTION(BlueprintCallable, Category = "Color Channel")
	void SetHorizontalLayout(
		float NewLabelWidth, float NewSliderLength, float NewSliderHeight,
		float NewTrackThickness, float NewSpinBoxWidth);

	UFUNCTION(BlueprintPure, Category = "Color Channel")
	float GetValue() const { return Value; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;
	virtual void NativePreConstruct() override;

private:
	void SynchronizeSlateWidget();
	float ClampToRange(float InValue) const;
	float GetDelta() const;
	float GetMinValue() const { return MinValue; }
	float GetMaxValue() const { return MaxValue; }
	FText GetSliderLabel() const { return Label; }
	TArray<FLinearColor> GetGradientColors() const;
	bool HasAlphaBackground() const;
	bool SupportsDynamicSliderMaxValue() const;

	void HandleSliderValueChanged(float NewValue);
	void HandleSliderInteractionStarted();
	void HandleSliderValueCommitted(float NewValue);

	TSharedPtr<SRuntimeMeshPaintColorSlider> MyColorSlider;

	UPROPERTY(Transient)
	TObjectPtr<UNativeWidgetHost> NativeSliderHost;

	bool bIsSynchronizing;
};
