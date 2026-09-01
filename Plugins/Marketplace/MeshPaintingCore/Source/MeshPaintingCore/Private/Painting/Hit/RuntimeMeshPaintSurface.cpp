// Copyright Shared Orbit 2026. All Rights Reserved.
#include "RuntimeMeshPaintHitUtilsInternal.h"

static bool FindCachedPaintTriangleIndex(
	UMeshComponent* MeshComponent,
	int32 UVChannel,
	const FVector2D& HitUV,
	int32 FaceIndex,
	TSharedPtr<FPaintUVCache>& OutCache,
	int32& OutTriangleIndex)
{
	OutTriangleIndex = INDEX_NONE;
	OutCache = FRuntimeMeshPaintUVCache::FindOrBuildPaintUVCache(MeshComponent, UVChannel, 0.0f);
	if (!OutCache.IsValid()) return false;

	int32 TriangleArrayIndex = INDEX_NONE;
	if (!FRuntimeMeshPaintUVCache::FindSeedTriangleArrayIndex(*OutCache, FaceIndex, HitUV, TriangleArrayIndex) ||
		!OutCache->Triangles.IsValidIndex(TriangleArrayIndex))
	{
		return false;
	}

	OutTriangleIndex = OutCache->Triangles[TriangleArrayIndex].TriangleIndex;
	return OutTriangleIndex != INDEX_NONE;
}

bool FRuntimeMeshPaintSurface::EstimateStaticHitTriangleUnitsPerUV(
	const FHitResult& HitResult,
	int32 UVChannel,
	const FVector2D& HitUV,
	float& OutHitLocalUnitsPerUV,
	float& OutHitWorldUnitsPerUV,
	float& OutAverageLocalUnitsPerUV)
{
	OutHitLocalUnitsPerUV = 0.0f;
	OutHitWorldUnitsPerUV = 0.0f;
	OutAverageLocalUnitsPerUV = 0.0f;

	const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(HitResult.GetComponent());
	const UStaticMesh* StaticMesh = StaticMeshComponent ? StaticMeshComponent->GetStaticMesh() : nullptr;
	const FStaticMeshRenderData* RenderData = StaticMesh ? StaticMesh->GetRenderData() : nullptr;
	if (!RenderData || RenderData->LODResources.Num() == 0 || UVChannel < 0) return false;

	TSharedPtr<FPaintUVCache> Cache;
	int32 TriangleIndex = HitResult.FaceIndex;
	if (TriangleIndex == INDEX_NONE &&
		!FindCachedPaintTriangleIndex(Cast<UMeshComponent>(HitResult.GetComponent()), UVChannel, HitUV, HitResult.FaceIndex, Cache, TriangleIndex))
	{
		return false;
	}

	if (!Cache.IsValid())
	{
		TSharedPtr<FPaintUVCache> CachedTriangleCache;
		int32 CachedTriangleIndex = INDEX_NONE;
		if (FindCachedPaintTriangleIndex(
			Cast<UMeshComponent>(HitResult.GetComponent()),
			UVChannel,
			HitUV,
			TriangleIndex,
			CachedTriangleCache,
			CachedTriangleIndex))
		{
			Cache = CachedTriangleCache;
			TriangleIndex = CachedTriangleIndex;
		}
	}

	const FStaticMeshLODResources& LODResources = RenderData->LODResources[0];

	FPaintTriangleSurfaceData LocalTriangle;
	if (!FRuntimeMeshPaintStaticMesh::GetStaticTriangleData(LODResources, FTransform::Identity, TriangleIndex, UVChannel, LocalTriangle))
	{
		return false;
	}

	FPaintTriangleSurfaceData WorldTriangle;
	if (!FRuntimeMeshPaintStaticMesh::GetStaticTriangleData(LODResources, StaticMeshComponent->GetComponentTransform(), TriangleIndex, UVChannel, WorldTriangle))
	{
		return false;
	}

	if (!FRuntimeMeshPaintGeometry::EstimateTriangleUnitsPerUV(LocalTriangle, OutHitLocalUnitsPerUV) ||
		!FRuntimeMeshPaintGeometry::EstimateTriangleUnitsPerUV(WorldTriangle, OutHitWorldUnitsPerUV))
	{
		return false;
	}

	OutAverageLocalUnitsPerUV = Cache.IsValid() ? Cache->AverageLocalUnitsPerUV : OutHitLocalUnitsPerUV;
	return OutAverageLocalUnitsPerUV > KINDA_SMALL_NUMBER;
}

bool FRuntimeMeshPaintSurface::BuildSkeletalTriangleDataLazy(
	USkeletalMeshComponent* SkeletalMeshComponent,
	const FSkeletalMeshLODRenderData& LODData,
	const FSkinWeightVertexBuffer& SkinWeightBuffer,
	const FRawStaticIndexBuffer16or32Interface& IndexBuffer,
	int32 TriangleIndex,
	int32 UVChannel,
	TArray<FMatrix44f>& CachedRefToLocals,
	FPaintTriangleSurfaceData& OutLocalTriangle,
	FPaintTriangleSurfaceData& OutWorldTriangle)
{
	FIntVector VertexIndices = FIntVector::ZeroValue;
	if (!FRuntimeMeshPaintSkeletalMesh::GetSkeletalMeshTriangleVertexIndices(LODData, IndexBuffer, TriangleIndex, VertexIndices)) return false;

	FVector Position0 = FVector::ZeroVector;
	FVector Position1 = FVector::ZeroVector;
	FVector Position2 = FVector::ZeroVector;
	if (!FRuntimeMeshPaintSkeletalMesh::SkinSkeletalTrianglePositions(
		SkeletalMeshComponent,
		LODData,
		SkinWeightBuffer,
		VertexIndices,
		CachedRefToLocals,
		Position0,
		Position1,
		Position2))
	{
		return false;
	}

	const FVector2f UV0 = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndices.X, UVChannel);
	const FVector2f UV1 = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndices.Y, UVChannel);
	const FVector2f UV2 = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndices.Z, UVChannel);

	OutLocalTriangle.WorldPosition0 = Position0;
	OutLocalTriangle.WorldPosition1 = Position1;
	OutLocalTriangle.WorldPosition2 = Position2;
	OutLocalTriangle.UV0 = FVector2D(UV0.X, UV0.Y);
	OutLocalTriangle.UV1 = FVector2D(UV1.X, UV1.Y);
	OutLocalTriangle.UV2 = FVector2D(UV2.X, UV2.Y);

	const FTransform& ComponentTransform = SkeletalMeshComponent->GetComponentTransform();
	OutWorldTriangle = OutLocalTriangle;
	OutWorldTriangle.WorldPosition0 = ComponentTransform.TransformPosition(Position0);
	OutWorldTriangle.WorldPosition1 = ComponentTransform.TransformPosition(Position1);
	OutWorldTriangle.WorldPosition2 = ComponentTransform.TransformPosition(Position2);
	return true;
}

bool FRuntimeMeshPaintSurface::EstimateSkeletalHitTriangleUnitsPerUV(
	const FHitResult& HitResult,
	int32 UVChannel,
	const FVector2D& HitUV,
	float& OutHitLocalUnitsPerUV,
	float& OutHitWorldUnitsPerUV,
	float& OutAverageLocalUnitsPerUV)
{
	OutHitLocalUnitsPerUV = 0.0f;
	OutHitWorldUnitsPerUV = 0.0f;
	OutAverageLocalUnitsPerUV = 0.0f;

	USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(HitResult.GetComponent());
	FSkeletalMeshRenderData* RenderData = SkeletalMeshComponent ? SkeletalMeshComponent->GetSkeletalMeshRenderData() : nullptr;
	if (!RenderData || RenderData->LODRenderData.Num() == 0 || UVChannel < 0) return false;

	const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[0];
	const FRawStaticIndexBuffer16or32Interface* IndexBuffer = LODData.MultiSizeIndexContainer.IsIndexBufferValid()
		? LODData.MultiSizeIndexContainer.GetIndexBuffer()
		: nullptr;
	const FSkinWeightVertexBuffer* SkinWeightBuffer = LODData.GetSkinWeightVertexBuffer();
	if (!IndexBuffer || !SkinWeightBuffer || IndexBuffer->Num() < 3 ||
		static_cast<uint32>(UVChannel) >= LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords())
	{
		return false;
	}

	TSharedPtr<FPaintUVCache> Cache;
	int32 TriangleIndex = HitResult.FaceIndex;
	if (TriangleIndex == INDEX_NONE &&
		!FindCachedPaintTriangleIndex(SkeletalMeshComponent, UVChannel, HitUV, HitResult.FaceIndex, Cache, TriangleIndex))
	{
		return false;
	}

	if (!Cache.IsValid())
	{
		TSharedPtr<FPaintUVCache> CachedTriangleCache;
		int32 CachedTriangleIndex = INDEX_NONE;
		if (FindCachedPaintTriangleIndex(
			SkeletalMeshComponent,
			UVChannel,
			HitUV,
			TriangleIndex,
			CachedTriangleCache,
			CachedTriangleIndex))
		{
			Cache = CachedTriangleCache;
			TriangleIndex = CachedTriangleIndex;
		}
	}

	TArray<FMatrix44f> CachedRefToLocals;
	SkeletalMeshComponent->CacheRefToLocalMatrices(CachedRefToLocals);
	if (CachedRefToLocals.Num() == 0) return false;

	FPaintTriangleSurfaceData LocalTriangle;
	FPaintTriangleSurfaceData WorldTriangle;
	if (!FRuntimeMeshPaintSurface::BuildSkeletalTriangleDataLazy(
		SkeletalMeshComponent,
		LODData,
		*SkinWeightBuffer,
		*IndexBuffer,
		TriangleIndex,
		UVChannel,
		CachedRefToLocals,
		LocalTriangle,
		WorldTriangle))
	{
		return false;
	}

	if (!FRuntimeMeshPaintGeometry::EstimateTriangleUnitsPerUV(LocalTriangle, OutHitLocalUnitsPerUV) ||
		!FRuntimeMeshPaintGeometry::EstimateTriangleUnitsPerUV(WorldTriangle, OutHitWorldUnitsPerUV))
	{
		return false;
	}

	OutAverageLocalUnitsPerUV = Cache.IsValid() ? Cache->AverageLocalUnitsPerUV : OutHitLocalUnitsPerUV;
	return OutAverageLocalUnitsPerUV > KINDA_SMALL_NUMBER;
}

bool FRuntimeMeshPaintSurface::EstimatePaintHitTriangleUnitsPerUV(
	const FHitResult& HitResult,
	int32 UVChannel,
	const FVector2D& HitUV,
	float& OutHitLocalUnitsPerUV,
	float& OutHitWorldUnitsPerUV,
	float& OutAverageLocalUnitsPerUV)
{
	return FRuntimeMeshPaintSurface::EstimateStaticHitTriangleUnitsPerUV(
			HitResult,
			UVChannel,
			HitUV,
			OutHitLocalUnitsPerUV,
			OutHitWorldUnitsPerUV,
			OutAverageLocalUnitsPerUV) ||
		FRuntimeMeshPaintSurface::EstimateSkeletalHitTriangleUnitsPerUV(
			HitResult,
			UVChannel,
			HitUV,
			OutHitLocalUnitsPerUV,
			OutHitWorldUnitsPerUV,
			OutAverageLocalUnitsPerUV);
}

static FVector OrientPaintSurfaceNormalForPreview(
	const FHitResult& HitResult,
	const FVector& SurfaceWorldPosition,
	const FVector& SurfaceWorldNormal)
{
	FVector OrientedNormal = SurfaceWorldNormal.GetSafeNormal(
		SMALL_NUMBER,
		HitResult.ImpactNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector));

	const FVector ToTraceStart = (FVector(HitResult.TraceStart) - SurfaceWorldPosition).GetSafeNormal();
	if (!ToTraceStart.IsNearlyZero())
	{
		if (FVector::DotProduct(OrientedNormal, ToTraceStart) < 0.0)
		{
			OrientedNormal *= -1.0;
		}
		return OrientedNormal;
	}

	const FVector HitNormal = HitResult.ImpactNormal.GetSafeNormal();
	if (!HitNormal.IsNearlyZero() && FVector::DotProduct(OrientedNormal, HitNormal) < 0.0)
	{
		OrientedNormal *= -1.0;
	}

	return OrientedNormal;
}

bool FRuntimeMeshPaintSurface::ResolveStaticPaintHitSurfaceData(
	const FHitResult& HitResult,
	int32 UVChannel,
	const FVector2D& HitUV,
	FVector& OutWorldPosition,
	FVector& OutWorldNormal)
{
	const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(HitResult.GetComponent());
	const UStaticMesh* StaticMesh = StaticMeshComponent ? StaticMeshComponent->GetStaticMesh() : nullptr;
	const FStaticMeshRenderData* RenderData = StaticMesh ? StaticMesh->GetRenderData() : nullptr;
	if (!RenderData || RenderData->LODResources.Num() == 0 || HitResult.FaceIndex == INDEX_NONE) return false;

	FPaintTriangleSurfaceData Triangle;
	if (!FRuntimeMeshPaintStaticMesh::GetStaticTriangleData(RenderData->LODResources[0], StaticMeshComponent->GetComponentTransform(), HitResult.FaceIndex, UVChannel, Triangle))
	{
		return false;
	}

	FVector Barycentric = FVector::ZeroVector;
	if (!FRuntimeMeshPaintGeometry::ComputeUVBarycentric(HitUV, Triangle.UV0, Triangle.UV1, Triangle.UV2, Barycentric)) return false;

	OutWorldPosition = FRuntimeMeshPaintGeometry::InterpolateTrianglePosition(Triangle, Barycentric);
	OutWorldNormal = OrientPaintSurfaceNormalForPreview(HitResult, OutWorldPosition, Triangle.WorldNormal);
	return true;
}

bool FRuntimeMeshPaintSurface::ResolveSkeletalPaintHitSurfaceData(
	const FHitResult& HitResult,
	int32 UVChannel,
	const FVector2D& HitUV,
	FVector& OutWorldPosition,
	FVector& OutWorldNormal)
{
	USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(HitResult.GetComponent());
	FSkeletalMeshRenderData* RenderData = SkeletalMeshComponent ? SkeletalMeshComponent->GetSkeletalMeshRenderData() : nullptr;
	if (!RenderData || RenderData->LODRenderData.Num() == 0 || HitResult.FaceIndex == INDEX_NONE || UVChannel < 0)
	{
		return false;
	}

	const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[0];
	const FRawStaticIndexBuffer16or32Interface* IndexBuffer = LODData.MultiSizeIndexContainer.IsIndexBufferValid()
		? LODData.MultiSizeIndexContainer.GetIndexBuffer()
		: nullptr;
	const FSkinWeightVertexBuffer* SkinWeightBuffer = LODData.GetSkinWeightVertexBuffer();
	if (!IndexBuffer || !SkinWeightBuffer || IndexBuffer->Num() < 3 ||
		static_cast<uint32>(UVChannel) >= LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords())
	{
		return false;
	}

	TArray<FMatrix44f> CachedRefToLocals;
	SkeletalMeshComponent->CacheRefToLocalMatrices(CachedRefToLocals);
	if (CachedRefToLocals.Num() == 0) return false;

	FPaintTriangleSurfaceData LocalTriangle;
	FPaintTriangleSurfaceData WorldTriangle;
	if (!FRuntimeMeshPaintSurface::BuildSkeletalTriangleDataLazy(
		SkeletalMeshComponent,
		LODData,
		*SkinWeightBuffer,
		*IndexBuffer,
		HitResult.FaceIndex,
		UVChannel,
		CachedRefToLocals,
		LocalTriangle,
		WorldTriangle))
	{
		return false;
	}

	FVector Barycentric = FVector::ZeroVector;
	if (!FRuntimeMeshPaintGeometry::ComputeUVBarycentric(HitUV, WorldTriangle.UV0, WorldTriangle.UV1, WorldTriangle.UV2, Barycentric)) return false;

	OutWorldPosition = FRuntimeMeshPaintGeometry::InterpolateTrianglePosition(WorldTriangle, Barycentric);
	const FVector SurfaceNormal = FVector::CrossProduct(
		WorldTriangle.WorldPosition1 - WorldTriangle.WorldPosition0,
		WorldTriangle.WorldPosition2 - WorldTriangle.WorldPosition0).GetSafeNormal(
			SMALL_NUMBER,
			HitResult.ImpactNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector));
	OutWorldNormal = OrientPaintSurfaceNormalForPreview(HitResult, OutWorldPosition, SurfaceNormal);
	return true;
}
