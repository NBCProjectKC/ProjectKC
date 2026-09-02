// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "MeshPaintingCoreTypes.h"

class APlayerController;

namespace ColorPickerEyedropper
{
	bool SampleUnderCursor(
		APlayerController* PlayerController, ECollisionChannel TraceChannel, bool bTraceComplex,
		UObject* FallbackPaintTarget, FRuntimeMeshPaintSampleResult& OutSampleResult,
		ERuntimeMeshPaintColorSampleMode SampleMode = ERuntimeMeshPaintColorSampleMode::MeshUnlitColor);
}
