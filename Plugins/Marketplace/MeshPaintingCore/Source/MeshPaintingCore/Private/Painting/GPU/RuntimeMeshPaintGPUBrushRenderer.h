// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "MeshPaintingCoreTypes.h"

class UTextureRenderTarget2D;
class UMeshComponent;

class FRuntimeMeshPaintGPUBrushRenderer
{
public:
	static bool PrecacheProjectedMeshResource(
		UMeshComponent* MeshComponent,
		int32 UVChannel,
		float UVIslandConnectionTolerance);
	static bool DrawProjectedBrush(
		UMeshComponent* MeshComponent,
		UTextureRenderTarget2D* ColorRenderTarget,
		UTextureRenderTarget2D* MaterialSettingsRenderTarget,
		int32 UVChannel,
		float UVIslandConnectionTolerance,
		const FVector& BrushRayStart,
		const FVector& BrushRayEnd,
		const FVector& BrushWorldCenter,
		const FVector& BrushWorldNormal,
		float BrushWorldRadius,
		const FRuntimeMeshPaintScreenProjectionData& ProjectionData,
		const FMeshPaintBrushMaterialSettings& BrushSettings);

	static bool DrawProjectedBrushPreviewMask(
		UMeshComponent* MeshComponent,
		UTextureRenderTarget2D* PreviewMaskRenderTarget,
		int32 UVChannel,
		float UVIslandConnectionTolerance,
		const FVector& BrushRayStart,
		const FVector& BrushRayEnd,
		const FVector& BrushWorldCenter,
		const FVector& BrushWorldNormal,
		float BrushWorldRadius,
		const FRuntimeMeshPaintScreenProjectionData& ProjectionData,
		const FLinearColor& PreviewColor,
		float PreviewLineThickness);
};
