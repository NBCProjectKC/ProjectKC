// Copyright Shared Orbit 2026. All Rights Reserved.
#include "RuntimeMeshPaintHitUtilsInternal.h"

#include "../../Core/MeshPaintingCoreStats.h"

bool FRuntimeMeshPaintSkeletalMesh::CollectSkeletalMeshUVTriangles(
	const USkeletalMeshComponent* SkeletalMeshComponent,
	int32 UVChannel,
	TArray<RuntimeMeshPaint::FPaintUVTriangle>& OutTriangles,
	TArray<FIntVector>* OutTriangleVertexIndices = nullptr,
	TArray<int32>* OutTriangleArraySectionIds = nullptr,
	float* OutAverageLocalUnitsPerUV = nullptr)
{
	SCOPE_CYCLE_COUNTER(STAT_MeshPaintingCore_CollectSkeletalMeshUVTriangles);

	OutTriangles.Reset();
	if (OutTriangleVertexIndices) OutTriangleVertexIndices->Reset();
	if (OutTriangleArraySectionIds) OutTriangleArraySectionIds->Reset();
	if (OutAverageLocalUnitsPerUV) *OutAverageLocalUnitsPerUV = 0.0f;

	const FSkeletalMeshRenderData* RenderData = SkeletalMeshComponent ? SkeletalMeshComponent->GetSkeletalMeshRenderData() : nullptr;
	if (!RenderData || RenderData->LODRenderData.Num() == 0 || UVChannel < 0) return false;

	const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[0];
	const FRawStaticIndexBuffer16or32Interface* IndexBuffer = LODData.MultiSizeIndexContainer.IsIndexBufferValid()
		? LODData.MultiSizeIndexContainer.GetIndexBuffer()
		: nullptr;
	if (!IndexBuffer || IndexBuffer->Num() < 3 ||
		static_cast<uint32>(UVChannel) >= LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords())
	{
		return false;
	}

	const uint32 NumUVVertices = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumVertices();
	const uint32 NumPositionVertices = LODData.StaticVertexBuffers.PositionVertexBuffer.GetNumVertices();
	const int32 TriangleCount = IndexBuffer->Num() / 3;
	INC_DWORD_STAT_BY(STAT_MeshPaintingCore_UVIslandTrianglesScanned, TriangleCount);
	OutTriangles.Reserve(TriangleCount);
	if (OutTriangleVertexIndices) OutTriangleVertexIndices->Reserve(TriangleCount);
	if (OutTriangleArraySectionIds) OutTriangleArraySectionIds->Reserve(TriangleCount);
	double LocalAreaSum = 0.0;
	double UVAreaSum = 0.0;

	for (int32 SectionIndex = 0; SectionIndex < LODData.RenderSections.Num(); ++SectionIndex)
	{
		const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];
		const int32 FirstSectionTriangleIndex = static_cast<int32>(Section.BaseIndex / 3);
		for (uint32 LocalTriangleIndex = 0; LocalTriangleIndex < Section.NumTriangles; ++LocalTriangleIndex)
		{
			const int32 TriangleIndex = FirstSectionTriangleIndex + static_cast<int32>(LocalTriangleIndex);
			const int32 FirstIndex = TriangleIndex * 3;
			if (FirstIndex < 0 || FirstIndex + 2 >= IndexBuffer->Num()) continue;

			const uint32 Index0 = IndexBuffer->Get(FirstIndex);
			const uint32 Index1 = IndexBuffer->Get(FirstIndex + 1);
			const uint32 Index2 = IndexBuffer->Get(FirstIndex + 2);
			if (Index0 >= NumUVVertices || Index1 >= NumUVVertices || Index2 >= NumUVVertices ||
				Index0 >= NumPositionVertices || Index1 >= NumPositionVertices || Index2 >= NumPositionVertices)
			{
				continue;
			}

			const FVector2f UV0 = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index0, UVChannel);
			const FVector2f UV1 = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index1, UVChannel);
			const FVector2f UV2 = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index2, UVChannel);
			const FVector2D UV0D(UV0.X, UV0.Y);
			const FVector2D UV1D(UV1.X, UV1.Y);
			const FVector2D UV2D(UV2.X, UV2.Y);
			RuntimeMeshPaint::FPaintUVTriangle UVTriangle = FRuntimeMeshPaintGeometry::MakePaintUVTriangle(
				TriangleIndex,
				UV0D,
				UV1D,
				UV2D);
			if (!FRuntimeMeshPaintGeometry::IsValidUVTriangle(UVTriangle)) continue;

			OutTriangles.Add(UVTriangle);
			FRuntimeMeshPaintGeometry::AccumulateTriangleLocalUVAreaScale(
				FVector(LODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(Index0)),
				FVector(LODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(Index1)),
				FVector(LODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(Index2)),
				UV0D,
				UV1D,
				UV2D,
				LocalAreaSum,
				UVAreaSum);
			if (OutTriangleVertexIndices)
			{
				OutTriangleVertexIndices->Add(FIntVector(
					static_cast<int32>(Index0),
					static_cast<int32>(Index1),
					static_cast<int32>(Index2)));
			}
			if (OutTriangleArraySectionIds)
			{
				OutTriangleArraySectionIds->Add(SectionIndex);
			}
		}
	}

	if (OutAverageLocalUnitsPerUV) *OutAverageLocalUnitsPerUV = FRuntimeMeshPaintGeometry::MakeAverageUnitsPerUV(LocalAreaSum, UVAreaSum);
	return OutTriangles.Num() > 0;
}
void FRuntimeMeshPaintSkeletalMesh::BuildSkeletalMeshTriangleSectionIds(
	const USkeletalMeshComponent* SkeletalMeshComponent, int32 TriangleCount, TArray<int32>& OutTriangleSectionIds)
{
	OutTriangleSectionIds.Init(INDEX_NONE, TriangleCount);

	const FSkeletalMeshRenderData* RenderData = SkeletalMeshComponent ? SkeletalMeshComponent->GetSkeletalMeshRenderData() : nullptr;
	if (!RenderData || RenderData->LODRenderData.Num() <= RuntimeMeshPaintCacheLODIndex) return;

	const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[RuntimeMeshPaintCacheLODIndex];
	for (int32 SectionIndex = 0; SectionIndex < LODData.RenderSections.Num(); ++SectionIndex)
	{
		const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];
		const int32 FirstTriangleIndex = Section.BaseIndex / 3;
		for (uint32 LocalTriangleIndex = 0; LocalTriangleIndex < Section.NumTriangles; ++LocalTriangleIndex)
		{
			const int32 TriangleIndex = FirstTriangleIndex + static_cast<int32>(LocalTriangleIndex);
			if (OutTriangleSectionIds.IsValidIndex(TriangleIndex))
			{
				OutTriangleSectionIds[TriangleIndex] = SectionIndex;
			}
		}
	}
}
bool FRuntimeMeshPaintSkeletalMesh::BuildSkeletalMeshUVCacheDescriptor(
	const USkeletalMeshComponent* SkeletalMeshComponent, int32 UVChannel, float UVConnectionTolerance,
	FPaintUVCacheDescriptor& OutDescriptor)
{
	USkeletalMesh* SkeletalMesh = SkeletalMeshComponent ? SkeletalMeshComponent->GetSkeletalMeshAsset() : nullptr;
	const FSkeletalMeshRenderData* RenderData = SkeletalMeshComponent ? SkeletalMeshComponent->GetSkeletalMeshRenderData() : nullptr;
	if (!SkeletalMesh || !RenderData || RenderData->LODRenderData.Num() <= RuntimeMeshPaintCacheLODIndex || UVChannel < 0) return false;

	const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[RuntimeMeshPaintCacheLODIndex];
	const FRawStaticIndexBuffer16or32Interface* IndexBuffer = LODData.MultiSizeIndexContainer.IsIndexBufferValid()
		? LODData.MultiSizeIndexContainer.GetIndexBuffer()
		: nullptr;
	const int32 NumTexCoords = static_cast<int32>(LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords());
	if (!IndexBuffer || IndexBuffer->Num() < 3 || UVChannel >= NumTexCoords) return false;

	OutDescriptor = FPaintUVCacheDescriptor();
	OutDescriptor.MeshAssetKey = FObjectKey(SkeletalMesh);
	OutDescriptor.MeshAsset = SkeletalMesh;
	OutDescriptor.MeshType = EPaintUVCacheMeshType::SkeletalMesh;
	OutDescriptor.LODIndex = RuntimeMeshPaintCacheLODIndex;
	OutDescriptor.UVChannel = UVChannel;
	OutDescriptor.UVConnectionToleranceKey = FRuntimeMeshPaintUVCache::MakeUVConnectionToleranceKey(UVConnectionTolerance);
	OutDescriptor.IndexCount = IndexBuffer->Num();
	OutDescriptor.TriangleCount = IndexBuffer->Num() / 3;
	OutDescriptor.UVVertexCount = static_cast<int32>(LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumVertices());
	OutDescriptor.NumTexCoords = NumTexCoords;
	OutDescriptor.RenderDataPointer = RenderData;
	OutDescriptor.LODDataPointer = &LODData;
	return OutDescriptor.MeshAsset.IsValid() && OutDescriptor.TriangleCount > 0;
}
static float ComputePointAABBDistanceSq(const FVector& Point, const FVector& BoundsMin, const FVector& BoundsMax)
{
	double DistanceSq = 0.0;

	if (Point.X < BoundsMin.X)
	{
		DistanceSq += FMath::Square(BoundsMin.X - Point.X);
	}
	else if (Point.X > BoundsMax.X)
	{
		DistanceSq += FMath::Square(Point.X - BoundsMax.X);
	}

	if (Point.Y < BoundsMin.Y)
	{
		DistanceSq += FMath::Square(BoundsMin.Y - Point.Y);
	}
	else if (Point.Y > BoundsMax.Y)
	{
		DistanceSq += FMath::Square(Point.Y - BoundsMax.Y);
	}

	if (Point.Z < BoundsMin.Z)
	{
		DistanceSq += FMath::Square(BoundsMin.Z - Point.Z);
	}
	else if (Point.Z > BoundsMax.Z)
	{
		DistanceSq += FMath::Square(Point.Z - BoundsMax.Z);
	}

	return static_cast<float>(DistanceSq);
}

bool FRuntimeMeshPaintSkeletalMesh::GetSkeletalMeshTriangleVertexIndices(
	const FSkeletalMeshLODRenderData& LODData,
	const FRawStaticIndexBuffer16or32Interface& IndexBuffer,
	int32 TriangleIndex,
	FIntVector& OutVertexIndices)
{
	const int32 FirstIndex = TriangleIndex * 3;
	if (TriangleIndex < 0 || FirstIndex + 2 >= IndexBuffer.Num()) return false;

	const uint32 Index0 = IndexBuffer.Get(FirstIndex);
	const uint32 Index1 = IndexBuffer.Get(FirstIndex + 1);
	const uint32 Index2 = IndexBuffer.Get(FirstIndex + 2);
	const uint32 NumVertices = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumVertices();
	if (Index0 >= NumVertices || Index1 >= NumVertices || Index2 >= NumVertices ||
		Index0 > static_cast<uint32>(TNumericLimits<int32>::Max()) ||
		Index1 > static_cast<uint32>(TNumericLimits<int32>::Max()) ||
		Index2 > static_cast<uint32>(TNumericLimits<int32>::Max()))
	{
		return false;
	}

	OutVertexIndices = FIntVector(
		static_cast<int32>(Index0),
		static_cast<int32>(Index1),
		static_cast<int32>(Index2));
	return true;
}

bool FRuntimeMeshPaintSkeletalMesh::SkinSkeletalTrianglePositions(
	USkeletalMeshComponent* SkeletalMeshComponent,
	const FSkeletalMeshLODRenderData& LODData,
	const FSkinWeightVertexBuffer& SkinWeightBuffer,
	const FIntVector& VertexIndices,
	TArray<FMatrix44f>& CachedRefToLocals,
	FVector& OutPosition0,
	FVector& OutPosition1,
	FVector& OutPosition2)
{
	if (!SkeletalMeshComponent || CachedRefToLocals.Num() == 0) return false;

	const int32 NumVertices = static_cast<int32>(LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumVertices());
	if (VertexIndices.X < 0 || VertexIndices.X >= NumVertices ||
		VertexIndices.Y < 0 || VertexIndices.Y >= NumVertices ||
		VertexIndices.Z < 0 || VertexIndices.Z >= NumVertices)
	{
		return false;
	}

	OutPosition0 = FVector(USkeletalMeshComponent::GetSkinnedVertexPosition(
		SkeletalMeshComponent, VertexIndices.X, LODData, SkinWeightBuffer, CachedRefToLocals));
	OutPosition1 = FVector(USkeletalMeshComponent::GetSkinnedVertexPosition(
		SkeletalMeshComponent, VertexIndices.Y, LODData, SkinWeightBuffer, CachedRefToLocals));
	OutPosition2 = FVector(USkeletalMeshComponent::GetSkinnedVertexPosition(
		SkeletalMeshComponent, VertexIndices.Z, LODData, SkinWeightBuffer, CachedRefToLocals));
	return true;
}

uint32 BeginSkeletalVertexSkinQuery(const FPaintUVCache& Cache, int32 NumVertices)
{
	if (NumVertices <= 0) return 0;

	if (Cache.SkeletalVertexSkinMarks.Num() != NumVertices ||
		Cache.SkeletalVertexSkinPositions.Num() != NumVertices ||
		Cache.SkeletalVertexSkinSerial == MAX_uint32)
	{
		Cache.SkeletalVertexSkinMarks.Init(0, NumVertices);
		Cache.SkeletalVertexSkinPositions.SetNumUninitialized(NumVertices);
		Cache.SkeletalVertexSkinSerial = 1;
	}

	return Cache.SkeletalVertexSkinSerial++;
}

static bool SkinSkeletalVertexPositionCached(
	USkeletalMeshComponent* SkeletalMeshComponent,
	const FSkeletalMeshLODRenderData& LODData,
	const FSkinWeightVertexBuffer& SkinWeightBuffer,
	int32 VertexIndex,
	TArray<FMatrix44f>& CachedRefToLocals,
	const FPaintUVCache& Cache,
	uint32 SkinQueryId,
	FVector& OutPosition)
{
	if (!SkeletalMeshComponent || CachedRefToLocals.Num() == 0 || SkinQueryId == 0 ||
		!Cache.SkeletalVertexSkinMarks.IsValidIndex(VertexIndex) ||
		!Cache.SkeletalVertexSkinPositions.IsValidIndex(VertexIndex))
	{
		return false;
	}

	if (Cache.SkeletalVertexSkinMarks[VertexIndex] != SkinQueryId)
	{
		Cache.SkeletalVertexSkinPositions[VertexIndex] = USkeletalMeshComponent::GetSkinnedVertexPosition(
			SkeletalMeshComponent,
			VertexIndex,
			LODData,
			SkinWeightBuffer,
			CachedRefToLocals);
		Cache.SkeletalVertexSkinMarks[VertexIndex] = SkinQueryId;
	}

	OutPosition = FVector(Cache.SkeletalVertexSkinPositions[VertexIndex]);
	return true;
}

static bool SkinSkeletalTrianglePositionsCached(
	USkeletalMeshComponent* SkeletalMeshComponent,
	const FSkeletalMeshLODRenderData& LODData,
	const FSkinWeightVertexBuffer& SkinWeightBuffer,
	const FIntVector& VertexIndices,
	TArray<FMatrix44f>& CachedRefToLocals,
	const FPaintUVCache& Cache,
	uint32 SkinQueryId,
	FVector& OutPosition0,
	FVector& OutPosition1,
	FVector& OutPosition2)
{
	return SkinSkeletalVertexPositionCached(
			SkeletalMeshComponent, LODData, SkinWeightBuffer, VertexIndices.X,
			CachedRefToLocals, Cache, SkinQueryId, OutPosition0) &&
		SkinSkeletalVertexPositionCached(
			SkeletalMeshComponent, LODData, SkinWeightBuffer, VertexIndices.Y,
			CachedRefToLocals, Cache, SkinQueryId, OutPosition1) &&
		SkinSkeletalVertexPositionCached(
			SkeletalMeshComponent, LODData, SkinWeightBuffer, VertexIndices.Z,
			CachedRefToLocals, Cache, SkinQueryId, OutPosition2);
}

static bool GetSkeletalMeshTriangleDistanceSqLazyCached(
	USkeletalMeshComponent* SkeletalMeshComponent,
	const FSkeletalMeshLODRenderData& LODData,
	const FSkinWeightVertexBuffer& SkinWeightBuffer,
	const FIntVector& VertexIndices,
	TArray<FMatrix44f>& CachedRefToLocals,
	const FPaintUVCache& Cache,
	uint32 SkinQueryId,
	const FVector& LocalHitPosition,
	float CurrentBestDistanceSq,
	float& OutDistanceSq)
{
	FVector Position0 = FVector::ZeroVector;
	FVector Position1 = FVector::ZeroVector;
	FVector Position2 = FVector::ZeroVector;
	if (!SkinSkeletalTrianglePositionsCached(
		SkeletalMeshComponent,
		LODData,
		SkinWeightBuffer,
		VertexIndices,
		CachedRefToLocals,
		Cache,
		SkinQueryId,
		Position0,
		Position1,
		Position2))
	{
		return false;
	}

	const FVector BoundsMin(
		FMath::Min3(Position0.X, Position1.X, Position2.X),
		FMath::Min3(Position0.Y, Position1.Y, Position2.Y),
		FMath::Min3(Position0.Z, Position1.Z, Position2.Z));
	const FVector BoundsMax(
		FMath::Max3(Position0.X, Position1.X, Position2.X),
		FMath::Max3(Position0.Y, Position1.Y, Position2.Y),
		FMath::Max3(Position0.Z, Position1.Z, Position2.Z));
	if (ComputePointAABBDistanceSq(LocalHitPosition, BoundsMin, BoundsMax) > CurrentBestDistanceSq)
	{
		return false;
	}

	const FVector ClosestPoint = FMath::ClosestPointOnTriangleToPoint(LocalHitPosition, Position0, Position1, Position2);
	OutDistanceSq = static_cast<float>(FVector::DistSquared(LocalHitPosition, ClosestPoint));
	return true;
}

static bool GetSkeletalMeshTriangleUVLazyCached(
	USkeletalMeshComponent* SkeletalMeshComponent,
	const FSkeletalMeshLODRenderData& LODData,
	const FSkinWeightVertexBuffer& SkinWeightBuffer,
	const FIntVector& VertexIndices,
	TArray<FMatrix44f>& CachedRefToLocals,
	const FPaintUVCache& Cache,
	uint32 SkinQueryId,
	int32 UVChannel,
	const FVector& LocalHitPosition,
	FVector2D& OutUV,
	FVector* OutLocalSurfacePosition = nullptr,
	FVector* OutLocalSurfaceNormal = nullptr)
{
	if (UVChannel < 0 ||
		static_cast<uint32>(UVChannel) >= LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords())
	{
		return false;
	}

	FVector Position0 = FVector::ZeroVector;
	FVector Position1 = FVector::ZeroVector;
	FVector Position2 = FVector::ZeroVector;
	if (!SkinSkeletalTrianglePositionsCached(
		SkeletalMeshComponent,
		LODData,
		SkinWeightBuffer,
		VertexIndices,
		CachedRefToLocals,
		Cache,
		SkinQueryId,
		Position0,
		Position1,
		Position2))
	{
		return false;
	}

	const FVector ClosestPoint = FMath::ClosestPointOnTriangleToPoint(LocalHitPosition, Position0, Position1, Position2);
	const FVector Barycentric = FMath::ComputeBaryCentric2D(ClosestPoint, Position0, Position1, Position2);
	const FVector TriangleNormal = FVector::CrossProduct(Position1 - Position0, Position2 - Position0).GetSafeNormal(
		SMALL_NUMBER,
		FVector::UpVector);

	const FVector2f UV0 = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndices.X, UVChannel);
	const FVector2f UV1 = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndices.Y, UVChannel);
	const FVector2f UV2 = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndices.Z, UVChannel);
	const FVector2f UV =
		(UV0 * static_cast<float>(Barycentric.X)) +
		(UV1 * static_cast<float>(Barycentric.Y)) +
		(UV2 * static_cast<float>(Barycentric.Z));

	if (OutLocalSurfacePosition) *OutLocalSurfacePosition = ClosestPoint;
	if (OutLocalSurfaceNormal) *OutLocalSurfaceNormal = TriangleNormal;

	OutUV = FVector2D(UV.X, UV.Y);
	return true;
}

static bool GetSkeletalMeshTriangleRayUVLazyCached(
	USkeletalMeshComponent* SkeletalMeshComponent,
	const FSkeletalMeshLODRenderData& LODData,
	const FSkinWeightVertexBuffer& SkinWeightBuffer,
	const FIntVector& VertexIndices,
	TArray<FMatrix44f>& CachedRefToLocals,
	const FPaintUVCache& Cache,
	uint32 SkinQueryId,
	int32 UVChannel,
	const FVector& LocalTraceStart,
	const FVector& LocalTraceEnd,
	FVector2D& OutUV,
	float* OutDistanceSq = nullptr,
	FVector* OutLocalIntersectionPoint = nullptr,
	FVector* OutLocalTriangleNormal = nullptr)
{
	if (UVChannel < 0 ||
		static_cast<uint32>(UVChannel) >= LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords())
	{
		return false;
	}

	FVector Position0 = FVector::ZeroVector;
	FVector Position1 = FVector::ZeroVector;
	FVector Position2 = FVector::ZeroVector;
	if (!SkinSkeletalTrianglePositionsCached(
		SkeletalMeshComponent,
		LODData,
		SkinWeightBuffer,
		VertexIndices,
		CachedRefToLocals,
		Cache,
		SkinQueryId,
		Position0,
		Position1,
		Position2))
	{
		return false;
	}

	FVector IntersectionPoint = FVector::ZeroVector;
	FVector TriangleNormal = FVector::ZeroVector;
	if (!FMath::SegmentTriangleIntersection(
		LocalTraceStart,
		LocalTraceEnd,
		Position0,
		Position1,
		Position2,
		IntersectionPoint,
		TriangleNormal))
	{
		return false;
	}

	const FVector Barycentric = FMath::ComputeBaryCentric2D(IntersectionPoint, Position0, Position1, Position2);
	const FVector2f UV0 = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndices.X, UVChannel);
	const FVector2f UV1 = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndices.Y, UVChannel);
	const FVector2f UV2 = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndices.Z, UVChannel);
	const FVector2f UV =
		(UV0 * static_cast<float>(Barycentric.X)) +
		(UV1 * static_cast<float>(Barycentric.Y)) +
		(UV2 * static_cast<float>(Barycentric.Z));

	if (OutDistanceSq) *OutDistanceSq = FVector::DistSquared(LocalTraceStart, IntersectionPoint);
	if (OutLocalIntersectionPoint) *OutLocalIntersectionPoint = IntersectionPoint;
	if (OutLocalTriangleNormal)
	{
		FVector SafeTriangleNormal = TriangleNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
		const FVector ToTraceStart = (LocalTraceStart - IntersectionPoint).GetSafeNormal();
		if (!ToTraceStart.IsNearlyZero() && FVector::DotProduct(SafeTriangleNormal, ToTraceStart) < 0.0)
		{
			SafeTriangleNormal *= -1.0;
		}
		*OutLocalTriangleNormal = SafeTriangleNormal;
	}

	OutUV = FVector2D(UV.X, UV.Y);
	return true;
}

static int32 FindSkeletalTriangleArrayIndex(const FPaintUVCache& Cache, int32 TriangleIndex)
{
	if (Cache.FaceIndexToTriangleArrayIndex.IsValidIndex(TriangleIndex))
	{
		const int32 TriangleArrayIndex = Cache.FaceIndexToTriangleArrayIndex[TriangleIndex];
		if (Cache.Triangles.IsValidIndex(TriangleArrayIndex) &&
			Cache.Triangles[TriangleArrayIndex].TriangleIndex == TriangleIndex)
		{
			return TriangleArrayIndex;
		}
	}

	return INDEX_NONE;
}

static int32 ResolveSkeletalTriangleArrayIndex(
	USkeletalMeshComponent* SkeletalMeshComponent,
	int32 UVChannel,
	int32 TriangleIndex)
{
	if (!SkeletalMeshComponent || TriangleIndex == INDEX_NONE) return INDEX_NONE;

	TSharedPtr<FPaintUVCache> Cache = FRuntimeMeshPaintUVCache::FindOrBuildPaintUVCache(SkeletalMeshComponent, UVChannel, 0.0f);
	return Cache.IsValid() ? FindSkeletalTriangleArrayIndex(*Cache, TriangleIndex) : INDEX_NONE;
}

static void SetResolvedSkeletalTriangle(
	USkeletalMeshComponent* SkeletalMeshComponent,
	int32 UVChannel,
	int32 TriangleIndex,
	int32 TriangleArrayIndex,
	int32* OutResolvedFaceIndex,
	int32* OutResolvedTriangleArrayIndex)
{
	if (OutResolvedFaceIndex)
	{
		*OutResolvedFaceIndex = TriangleIndex;
	}
	if (OutResolvedTriangleArrayIndex)
	{
		*OutResolvedTriangleArrayIndex = TriangleArrayIndex != INDEX_NONE
			? TriangleArrayIndex
			: ResolveSkeletalTriangleArrayIndex(SkeletalMeshComponent, UVChannel, TriangleIndex);
	}
}

static bool FindSkeletalMeshLazyFullFallbackUV(
	USkeletalMeshComponent* SkeletalMeshComponent,
	const FSkeletalMeshLODRenderData& LODData,
	const FSkinWeightVertexBuffer& SkinWeightBuffer,
	int32 UVChannel,
	const FVector& LocalHitPosition,
	const FVector& LocalTraceStart,
	const FVector& LocalTraceEnd,
	float MaxFallbackDistance,
	FVector2D& OutUV,
	int32* OutResolvedFaceIndex = nullptr,
	int32* OutResolvedTriangleArrayIndex = nullptr,
	FVector* OutLocalSurfacePosition = nullptr,
	FVector* OutLocalSurfaceNormal = nullptr,
	bool* bOutUsedRayIntersection = nullptr,
	bool bRequireRayIntersection = false)
{
	if (OutLocalSurfacePosition) *OutLocalSurfacePosition = FVector::ZeroVector;
	if (OutLocalSurfaceNormal) *OutLocalSurfaceNormal = FVector::UpVector;
	if (bOutUsedRayIntersection) *bOutUsedRayIntersection = false;

	TSharedPtr<FPaintUVCache> Cache = FRuntimeMeshPaintUVCache::FindOrBuildPaintUVCache(SkeletalMeshComponent, UVChannel, 0.0f);
	if (!Cache.IsValid() ||
		Cache->MeshType != EPaintUVCacheMeshType::SkeletalMesh ||
		Cache->TriangleVertexIndices.Num() != Cache->Triangles.Num() ||
		Cache->Triangles.Num() == 0)
	{
		return false;
	}

	TArray<FMatrix44f> CachedRefToLocals;
	SkeletalMeshComponent->CacheRefToLocalMatrices(CachedRefToLocals);
	if (CachedRefToLocals.Num() == 0) return false;

	const int32 NumVertices = static_cast<int32>(LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetNumVertices());
	const uint32 SkinQueryId = BeginSkeletalVertexSkinQuery(*Cache, NumVertices);
	if (SkinQueryId == 0) return false;

	const int32 TriangleCount = Cache->Triangles.Num();
	FSkeletalTriangleDistanceSearchResult BestRayTriangle;
	int32 BestRayTriangleArrayIndex = INDEX_NONE;
	FVector2D BestRayUV = FVector2D::ZeroVector;
	FVector BestRayLocalPosition = FVector::ZeroVector;
	FVector BestRayLocalNormal = FVector::UpVector;
	INC_DWORD_STAT_BY(STAT_MeshPaintingCore_SkeletalUVFallbackTrianglesScanned, TriangleCount);
	for (int32 TriangleArrayIndex = 0; TriangleArrayIndex < TriangleCount; ++TriangleArrayIndex)
	{
		if (!Cache->TriangleVertexIndices.IsValidIndex(TriangleArrayIndex)) continue;

		float DistanceSq = 0.0f;
		FVector2D CandidateUV = FVector2D::ZeroVector;
		FVector CandidateLocalPosition = FVector::ZeroVector;
		FVector CandidateLocalNormal = FVector::UpVector;
		if (!GetSkeletalMeshTriangleRayUVLazyCached(
			SkeletalMeshComponent,
			LODData,
			SkinWeightBuffer,
			Cache->TriangleVertexIndices[TriangleArrayIndex],
			CachedRefToLocals,
			*Cache,
			SkinQueryId,
			UVChannel,
			LocalTraceStart,
			LocalTraceEnd,
			CandidateUV,
			&DistanceSq,
			&CandidateLocalPosition,
			&CandidateLocalNormal))
		{
			continue;
		}

		const FSkeletalTriangleDistanceSearchResult Candidate{
			DistanceSq,
			Cache->Triangles[TriangleArrayIndex].TriangleIndex};
		if (Candidate.IsBetterThan(BestRayTriangle))
		{
			BestRayTriangle = Candidate;
			BestRayTriangleArrayIndex = TriangleArrayIndex;
			BestRayUV = CandidateUV;
			BestRayLocalPosition = CandidateLocalPosition;
			BestRayLocalNormal = CandidateLocalNormal;
		}
	}

	if (BestRayTriangleArrayIndex != INDEX_NONE)
	{
		OutUV = BestRayUV;
		if (OutLocalSurfacePosition) *OutLocalSurfacePosition = BestRayLocalPosition;
		if (OutLocalSurfaceNormal) *OutLocalSurfaceNormal = BestRayLocalNormal;
		if (bOutUsedRayIntersection) *bOutUsedRayIntersection = true;
		SetResolvedSkeletalTriangle(
			SkeletalMeshComponent,
			UVChannel,
			BestRayTriangle.TriangleIndex,
			BestRayTriangleArrayIndex,
			OutResolvedFaceIndex,
			OutResolvedTriangleArrayIndex);
		return true;
	}

	if (bRequireRayIntersection)
	{
		return false;
	}

	FSkeletalTriangleDistanceSearchResult BestTriangle;
	int32 BestTriangleArrayIndex = INDEX_NONE;
	INC_DWORD_STAT_BY(STAT_MeshPaintingCore_SkeletalUVFallbackTrianglesScanned, TriangleCount);
	for (int32 TriangleArrayIndex = 0; TriangleArrayIndex < TriangleCount; ++TriangleArrayIndex)
	{
		if (!Cache->TriangleVertexIndices.IsValidIndex(TriangleArrayIndex)) continue;

		float DistanceSq = 0.0f;
		if (!GetSkeletalMeshTriangleDistanceSqLazyCached(
			SkeletalMeshComponent,
			LODData,
			SkinWeightBuffer,
			Cache->TriangleVertexIndices[TriangleArrayIndex],
			CachedRefToLocals,
			*Cache,
			SkinQueryId,
			LocalHitPosition,
			BestTriangle.DistanceSq,
			DistanceSq))
		{
			continue;
		}

		const FSkeletalTriangleDistanceSearchResult Candidate{
			DistanceSq,
			Cache->Triangles[TriangleArrayIndex].TriangleIndex};
		if (Candidate.IsBetterThan(BestTriangle))
		{
			BestTriangle = Candidate;
			BestTriangleArrayIndex = TriangleArrayIndex;
		}
	}

	if (BestTriangleArrayIndex == INDEX_NONE) return false;
	if (MaxFallbackDistance > 0.0f && BestTriangle.DistanceSq > FMath::Square(MaxFallbackDistance)) return false;

	FVector FallbackLocalPosition = FVector::ZeroVector;
	FVector FallbackLocalNormal = FVector::UpVector;
	const bool bResolvedUV = GetSkeletalMeshTriangleUVLazyCached(
		SkeletalMeshComponent,
		LODData,
		SkinWeightBuffer,
		Cache->TriangleVertexIndices[BestTriangleArrayIndex],
		CachedRefToLocals,
		*Cache,
		SkinQueryId,
		UVChannel,
		LocalHitPosition,
		OutUV,
		&FallbackLocalPosition,
		&FallbackLocalNormal);
	if (bResolvedUV)
	{
		if (OutLocalSurfacePosition) *OutLocalSurfacePosition = FallbackLocalPosition;
		if (OutLocalSurfaceNormal) *OutLocalSurfaceNormal = FallbackLocalNormal;
		SetResolvedSkeletalTriangle(
			SkeletalMeshComponent,
			UVChannel,
			BestTriangle.TriangleIndex,
			BestTriangleArrayIndex,
			OutResolvedFaceIndex,
			OutResolvedTriangleArrayIndex);
	}

	return bResolvedUV;
}

bool FRuntimeMeshPaintSkeletalMesh::FindSkeletalMeshFaceUV(
	const FHitResult& HitResult, int32 UVChannel, float MaxFallbackDistance,
	FVector2D& OutUV, int32* OutResolvedFaceIndex, int32* OutResolvedTriangleArrayIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_MeshPaintingCore_FindSkeletalMeshFaceUV);

	if (OutResolvedTriangleArrayIndex)
	{
		*OutResolvedTriangleArrayIndex = INDEX_NONE;
	}

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

	const FVector LocalHitPosition = SkeletalMeshComponent->GetComponentTransform().InverseTransformPosition(HitResult.ImpactPoint);
	const FVector LocalTraceStart = SkeletalMeshComponent->GetComponentTransform().InverseTransformPosition(HitResult.TraceStart);
	const FVector LocalTraceEnd = SkeletalMeshComponent->GetComponentTransform().InverseTransformPosition(HitResult.TraceEnd);

	return FindSkeletalMeshLazyFullFallbackUV(
		SkeletalMeshComponent,
		LODData,
		*SkinWeightBuffer,
		UVChannel,
		LocalHitPosition,
		LocalTraceStart,
		LocalTraceEnd,
		MaxFallbackDistance,
		OutUV,
		OutResolvedFaceIndex,
		OutResolvedTriangleArrayIndex);
}

bool FRuntimeMeshPaintSkeletalMesh::ResolveSkeletalMeshVisualHit(
	const FHitResult& HitResult, int32 UVChannel, float MaxFallbackDistance,
	bool bRequireRayIntersection, FSkeletalMeshPaintVisualHit& OutVisualHit)
{
	SCOPE_CYCLE_COUNTER(STAT_MeshPaintingCore_FindSkeletalMeshFaceUV);

	OutVisualHit = FSkeletalMeshPaintVisualHit();

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

	const FTransform& ComponentTransform = SkeletalMeshComponent->GetComponentTransform();
	const FVector LocalHitPosition = ComponentTransform.InverseTransformPosition(HitResult.ImpactPoint);
	const FVector LocalTraceStart = ComponentTransform.InverseTransformPosition(HitResult.TraceStart);
	const FVector LocalTraceEnd = ComponentTransform.InverseTransformPosition(HitResult.TraceEnd);

	bool bUsedRayIntersection = false;
	if (!FindSkeletalMeshLazyFullFallbackUV(
		SkeletalMeshComponent,
		LODData,
		*SkinWeightBuffer,
		UVChannel,
		LocalHitPosition,
		LocalTraceStart,
		LocalTraceEnd,
		MaxFallbackDistance,
		OutVisualHit.UV,
		&OutVisualHit.FaceIndex,
		&OutVisualHit.TriangleArrayIndex,
		&OutVisualHit.LocalPosition,
		&OutVisualHit.LocalNormal,
		&bUsedRayIntersection,
		bRequireRayIntersection))
	{
		return false;
	}

	OutVisualHit.bRayIntersection = bUsedRayIntersection;
	OutVisualHit.WorldPosition = ComponentTransform.TransformPosition(OutVisualHit.LocalPosition);
	OutVisualHit.WorldNormal = ComponentTransform.TransformVectorNoScale(OutVisualHit.LocalNormal).GetSafeNormal(
		SMALL_NUMBER,
		HitResult.ImpactNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector));
	return true;
}
