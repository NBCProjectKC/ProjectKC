// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "ColorPickerTypes.generated.h"

UENUM(BlueprintType)
enum class EColorBarGradientMode : uint8
{
	TwoColor UMETA(DisplayName = "Two Color"),
	Hue UMETA(DisplayName = "Hue Rainbow"),
	Alpha UMETA(DisplayName = "Alpha Checker")
};

UENUM(BlueprintType)
enum class EHexColorInputMode : uint8
{
	SRGB UMETA(DisplayName = "Hex sRGB"),
	Linear UMETA(DisplayName = "Hex Linear")
};
