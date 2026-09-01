// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "MeshPaintingCoreTypes.h"
#include "Engine/EngineTypes.h"

class UMeshComponent;

namespace RuntimeMeshPaint
{
	bool FindPaintHitUV(
		const FHitResult& HitResult, int32 UVChannel, float MaxSkeletalMeshUVFallbackDistance,
		FVector2D& OutUV, int32* OutResolvedFaceIndex = nullptr, int32* OutResolvedTriangleArrayIndex = nullptr);
	bool ResolvePaintBrushRadii(
		const FHitResult& HitResult, int32 UVChannel, float MaxSkeletalMeshUVFallbackDistance,
		const FVector2D& HitUV, float BrushSize, float& OutUVRadius, float& OutWorldRadius);
	bool ResolvePaintHitSurfaceData(
		const FHitResult& HitResult, int32 UVChannel, const FVector2D& HitUV,
		FVector& OutWorldPosition, FVector& OutWorldNormal);
}
