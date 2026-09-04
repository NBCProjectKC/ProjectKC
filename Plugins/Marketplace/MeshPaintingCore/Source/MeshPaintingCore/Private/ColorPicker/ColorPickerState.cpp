// Copyright Shared Orbit 2026. All Rights Reserved.
#include "ColorPicker/ColorPickerState.h"

namespace
{
	FLinearColor ClampLinearColor(FLinearColor Color)
	{
		Color.R = FMath::Clamp(Color.R, 0.0f, 1.0f);
		Color.G = FMath::Clamp(Color.G, 0.0f, 1.0f);
		Color.B = FMath::Clamp(Color.B, 0.0f, 1.0f);
		Color.A = 1.0f;
		return Color;
	}
}

void UColorPickerState::SetCurrentColor(FLinearColor NewLinearColor, bool bCapturePrevious, bool bBroadcast)
{
	NewLinearColor = ClampLinearColor(NewLinearColor);
	if (CurrentLinearColor.Equals(NewLinearColor, KINDA_SMALL_NUMBER)) return;

	if (bCapturePrevious) PreviousLinearColor = CurrentLinearColor;

	CurrentLinearColor = NewLinearColor;
	if (bBroadcast) OnColorChanged.Broadcast(CurrentLinearColor);

	if (bEraserActive) bEraserActive = false;

	BroadcastSettingsIfNeeded(bBroadcast);
}

void UColorPickerState::RestorePreviousColor(bool bBroadcast)
{
	SetCurrentColor(PreviousLinearColor, false, bBroadcast);
}

void UColorPickerState::SetBrushSize(float NewBrushSize, bool bBroadcast)
{
	const float ClampedBrushSize = FMath::Max(0.001f, NewBrushSize);
	if (FMath::IsNearlyEqual(BrushSize, ClampedBrushSize)) return;

	BrushSize = ClampedBrushSize;
	BroadcastSettingsIfNeeded(bBroadcast);
}

void UColorPickerState::SetMetallic(float NewMetallic, bool bBroadcast)
{
	const float ClampedMetallic = FMath::Clamp(NewMetallic, 0.0f, 1.0f);
	if (FMath::IsNearlyEqual(Metallic, ClampedMetallic)) return;

	Metallic = ClampedMetallic;
	BroadcastSettingsIfNeeded(bBroadcast);
}

void UColorPickerState::SetRoughness(float NewRoughness, bool bBroadcast)
{
	const float ClampedRoughness = FMath::Clamp(NewRoughness, 0.0f, 1.0f);
	if (FMath::IsNearlyEqual(Roughness, ClampedRoughness)) return;

	Roughness = ClampedRoughness;
	BroadcastSettingsIfNeeded(bBroadcast);
}

void UColorPickerState::SetSRGBPreview(bool bNewSRGBPreview)
{
	bSRGBPreview = bNewSRGBPreview;
}

void UColorPickerState::SetEraserActive(bool bNewEraserActive, bool bBroadcast)
{
	if (bEraserActive == bNewEraserActive) return;

	bEraserActive = bNewEraserActive;
	BroadcastSettingsIfNeeded(bBroadcast);
}

FMeshPaintBrushMaterialSettings UColorPickerState::GetBrushMaterialSettings() const
{
	FMeshPaintBrushMaterialSettings Settings;
	Settings.Color = CurrentLinearColor;
	Settings.BrushSize = BrushSize;
	Settings.Metallic = Metallic;
	Settings.Roughness = Roughness;
	Settings.bErase = bEraserActive;
	Settings.Clamp();
	return Settings;
}

void UColorPickerState::BroadcastSettingsIfNeeded(bool bBroadcast)
{
	if (bBroadcast) OnBrushSettingsChanged.Broadcast(GetBrushMaterialSettings());
}
