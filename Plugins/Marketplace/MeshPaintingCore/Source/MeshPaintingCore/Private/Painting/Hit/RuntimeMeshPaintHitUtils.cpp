// Copyright Shared Orbit 2026. All Rights Reserved.
#include "RuntimeMeshPaintHitUtils.h"

#include "RuntimeMeshPaintHitUtilsInternal.h"
#include "../../Core/MeshPaintingCoreStats.h"

static constexpr float RuntimeMeshPaintBrushSizeToWorldRadiusScale = 100.0f;

static float MakeRuntimeMeshPaintBrushWorldRadius(float BrushSize)
{
	return FMath::Max(BrushSize, KINDA_SMALL_NUMBER) * RuntimeMeshPaintBrushSizeToWorldRadiusScale;
}

namespace RuntimeMeshPaint
{
	bool FindPaintHitUV(
		const FHitResult& HitResult, int32 UVChannel, float MaxSkeletalMeshUVFallbackDistance,
		FVector2D& OutUV, int32* OutResolvedFaceIndex, int32* OutResolvedTriangleArrayIndex)
	{
		SCOPE_CYCLE_COUNTER(STAT_MeshPaintingCore_FindPaintHitUV);

		if (OutResolvedFaceIndex) *OutResolvedFaceIndex = INDEX_NONE;
		if (OutResolvedTriangleArrayIndex) *OutResolvedTriangleArrayIndex = INDEX_NONE;

		(void)MaxSkeletalMeshUVFallbackDistance;
		(void)OutResolvedTriangleArrayIndex;

		if (FRuntimeMeshPaintStaticMesh::FindStaticMeshFaceUV(HitResult, UVChannel, OutUV, OutResolvedFaceIndex))
		{
			return true;
		}

		return false;
	}

	bool ResolvePaintBrushRadii(
		const FHitResult& HitResult, int32 UVChannel, float MaxSkeletalMeshUVFallbackDistance,
		const FVector2D& HitUV, float BrushSize, float& OutUVRadius, float& OutWorldRadius)
	{
		(void)MaxSkeletalMeshUVFallbackDistance;

		OutUVRadius = FMath::Max(BrushSize, KINDA_SMALL_NUMBER);
		OutWorldRadius = MakeRuntimeMeshPaintBrushWorldRadius(BrushSize);

		float HitLocalUnitsPerUV = 0.0f;
		float HitWorldUnitsPerUV = 0.0f;
		float AverageLocalUnitsPerUV = 0.0f;
		if (!FRuntimeMeshPaintSurface::EstimatePaintHitTriangleUnitsPerUV(
			HitResult,
			UVChannel,
			HitUV,
			HitLocalUnitsPerUV,
			HitWorldUnitsPerUV,
			AverageLocalUnitsPerUV))
		{
			return false;
		}

		if (HitWorldUnitsPerUV <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		OutUVRadius = FMath::Max(OutWorldRadius / HitWorldUnitsPerUV, KINDA_SMALL_NUMBER);
		return OutWorldRadius > KINDA_SMALL_NUMBER;
	}

	bool ResolvePaintHitSurfaceData(
		const FHitResult& HitResult, int32 UVChannel, const FVector2D& HitUV,
		FVector& OutWorldPosition, FVector& OutWorldNormal)
	{
		OutWorldPosition = FVector::ZeroVector;
		OutWorldNormal = FVector::ZeroVector;

		return FRuntimeMeshPaintSurface::ResolveStaticPaintHitSurfaceData(HitResult, UVChannel, HitUV, OutWorldPosition, OutWorldNormal) ||
			FRuntimeMeshPaintSurface::ResolveSkeletalPaintHitSurfaceData(HitResult, UVChannel, HitUV, OutWorldPosition, OutWorldNormal);
	}

}
