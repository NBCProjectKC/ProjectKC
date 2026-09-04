// Copyright Shared Orbit 2026. All Rights Reserved.
#include "ColorPicker/ColorPickerConversionLibrary.h"

namespace
{
	bool IsHexDigit(TCHAR Character)
	{
		return (Character >= TEXT('0') && Character <= TEXT('9'))
			|| (Character >= TEXT('a') && Character <= TEXT('f'))
			|| (Character >= TEXT('A') && Character <= TEXT('F'));
	}

	uint8 ParseHexByte(const FString& Text, int32 Offset)
	{
		return static_cast<uint8>(FParse::HexNumber(*Text.Mid(Offset, 2)));
	}
}

FLinearColor UColorPickerConversionLibrary::HSVToLinearRGB(float Hue, float Saturation, float Value, float Alpha)
{
	FLinearColor HSVColor(
		NormalizeHue(Hue),
		FMath::Clamp(Saturation, 0.0f, 1.0f),
		FMath::Clamp(Value, 0.0f, 1.0f),
		FMath::Clamp(Alpha, 0.0f, 1.0f));
	FLinearColor RGBColor = HSVColor.HSVToLinearRGB();
	RGBColor.A = HSVColor.A;
	return RGBColor;
}

void UColorPickerConversionLibrary::LinearRGBToHSV(FLinearColor LinearColor, float& Hue, float& Saturation, float& Value, float& Alpha)
{
	LinearColor.R = FMath::Clamp(LinearColor.R, 0.0f, 1.0f);
	LinearColor.G = FMath::Clamp(LinearColor.G, 0.0f, 1.0f);
	LinearColor.B = FMath::Clamp(LinearColor.B, 0.0f, 1.0f);
	LinearColor.A = FMath::Clamp(LinearColor.A, 0.0f, 1.0f);

	const FLinearColor HSV = LinearColor.LinearRGBToHSV();
	Hue = NormalizeHue(FMath::IsFinite(HSV.R) ? HSV.R : 0.0f);
	Saturation = FMath::Clamp(FMath::IsFinite(HSV.G) ? HSV.G : 0.0f, 0.0f, 1.0f);
	Value = FMath::Clamp(FMath::IsFinite(HSV.B) ? HSV.B : 0.0f, 0.0f, 1.0f);
	Alpha = LinearColor.A;
}

FColor UColorPickerConversionLibrary::LinearRGBToSRGB8(FLinearColor LinearColor)
{
	LinearColor.R = FMath::Clamp(LinearColor.R, 0.0f, 1.0f);
	LinearColor.G = FMath::Clamp(LinearColor.G, 0.0f, 1.0f);
	LinearColor.B = FMath::Clamp(LinearColor.B, 0.0f, 1.0f);
	LinearColor.A = FMath::Clamp(LinearColor.A, 0.0f, 1.0f);
	return LinearColor.ToFColor(true);
}

FLinearColor UColorPickerConversionLibrary::SRGB8ToLinearRGB(FColor SRGBColor)
{
	FLinearColor LinearColor = FLinearColor::FromSRGBColor(SRGBColor);
	LinearColor.A = SRGBColor.A / 255.0f;
	return LinearColor;
}

FString UColorPickerConversionLibrary::LinearRGBToHex(FLinearColor LinearColor, bool bIncludeAlpha, bool bPrefixHash)
{
	const FColor SRGBColor = LinearRGBToSRGB8(LinearColor);
	const FString Prefix = bPrefixHash ? TEXT("#") : TEXT("");
	return bIncludeAlpha
		? FString::Printf(TEXT("%s%02X%02X%02X%02X"), *Prefix, SRGBColor.R, SRGBColor.G, SRGBColor.B, SRGBColor.A)
		: FString::Printf(TEXT("%s%02X%02X%02X"), *Prefix, SRGBColor.R, SRGBColor.G, SRGBColor.B);
}

bool UColorPickerConversionLibrary::HexToLinearRGB(const FString& HexText, FLinearColor& OutLinearColor, bool& bOutHasAlpha)
{
	FString Sanitized = HexText.TrimStartAndEnd();
	if (Sanitized.StartsWith(TEXT("#"))) Sanitized.RightChopInline(1);

	if (Sanitized.Len() != 6 && Sanitized.Len() != 8)
	{
		OutLinearColor = FLinearColor::Transparent;
		bOutHasAlpha = false;
		return false;
	}

	for (const TCHAR Character : Sanitized)
	{
		if (!IsHexDigit(Character))
		{
			OutLinearColor = FLinearColor::Transparent;
			bOutHasAlpha = false;
			return false;
		}
	}

	const uint8 R = ParseHexByte(Sanitized, 0);
	const uint8 G = ParseHexByte(Sanitized, 2);
	const uint8 B = ParseHexByte(Sanitized, 4);
	const uint8 A = Sanitized.Len() == 8 ? ParseHexByte(Sanitized, 6) : 255;

	bOutHasAlpha = Sanitized.Len() == 8;
	OutLinearColor = SRGB8ToLinearRGB(FColor(R, G, B, A));
	return true;
}

float UColorPickerConversionLibrary::NormalizeHue(float Hue)
{
	float NormalizedHue = FMath::Fmod(Hue, 360.0f);
	if (NormalizedHue < 0.0f) NormalizedHue += 360.0f;
	return NormalizedHue;
}
