// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ColorPickerConversionLibrary.generated.h"

UCLASS()
class MESHPAINTINGCORE_API UColorPickerConversionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Runtime Mesh Painting|Color")
	static FLinearColor HSVToLinearRGB(float Hue, float Saturation, float Value, float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Runtime Mesh Painting|Color")
	static void LinearRGBToHSV(FLinearColor LinearColor, float& Hue, float& Saturation, float& Value, float& Alpha);

	UFUNCTION(BlueprintPure, Category = "Runtime Mesh Painting|Color")
	static FColor LinearRGBToSRGB8(FLinearColor LinearColor);

	UFUNCTION(BlueprintPure, Category = "Runtime Mesh Painting|Color")
	static FLinearColor SRGB8ToLinearRGB(FColor SRGBColor);

	UFUNCTION(BlueprintPure, Category = "Runtime Mesh Painting|Color")
	static FString LinearRGBToHex(FLinearColor LinearColor, bool bIncludeAlpha = false, bool bPrefixHash = true);

	UFUNCTION(BlueprintPure, Category = "Runtime Mesh Painting|Color")
	static bool HexToLinearRGB(const FString& HexText, FLinearColor& OutLinearColor, bool& bOutHasAlpha);

	static float NormalizeHue(float Hue);
};
