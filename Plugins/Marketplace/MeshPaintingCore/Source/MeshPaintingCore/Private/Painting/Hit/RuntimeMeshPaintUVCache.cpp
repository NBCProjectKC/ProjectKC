// Copyright Shared Orbit 2026. All Rights Reserved.
#include "RuntimeMeshPaintHitUtilsInternal.h"

static TMap<FPaintUVCacheKey, TSharedPtr<FPaintUVCache>> GPaintUVCacheByMesh;

int32 FRuntimeMeshPaintUVCache::MakeUVConnectionToleranceKey(float UVConnectionTolerance)
{
	return FMath::RoundToInt(FMath::Max(0.0f, UVConnectionTolerance) * RuntimeMeshPaintUVToleranceHashScale);
}

float FRuntimeMeshPaintUVCache::MakeUVConnectionToleranceFromKey(int32 UVConnectionToleranceKey)
{
	return FMath::Max(0, UVConnectionToleranceKey) / RuntimeMeshPaintUVToleranceHashScale;
}

static FPaintUVCacheKey MakePaintUVCacheKey(const FPaintUVCacheDescriptor& Descriptor)
{
	FPaintUVCacheKey Key;
	Key.MeshAssetKey = Descriptor.MeshAssetKey;
	Key.MeshType = Descriptor.MeshType;
	Key.LODIndex = Descriptor.LODIndex;
	Key.UVChannel = Descriptor.UVChannel;
	Key.UVConnectionToleranceKey = Descriptor.UVConnectionToleranceKey;
	return Key;
}
bool FRuntimeMeshPaintUVCache::BuildPaintUVCacheDescriptor(
	UMeshComponent* MeshComponent, int32 UVChannel, float UVConnectionTolerance,
	FPaintUVCacheDescriptor& OutDescriptor)
{
	if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent))
	{
		return FRuntimeMeshPaintStaticMesh::BuildStaticMeshUVCacheDescriptor(StaticMeshComponent, UVChannel, UVConnectionTolerance, OutDescriptor);
	}

	if (const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
	{
		return FRuntimeMeshPaintSkeletalMesh::BuildSkeletalMeshUVCacheDescriptor(SkeletalMeshComponent, UVChannel, UVConnectionTolerance, OutDescriptor);
	}

	return false;
}

static FPaintUVTriangleBounds MakeTriangleBounds(const RuntimeMeshPaint::FPaintUVTriangle& Triangle)
{
	FPaintUVTriangleBounds Bounds;
	Bounds.Min.X = FMath::Min3(Triangle.UV0.X, Triangle.UV1.X, Triangle.UV2.X);
	Bounds.Min.Y = FMath::Min3(Triangle.UV0.Y, Triangle.UV1.Y, Triangle.UV2.Y);
	Bounds.Max.X = FMath::Max3(Triangle.UV0.X, Triangle.UV1.X, Triangle.UV2.X);
	Bounds.Max.Y = FMath::Max3(Triangle.UV0.Y, Triangle.UV1.Y, Triangle.UV2.Y);
	return Bounds;
}

static void ExpandIslandBounds(FPaintUVIsland& Island, const FPaintUVTriangleBounds& TriangleBounds)
{
	Island.BoundsMin.X = FMath::Min(Island.BoundsMin.X, TriangleBounds.Min.X);
	Island.BoundsMin.Y = FMath::Min(Island.BoundsMin.Y, TriangleBounds.Min.Y);
	Island.BoundsMax.X = FMath::Max(Island.BoundsMax.X, TriangleBounds.Max.X);
	Island.BoundsMax.Y = FMath::Max(Island.BoundsMax.Y, TriangleBounds.Max.Y);
}

static void BuildTriangleLookupData(FPaintUVCache& Cache)
{
	Cache.TriangleBounds.Reset(Cache.Triangles.Num());
	Cache.FaceIndexToTriangleArrayIndex.Init(INDEX_NONE, Cache.TriangleCount);

	for (int32 TriangleArrayIndex = 0; TriangleArrayIndex < Cache.Triangles.Num(); ++TriangleArrayIndex)
	{
		const RuntimeMeshPaint::FPaintUVTriangle& Triangle = Cache.Triangles[TriangleArrayIndex];
		Cache.TriangleBounds.Add(MakeTriangleBounds(Triangle));
		if (Cache.FaceIndexToTriangleArrayIndex.IsValidIndex(Triangle.TriangleIndex))
		{
			Cache.FaceIndexToTriangleArrayIndex[Triangle.TriangleIndex] = TriangleArrayIndex;
		}
	}
}

static void AddConnectedTrianglesForCachedEdge(
	const FUVEdgeKey& EdgeKey,
	const TMultiMap<FUVEdgeKey, int32>& EdgeToTriangles,
	const TArray<int32>& TriangleIslandIds,
	TArray<int32>& ConnectedTriangles,
	TArray<int32>& TriangleStack)
{
	ConnectedTriangles.Reset();
	EdgeToTriangles.MultiFind(EdgeKey, ConnectedTriangles);
	for (int32 ConnectedTriangle : ConnectedTriangles)
	{
		if (TriangleIslandIds.IsValidIndex(ConnectedTriangle) && TriangleIslandIds[ConnectedTriangle] == INDEX_NONE)
		{
			TriangleStack.Add(ConnectedTriangle);
		}
	}
}

static void BuildCachedUVIslands(FPaintUVCache& Cache)
{
	Cache.Islands.Reset();
	Cache.TriangleIslandIds.Init(INDEX_NONE, Cache.Triangles.Num());

	TMultiMap<FUVEdgeKey, int32> EdgeToTriangles;
	EdgeToTriangles.Reserve(Cache.Triangles.Num() * 3);
	for (int32 TriangleArrayIndex = 0; TriangleArrayIndex < Cache.Triangles.Num(); ++TriangleArrayIndex)
	{
		FRuntimeMeshPaintGeometry::AddTriangleEdgesToMap(
			Cache.Triangles[TriangleArrayIndex],
			TriangleArrayIndex,
			Cache.UVConnectionTolerance,
			EdgeToTriangles);
	}

	TArray<int32> TriangleStack;
	TArray<int32> ConnectedTriangles;
	TriangleStack.Reserve(128);

	for (int32 SeedTriangleArrayIndex = 0; SeedTriangleArrayIndex < Cache.Triangles.Num(); ++SeedTriangleArrayIndex)
	{
		if (Cache.TriangleIslandIds[SeedTriangleArrayIndex] != INDEX_NONE) continue;

		const int32 IslandId = Cache.Islands.AddDefaulted();
		FPaintUVIsland& Island = Cache.Islands[IslandId];
		TriangleStack.Reset();
		TriangleStack.Add(SeedTriangleArrayIndex);

		while (TriangleStack.Num() > 0)
		{
			const int32 TriangleArrayIndex = TriangleStack.Pop(EAllowShrinking::No);
			if (!Cache.Triangles.IsValidIndex(TriangleArrayIndex) ||
				Cache.TriangleIslandIds[TriangleArrayIndex] != INDEX_NONE)
			{
				continue;
			}

			Cache.TriangleIslandIds[TriangleArrayIndex] = IslandId;
			Island.TriangleArrayIndices.Add(TriangleArrayIndex);
			ExpandIslandBounds(Island, Cache.TriangleBounds[TriangleArrayIndex]);

			const RuntimeMeshPaint::FPaintUVTriangle& Triangle = Cache.Triangles[TriangleArrayIndex];
			AddConnectedTrianglesForCachedEdge(
				FRuntimeMeshPaintGeometry::MakeUVEdgeKey(Triangle.UV0, Triangle.UV1, Cache.UVConnectionTolerance),
				EdgeToTriangles, Cache.TriangleIslandIds, ConnectedTriangles, TriangleStack);
			AddConnectedTrianglesForCachedEdge(
				FRuntimeMeshPaintGeometry::MakeUVEdgeKey(Triangle.UV1, Triangle.UV2, Cache.UVConnectionTolerance),
				EdgeToTriangles, Cache.TriangleIslandIds, ConnectedTriangles, TriangleStack);
			AddConnectedTrianglesForCachedEdge(
				FRuntimeMeshPaintGeometry::MakeUVEdgeKey(Triangle.UV2, Triangle.UV0, Cache.UVConnectionTolerance),
				EdgeToTriangles, Cache.TriangleIslandIds, ConnectedTriangles, TriangleStack);
		}
	}
}

static int32 SelectUVGridResolution(int32 TriangleCount)
{
	if (TriangleCount < RuntimeMeshPaintUVGridTriangleThreshold) return 0;
	if (TriangleCount < RuntimeMeshPaintUVGridMediumTriangleThreshold) return RuntimeMeshPaintUVGridMediumResolution;
	if (TriangleCount < RuntimeMeshPaintUVGridLargeTriangleThreshold) return RuntimeMeshPaintUVGridLargeResolution;
	return RuntimeMeshPaintUVGridMaxResolution;
}

static bool GetUVGridRange(
	const FVector2D& BoundsMin, const FVector2D& BoundsMax, int32 Resolution,
	int32& OutMinX, int32& OutMaxX, int32& OutMinY, int32& OutMaxY)
{
	if (Resolution <= 0 ||
		BoundsMax.X < 0.0 || BoundsMax.Y < 0.0 ||
		BoundsMin.X > 1.0 || BoundsMin.Y > 1.0)
	{
		return false;
	}

	const double MinX = FMath::Clamp(BoundsMin.X, 0.0, 1.0);
	const double MaxX = FMath::Clamp(BoundsMax.X, 0.0, 1.0);
	const double MinY = FMath::Clamp(BoundsMin.Y, 0.0, 1.0);
	const double MaxY = FMath::Clamp(BoundsMax.Y, 0.0, 1.0);
	if (MinX > MaxX || MinY > MaxY) return false;

	OutMinX = FMath::Clamp(FMath::FloorToInt(MinX * Resolution), 0, Resolution - 1);
	OutMaxX = FMath::Clamp(FMath::FloorToInt(MaxX * Resolution), 0, Resolution - 1);
	OutMinY = FMath::Clamp(FMath::FloorToInt(MinY * Resolution), 0, Resolution - 1);
	OutMaxY = FMath::Clamp(FMath::FloorToInt(MaxY * Resolution), 0, Resolution - 1);
	return true;
}

static int32 GetUVGridCellIndex(int32 X, int32 Y, int32 Resolution)
{
	return Y * Resolution + X;
}

static void BuildCachedUVGrid(FPaintUVCache& Cache)
{
	Cache.UVGrid = FPaintUVGrid();

	const int32 Resolution = SelectUVGridResolution(Cache.Triangles.Num());
	if (Resolution <= 0) return;

	const int32 CellCount = Resolution * Resolution;
	TArray<int32> CellCounts;
	CellCounts.Init(0, CellCount);

	for (int32 TriangleArrayIndex = 0; TriangleArrayIndex < Cache.TriangleBounds.Num(); ++TriangleArrayIndex)
	{
		int32 MinX = 0;
		int32 MaxX = 0;
		int32 MinY = 0;
		int32 MaxY = 0;
		if (!GetUVGridRange(
			Cache.TriangleBounds[TriangleArrayIndex].Min,
			Cache.TriangleBounds[TriangleArrayIndex].Max,
			Resolution,
			MinX,
			MaxX,
			MinY,
			MaxY))
		{
			continue;
		}

		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				++CellCounts[GetUVGridCellIndex(X, Y, Resolution)];
			}
		}
	}

	Cache.UVGrid.Resolution = Resolution;
	Cache.UVGrid.CellOffsets.SetNumZeroed(CellCount + 1);
	for (int32 CellIndex = 0; CellIndex < CellCount; ++CellIndex)
	{
		Cache.UVGrid.CellOffsets[CellIndex + 1] = Cache.UVGrid.CellOffsets[CellIndex] + CellCounts[CellIndex];
	}

	Cache.UVGrid.TriangleArrayIndices.SetNumUninitialized(Cache.UVGrid.CellOffsets.Last());

	TArray<int32> WriteOffsets = Cache.UVGrid.CellOffsets;
	for (int32 TriangleArrayIndex = 0; TriangleArrayIndex < Cache.TriangleBounds.Num(); ++TriangleArrayIndex)
	{
		int32 MinX = 0;
		int32 MaxX = 0;
		int32 MinY = 0;
		int32 MaxY = 0;
		if (!GetUVGridRange(
			Cache.TriangleBounds[TriangleArrayIndex].Min,
			Cache.TriangleBounds[TriangleArrayIndex].Max,
			Resolution,
			MinX,
			MaxX,
			MinY,
			MaxY))
		{
			continue;
		}

		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				const int32 CellIndex = GetUVGridCellIndex(X, Y, Resolution);
				Cache.UVGrid.TriangleArrayIndices[WriteOffsets[CellIndex]++] = TriangleArrayIndex;
			}
		}
	}
}

static double ComputePointToUVSegmentDistanceSq(const FVector2D& Point, const FVector2D& SegmentStart, const FVector2D& SegmentEnd)
{
	const FVector2D Segment = SegmentEnd - SegmentStart;
	const double SegmentLengthSq = Segment.SizeSquared();
	if (SegmentLengthSq <= UE_DOUBLE_SMALL_NUMBER)
	{
		return FVector2D::DistSquared(Point, SegmentStart);
	}

	const double SegmentT = FMath::Clamp(
		FVector2D::DotProduct(Point - SegmentStart, Segment) / SegmentLengthSq,
		0.0,
		1.0);
	const FVector2D ClosestPoint = SegmentStart + Segment * SegmentT;
	return FVector2D::DistSquared(Point, ClosestPoint);
}

static double ComputePointToUVTriangleDistanceSq(
	const FVector2D& UV,
	const RuntimeMeshPaint::FPaintUVTriangle& Triangle,
	float Tolerance)
{
	if (FRuntimeMeshPaintGeometry::IsUVInsideTriangle(UV, Triangle, Tolerance))
	{
		return 0.0;
	}

	return FMath::Min3(
		ComputePointToUVSegmentDistanceSq(UV, Triangle.UV0, Triangle.UV1),
		ComputePointToUVSegmentDistanceSq(UV, Triangle.UV1, Triangle.UV2),
		ComputePointToUVSegmentDistanceSq(UV, Triangle.UV2, Triangle.UV0));
}

static void TryFindNearestUVSeedTriangle(
	const FPaintUVCache& Cache,
	int32 TriangleArrayIndex,
	const FVector2D& HitUV,
	double MaxDistanceSq,
	int32& InOutBestTriangleArrayIndex,
	double& InOutBestDistanceSq)
{
	if (!Cache.Triangles.IsValidIndex(TriangleArrayIndex)) return;

	const double DistanceSq = ComputePointToUVTriangleDistanceSq(
		HitUV,
		Cache.Triangles[TriangleArrayIndex],
		RuntimeMeshPaintUVIslandContainmentTolerance);
	if (DistanceSq > MaxDistanceSq || DistanceSq >= InOutBestDistanceSq) return;

	InOutBestTriangleArrayIndex = TriangleArrayIndex;
	InOutBestDistanceSq = DistanceSq;
}

static bool FindNearestUVSeedTriangleArrayIndex(
	const FPaintUVCache& Cache,
	const FVector2D& HitUV,
	int32& OutSeedArrayIndex)
{
	const double MaxDistance = FMath::Max3(
		RuntimeMeshPaintUVIslandContainmentTolerance * 64.0,
		Cache.UVConnectionTolerance * 8.0,
		0.002);
	const double MaxDistanceSq = FMath::Square(MaxDistance);
	int32 BestTriangleArrayIndex = INDEX_NONE;
	double BestDistanceSq = TNumericLimits<double>::Max();

	if (Cache.UVGrid.IsValid())
	{
		const FVector2D SearchExtent(MaxDistance, MaxDistance);
		int32 MinX = 0;
		int32 MaxX = 0;
		int32 MinY = 0;
		int32 MaxY = 0;
		if (GetUVGridRange(HitUV - SearchExtent, HitUV + SearchExtent, Cache.UVGrid.Resolution, MinX, MaxX, MinY, MaxY))
		{
			for (int32 Y = MinY; Y <= MaxY; ++Y)
			{
				for (int32 X = MinX; X <= MaxX; ++X)
				{
					const int32 CellIndex = GetUVGridCellIndex(X, Y, Cache.UVGrid.Resolution);
					const int32 CellStart = Cache.UVGrid.CellOffsets[CellIndex];
					const int32 CellEnd = Cache.UVGrid.CellOffsets[CellIndex + 1];
					for (int32 CellTriangleIndex = CellStart; CellTriangleIndex < CellEnd; ++CellTriangleIndex)
					{
						const int32 TriangleArrayIndex = Cache.UVGrid.TriangleArrayIndices.IsValidIndex(CellTriangleIndex)
							? Cache.UVGrid.TriangleArrayIndices[CellTriangleIndex]
							: INDEX_NONE;
						TryFindNearestUVSeedTriangle(
							Cache,
							TriangleArrayIndex,
							HitUV,
							MaxDistanceSq,
							BestTriangleArrayIndex,
							BestDistanceSq);
					}
				}
			}
		}
	}
	else
	{
		for (int32 TriangleArrayIndex = 0; TriangleArrayIndex < Cache.Triangles.Num(); ++TriangleArrayIndex)
		{
			TryFindNearestUVSeedTriangle(
				Cache,
				TriangleArrayIndex,
				HitUV,
				MaxDistanceSq,
				BestTriangleArrayIndex,
				BestDistanceSq);
		}
	}

	if (BestTriangleArrayIndex == INDEX_NONE) return false;

	OutSeedArrayIndex = BestTriangleArrayIndex;
	return true;
}

bool FRuntimeMeshPaintUVCache::FindSeedTriangleArrayIndex(
	const FPaintUVCache& Cache,
	int32 FaceIndex,
	const FVector2D& HitUV,
	int32& OutSeedArrayIndex)
{
	OutSeedArrayIndex = INDEX_NONE;

	int32 FaceIndexArrayIndex = INDEX_NONE;
	if (Cache.FaceIndexToTriangleArrayIndex.IsValidIndex(FaceIndex))
	{
		FaceIndexArrayIndex = Cache.FaceIndexToTriangleArrayIndex[FaceIndex];
		if (Cache.Triangles.IsValidIndex(FaceIndexArrayIndex) &&
			FRuntimeMeshPaintGeometry::IsUVInsideTriangle(HitUV, Cache.Triangles[FaceIndexArrayIndex], RuntimeMeshPaintUVIslandContainmentTolerance))
		{
			OutSeedArrayIndex = FaceIndexArrayIndex;
			return true;
		}
	}

	if (Cache.UVGrid.IsValid())
	{
		int32 MinX = 0;
		int32 MaxX = 0;
		int32 MinY = 0;
		int32 MaxY = 0;
		if (GetUVGridRange(HitUV, HitUV, Cache.UVGrid.Resolution, MinX, MaxX, MinY, MaxY))
		{
			const int32 CellIndex = GetUVGridCellIndex(MinX, MinY, Cache.UVGrid.Resolution);
			const int32 CellStart = Cache.UVGrid.CellOffsets[CellIndex];
			const int32 CellEnd = Cache.UVGrid.CellOffsets[CellIndex + 1];
			for (int32 CellTriangleIndex = CellStart; CellTriangleIndex < CellEnd; ++CellTriangleIndex)
			{
				const int32 TriangleArrayIndex = Cache.UVGrid.TriangleArrayIndices[CellTriangleIndex];
				if (Cache.Triangles.IsValidIndex(TriangleArrayIndex) &&
					FRuntimeMeshPaintGeometry::IsUVInsideTriangle(HitUV, Cache.Triangles[TriangleArrayIndex], RuntimeMeshPaintUVIslandContainmentTolerance))
				{
					OutSeedArrayIndex = TriangleArrayIndex;
					return true;
				}
			}
		}
	}

	for (int32 TriangleArrayIndex = 0; TriangleArrayIndex < Cache.Triangles.Num(); ++TriangleArrayIndex)
	{
		if (FRuntimeMeshPaintGeometry::IsUVInsideTriangle(HitUV, Cache.Triangles[TriangleArrayIndex], RuntimeMeshPaintUVIslandContainmentTolerance))
		{
			OutSeedArrayIndex = TriangleArrayIndex;
			return true;
		}
	}

	if (Cache.Triangles.IsValidIndex(FaceIndexArrayIndex))
	{
		OutSeedArrayIndex = FaceIndexArrayIndex;
		return true;
	}

	return FindNearestUVSeedTriangleArrayIndex(Cache, HitUV, OutSeedArrayIndex);
}

static bool BuildPaintUVCache(UMeshComponent* MeshComponent, const FPaintUVCacheDescriptor& Descriptor, FPaintUVCache& OutCache)
{
	OutCache = FPaintUVCache();
	OutCache.MeshAsset = Descriptor.MeshAsset;
	OutCache.MeshType = Descriptor.MeshType;
	OutCache.LODIndex = Descriptor.LODIndex;
	OutCache.UVChannel = Descriptor.UVChannel;
	OutCache.UVConnectionToleranceKey = Descriptor.UVConnectionToleranceKey;
	OutCache.IndexCount = Descriptor.IndexCount;
	OutCache.TriangleCount = Descriptor.TriangleCount;
	OutCache.UVVertexCount = Descriptor.UVVertexCount;
	OutCache.NumTexCoords = Descriptor.NumTexCoords;
	OutCache.RenderDataPointer = Descriptor.RenderDataPointer;
	OutCache.LODDataPointer = Descriptor.LODDataPointer;
	OutCache.UVConnectionTolerance = FRuntimeMeshPaintUVCache::MakeUVConnectionToleranceFromKey(Descriptor.UVConnectionToleranceKey);

	const bool bCollectedTriangles =
		Descriptor.MeshType == EPaintUVCacheMeshType::StaticMesh
			? FRuntimeMeshPaintStaticMesh::CollectStaticMeshUVTriangles(
				Cast<UStaticMeshComponent>(MeshComponent),
				Descriptor.UVChannel,
				OutCache.Triangles,
				&OutCache.AverageLocalUnitsPerUV)
			: FRuntimeMeshPaintSkeletalMesh::CollectSkeletalMeshUVTriangles(
				Cast<USkeletalMeshComponent>(MeshComponent),
				Descriptor.UVChannel,
				OutCache.Triangles,
				&OutCache.TriangleVertexIndices,
				&OutCache.TriangleArraySectionIds,
				&OutCache.AverageLocalUnitsPerUV);
	if (!bCollectedTriangles) return false;

	if (Descriptor.MeshType == EPaintUVCacheMeshType::StaticMesh)
	{
		FRuntimeMeshPaintStaticMesh::BuildStaticMeshTriangleSectionIds(Cast<UStaticMeshComponent>(MeshComponent), Descriptor.TriangleCount, OutCache.TriangleSectionIds);
	}
	else
	{
		FRuntimeMeshPaintSkeletalMesh::BuildSkeletalMeshTriangleSectionIds(Cast<USkeletalMeshComponent>(MeshComponent), Descriptor.TriangleCount, OutCache.TriangleSectionIds);
	}

	BuildTriangleLookupData(OutCache);
	BuildCachedUVIslands(OutCache);
	BuildCachedUVGrid(OutCache);
	return OutCache.Triangles.Num() > 0 && OutCache.Islands.Num() > 0;
}

static void RemoveInvalidPaintUVCaches()
{
	for (auto CacheIt = GPaintUVCacheByMesh.CreateIterator(); CacheIt; ++CacheIt)
	{
		const TSharedPtr<FPaintUVCache>& Cache = CacheIt.Value();
		if (!Cache.IsValid() || !Cache->MeshAsset.IsValid())
		{
			CacheIt.RemoveCurrent();
		}
	}
}

TSharedPtr<FPaintUVCache> FRuntimeMeshPaintUVCache::FindOrBuildPaintUVCache(
	UMeshComponent* MeshComponent,
	int32 UVChannel,
	float UVConnectionTolerance)
{
	FPaintUVCacheDescriptor Descriptor;
	if (!FRuntimeMeshPaintUVCache::BuildPaintUVCacheDescriptor(MeshComponent, UVChannel, UVConnectionTolerance, Descriptor)) return nullptr;

	RemoveInvalidPaintUVCaches();

	const FPaintUVCacheKey Key = MakePaintUVCacheKey(Descriptor);
	if (const TSharedPtr<FPaintUVCache>* ExistingCache = GPaintUVCacheByMesh.Find(Key))
	{
		if (ExistingCache->IsValid() && (*ExistingCache)->IsCompatibleWith(Descriptor))
		{
			return *ExistingCache;
		}

		GPaintUVCacheByMesh.Remove(Key);
	}

	TSharedPtr<FPaintUVCache> NewCache = MakeShared<FPaintUVCache>();
	if (!BuildPaintUVCache(MeshComponent, Descriptor, *NewCache)) return nullptr;

	GPaintUVCacheByMesh.Add(Key, NewCache);
	return NewCache;
}
