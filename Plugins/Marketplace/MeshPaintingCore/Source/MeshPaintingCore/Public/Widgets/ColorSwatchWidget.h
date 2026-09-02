// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/ColorPreviewWidget.h"
#include "ColorSwatchWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorSwatchSelected, FLinearColor, Color, int32, SwatchIndex);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Color Swatch Widget"))
class MESHPAINTINGCORE_API UColorSwatchWidget : public UColorPreviewWidget
{
	GENERATED_BODY()

public:
	UColorSwatchWidget();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Swatch")
	int32 SwatchIndex;

	UPROPERTY(BlueprintAssignable, Category = "Color Swatch|Events")
	FOnColorSwatchSelected OnSwatchSelected;

	UFUNCTION(BlueprintCallable, Category = "Color Swatch")
	void SetSwatchIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category = "Color Swatch")
	void SetSwatchSize(float NewSize);

	virtual void HandleSlateClicked() override;
};
