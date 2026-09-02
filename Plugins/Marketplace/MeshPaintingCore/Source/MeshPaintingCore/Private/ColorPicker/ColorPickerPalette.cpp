// Copyright Shared Orbit 2026. All Rights Reserved.
#include "ColorPicker/ColorPickerPalette.h"

#include "Misc/ConfigCacheIni.h"

namespace
{
	constexpr int32 PaletteStorageVersion = 1;

	FLinearColor NormalizePaletteColor(FLinearColor Color)
	{
		Color.R = FMath::Clamp(Color.R, 0.0f, 1.0f);
		Color.G = FMath::Clamp(Color.G, 0.0f, 1.0f);
		Color.B = FMath::Clamp(Color.B, 0.0f, 1.0f);
		Color.A = 1.0f;
		return Color;
	}

	void RemoveMatchingColor(TArray<FLinearColor>& Colors, const FLinearColor& Color)
	{
		for (int32 Index = Colors.Num() - 1; Index >= 0; --Index)
		{
			if (Colors[Index].Equals(Color, 0.003f)) Colors.RemoveAt(Index, 1, EAllowShrinking::No);
		}
	}

	bool IsMatchingPaletteColor(const FLinearColor& A, const FLinearColor& B)
	{
		return A.Equals(B, 0.003f);
	}

	FString SanitizeConfigToken(const FString& Token)
	{
		FString Result = Token.TrimStartAndEnd();
		if (Result.IsEmpty()) Result = TEXT("Default");

		for (int32 Index = 0; Index < Result.Len(); ++Index)
		{
			const TCHAR Character = Result[Index];
			if (Character == TEXT('[') || Character == TEXT(']') || Character == TEXT('\r') || Character == TEXT('\n'))
			{
				Result[Index] = TEXT('_');
			}
		}

		return Result;
	}

	FString GetPaletteConfigSection(const UColorPickerPaletteStorage* Storage)
	{
		const FString SlotName = Storage ? SanitizeConfigToken(Storage->SaveSlotName) : TEXT("Default");
		const int32 UserIndex = Storage ? Storage->UserIndex : 0;
		return FString::Printf(TEXT("MeshPaintingCore.ColorPickerPalette.%s.User%d"), *SlotName, UserIndex);
	}

	FString PaletteColorToConfigString(const FLinearColor& Color)
	{
		return FString::Printf(TEXT("%.9g|%.9g|%.9g|%.9g"), Color.R, Color.G, Color.B, Color.A);
	}

	bool PaletteConfigStringToColor(const FString& Value, FLinearColor& OutColor)
	{
		TArray<FString> Parts;
		Value.ParseIntoArray(Parts, TEXT("|"), false);
		if (Parts.Num() != 4) return false;

		OutColor.R = FCString::Atof(*Parts[0]);
		OutColor.G = FCString::Atof(*Parts[1]);
		OutColor.B = FCString::Atof(*Parts[2]);
		OutColor.A = FCString::Atof(*Parts[3]);
		OutColor = NormalizePaletteColor(OutColor);
		return true;
	}
}

void UColorPickerPaletteStorage::Initialize(UColorPickerPaletteDataAsset* InPaletteDataAsset, const FString& InSaveSlotName)
{
	PaletteDataAsset = InPaletteDataAsset;
	if (!InSaveSlotName.IsEmpty()) SaveSlotName = InSaveSlotName;

	LoadCustomColors();
	BroadcastPalette();
}

void UColorPickerPaletteStorage::LoadCustomColors()
{
	MaxColors = FMath::Max(1, MaxColors);
	CustomColors.Reset();

	if (GConfig)
	{
		const FString Section = GetPaletteConfigSection(this);
		int32 StoredColorCount = 0;
		GConfig->GetInt(*Section, TEXT("ColorCount"), StoredColorCount, GGameUserSettingsIni);
		StoredColorCount = FMath::Max(0, StoredColorCount);

		for (int32 ColorIndex = 0; ColorIndex < StoredColorCount; ++ColorIndex)
		{
			FString StoredColor;
			const FString Key = FString::Printf(TEXT("Color%d"), ColorIndex);
			if (!GConfig->GetString(*Section, *Key, StoredColor, GGameUserSettingsIni)) continue;

			FLinearColor ParsedColor;
			if (PaletteConfigStringToColor(StoredColor, ParsedColor))
			{
				RemoveMatchingColor(CustomColors, ParsedColor);
				CustomColors.Add(ParsedColor);
			}
		}
	}

	if (CustomColors.Num() == 0 && bUseFallbackColorsWhenEmpty) CustomColors = GetInitialColors();

	TrimToMaxColors();
}

void UColorPickerPaletteStorage::SaveCustomColors() const
{
	if (!GConfig) return;

	const FString Section = GetPaletteConfigSection(this);
	GConfig->SetInt(*Section, TEXT("Version"), PaletteStorageVersion, GGameUserSettingsIni);
	GConfig->SetInt(*Section, TEXT("MaxColors"), MaxColors, GGameUserSettingsIni);
	GConfig->SetInt(*Section, TEXT("ColorCount"), CustomColors.Num(), GGameUserSettingsIni);

	for (int32 ColorIndex = 0; ColorIndex < CustomColors.Num(); ++ColorIndex)
	{
		const FString Key = FString::Printf(TEXT("Color%d"), ColorIndex);
		GConfig->SetString(*Section, *Key, *PaletteColorToConfigString(NormalizePaletteColor(CustomColors[ColorIndex])), GGameUserSettingsIni);
	}

	GConfig->Flush(false, GGameUserSettingsIni);
}

void UColorPickerPaletteStorage::AddCustomColor(FLinearColor Color)
{
	Color = NormalizePaletteColor(Color);
	RemoveMatchingColor(CustomColors, Color);

	CustomColors.Insert(Color, 0);
	TrimToMaxColors();
	SaveCustomColors();
	BroadcastPalette();
}

void UColorPickerPaletteStorage::AddRecentColor(FLinearColor Color)
{
	Color = NormalizePaletteColor(Color);
	MaxColors = FMath::Max(1, MaxColors);
	if (CustomColors.Num() > 0 && IsMatchingPaletteColor(CustomColors[0], Color))
	{
		return;
	}

	RemoveMatchingColor(CustomColors, Color);

	CustomColors.Insert(Color, 0);
	TrimToMaxColors();
	SaveCustomColors();
	BroadcastPalette();
}

void UColorPickerPaletteStorage::RemoveCustomColorAt(int32 Index)
{
	if (!CustomColors.IsValidIndex(Index)) return;

	CustomColors.RemoveAt(Index);
	SaveCustomColors();
	BroadcastPalette();
}

void UColorPickerPaletteStorage::RestoreDefaultPalette()
{
	CustomColors = GetInitialColors();
	TrimToMaxColors();
	SaveCustomColors();
	BroadcastPalette();
}

TArray<FLinearColor> UColorPickerPaletteStorage::GetCombinedColors() const
{
	TArray<FLinearColor> Colors = CustomColors;
	if (Colors.Num() == 0 && bUseFallbackColorsWhenEmpty) Colors = GetInitialColors();
	if (Colors.Num() > MaxColors) Colors.SetNum(MaxColors);
	return Colors;
}

TArray<FLinearColor> UColorPickerPaletteStorage::GetFallbackDefaultColors()
{
	return
	{
		FLinearColor(0.0f, 0.0f, 0.0f, 1.0f),
		FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)
	};
}

void UColorPickerPaletteStorage::TrimToMaxColors()
{
	MaxColors = FMath::Max(1, MaxColors);
	while (CustomColors.Num() > MaxColors)
	{
		CustomColors.RemoveAt(CustomColors.Num() - 1, 1, EAllowShrinking::No);
	}
}

TArray<FLinearColor> UColorPickerPaletteStorage::GetInitialColors() const
{
	return PaletteDataAsset && PaletteDataAsset->DefaultColors.Num() > 0
		? PaletteDataAsset->DefaultColors
		: GetFallbackDefaultColors();
}

void UColorPickerPaletteStorage::BroadcastPalette()
{
	OnPaletteChanged.Broadcast(GetCombinedColors());
}
