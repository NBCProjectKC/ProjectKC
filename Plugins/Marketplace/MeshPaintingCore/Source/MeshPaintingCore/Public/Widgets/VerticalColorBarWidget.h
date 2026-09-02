// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "VerticalColorBarWidget.generated.h"

class SRuntimeMeshPaintColorSlider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVerticalColorBarValueChanged, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVerticalColorBarInteractionStarted);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Vertical Color Bar Widget"))
class MESHPAINTINGCORE_API UVerticalColorBarWidget : public UWidget
{
	GENERATED_BODY()

public:
	UVerticalColorBarWidget();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Bar", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Bar")
	FLinearColor TopColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Bar")
	FLinearColor BottomColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Bar")
	bool bShowCheckerboard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector2D DesiredBarSize;

	UPROPERTY(BlueprintAssignable, Category = "Color Bar|Events")
	FOnVerticalColorBarValueChanged OnValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "Color Bar|Events")
	FOnVerticalColorBarInteractionStarted OnInteractionStarted;

	UPROPERTY(BlueprintAssignable, Category = "Color Bar|Events")
	FOnVerticalColorBarValueChanged OnValueCommitted;

	UFUNCTION(BlueprintCallable, Category = "Color Bar")
	void SetValue(float NewValue, bool bBroadcast = false);

	UFUNCTION(BlueprintPure, Category = "Color Bar")
	float GetValue() const { return Value; }

	UFUNCTION(BlueprintCallable, Category = "Color Bar")
	void SetGradientColors(FLinearColor NewBottomColor, FLinearColor NewTopColor);

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
