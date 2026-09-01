// Copyright Shared Orbit 2026. All Rights Reserved.
#include "RuntimeMeshPaintHitUtilsInternal.h"

#include "../../Core/MeshPaintingCoreStats.h"

bool FRuntimeMeshPaintStaticMesh::GetStaticTriangleData(
	const FStaticMeshLODResources& LODResources, const FTransform& ComponentTransform,
	int32 TriangleIndex, int32 UVChannel, FPaintTriangleSurfaceData& OutTriangle)
{
	if (TriangleIndex < 0 || UVChannel < 0 ||
		static_cast<uint32>(UVChannel) >= LODResources.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords())
	{
		return false;
	}

	const FIndexArrayView IndexBuffer = LODResources.IndexBuffer.GetArrayView();
	const int32 FirstIndex = TriangleIndex * 3;
	if (FirstIndex < 0 || FirstIndex + 2 >= IndexBuffer.Num()) return false;

	const uint32 Index0 = IndexBuffer[FirstIndex];
	const uint32 Index1 = IndexBuffer[FirstIndex + 1];
	const uint32 Index2 = IndexBuffer[FirstIndex + 2];
	const uint32 NumPositionVertices = LODResources.VertexBuffers.PositionVertexBuffer.GetNumVertices();
	const uint32 NumUVVertices = LODResources.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices();
	if (Index0 >= NumPositionVertices || Index1 >= NumPositionVertices || Index2 >= NumPositionVertices ||
		Index0 >= NumUVVertices || Index1 >= NumUVVertices || Index2 >= NumUVVertices)
	{
		return false;
	}

	OutTriangle.WorldPosition0 = ComponentTransform.TransformPosition(FVector(LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(Index0)));
	OutTriangle.WorldPosition1 = ComponentTransform.TransformPosition(FVector(LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(Index1)));
	OutTriangle.WorldPosition2 = ComponentTransform.TransformPosition(FVector(LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(Index2)));

	const FVector2f UV0 = LODResources.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index0, UVChannel);
	const FVector2f UV1 = LODResources.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index1, UVChannel);
	const FVector2f UV2 = LODResources.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index2, UVChannel);
	OutTriangle.UV0 = FVector2D(UV0.X, UV0.Y);
	OutTriangle.UV1 = FVector2D(UV1.X, UV1.Y);
	OutTriangle.UV2 = FVector2D(UV2.X, UV2.Y);
	OutTriangle.WorldNormal = FVector::CrossProduct(
		OutTriangle.WorldPosition1 - OutTriangle.WorldPosition0,
		OutTriangle.WorldPosition2 - OutTriangle.WorldPosition0).GetSafeNormal();
	return !OutTriangle.WorldNormal.IsNearlyZero();
}

bool FRuntimeMeshPaintStaticMesh::CollectStaticMeshUVTriangles(
	const UStaticMeshComponent* StaticMeshComponent,
	int32 UVChannel,
	TArray<RuntimeMeshPaint::FPaintUVTriangle>& OutTriangles,
	float* OutAverageLocalUnitsPerUV = nullptr)
{
	SCOPE_CYCLE_COUNTER(STAT_MeshPaintingCore_CollectStaticMeshUVTriangles);

	OutTriangles.Reset();
	if (OutAverageLocalUnitsPerUV) *OutAverageLocalUnitsPerUV = 0.0f;

	const UStaticMesh* StaticMesh = StaticMeshComponent ? StaticMeshComponent->GetStaticMesh() : nullptr;
	const FStaticMeshRenderData* RenderData = StaticMesh ? StaticMesh->GetRenderData() : nullptr;
	if (!RenderData || RenderData->LODResources.Num() == 0 || UVChannel < 0) return false;

	const FStaticMeshLODResources& LODResources = RenderData->LODResources[0];
	if (static_cast<uint32>(UVChannel) >= LODResources.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords()) return false;

	const FIndexArrayView IndexBuffer = LODResources.IndexBuffer.GetArrayView();
	const uint32 NumPositionVertices = LODResources.VertexBuffers.PositionVertexBuffer.GetNumVertices();
	const uint32 NumUVVertices = LODResources.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices();
	const int32 TriangleCount = IndexBuffer.Num() / 3;
	INC_DWORD_STAT_BY(STAT_MeshPaintingCore_UVIslandTrianglesScanned, TriangleCount);
	OutTriangles.Reserve(TriangleCount);
	double LocalAreaSum = 0.0;
	double UVAreaSum = 0.0;

	for (int32 TriangleIndex = 0; TriangleIndex < TriangleCount; ++TriangleIndex)
	{
		const int32 FirstIndex = TriangleIndex * 3;
		const uint32 Index0 = IndexBuffer[FirstIndex];
		const uint32 Index1 = IndexBuffer[FirstIndex + 1];
		const uint32 Index2 = IndexBuffer[FirstIndex + 2];
		if (Index0 >= NumUVVertices || Index1 >= NumUVVertices || Index2 >= NumUVVertices ||
			Index0 >= NumPositionVertices || Index1 >= NumPositionVertices || Index2 >= NumPositionVertices)
		{
			continue;
		}

		const FVector2f UV0 = LODResources.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index0, UVChannel);
		const FVector2f UV1 = LODResources.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index1, UVChannel);
		const FVector2f UV2 = LODResources.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index2, UVChannel);
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
			FVector(LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(Index0)),
			FVector(LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(Index1)),
			FVector(LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(Index2)),
			UV0D,
			UV1D,
			UV2D,
			LocalAreaSum,
			UVAreaSum);
	}

	if (OutAverageLocalUnitsPerUV) *OutAverageLocalUnitsPerUV = FRuntimeMeshPaintGeometry::MakeAverageUnitsPerUV(LocalAreaSum, UVAreaSum);
	return OutTriangles.Num() > 0;
}

bool FRuntimeMeshPaintStaticMesh::EstimateStaticBrushWorldRadius(const FHitResult& HitResult, int32 UVChannel, float BrushRadius, float& OutWorldRadius)
{
	const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(HitResult.GetComponent());
	const UStaticMesh* StaticMesh = StaticMeshComponent ? StaticMeshComponent->GetStaticMesh() : nullptr;
	const FStaticMeshRenderData* RenderData = StaticMesh ? StaticMesh->GetRenderData() : nullptr;
	if (!RenderData || RenderData->LODResources.Num() == 0 || HitResult.FaceIndex == INDEX_NONE) return false;

	FPaintTriangleSurfaceData Triangle;
	if (!FRuntimeMeshPaintStaticMesh::GetStaticTriangleData(RenderData->LODResources[0], StaticMeshComponent->GetComponentTransform(), HitResult.FaceIndex, UVChannel, Triangle))
		return false;

	return FRuntimeMeshPaintGeometry::EstimateTriangleBrushWorldRadius(Triangle, BrushRadius, OutWorldRadius);
}
void FRuntimeMeshPaintStaticMesh::BuildStaticMeshTriangleSectionIds(
	const UStaticMeshComponent* StaticMeshComponent, int32 TriangleCount, TArray<int32>& OutTriangleSectionIds)
{
	OutTriangleSectionIds.Init(INDEX_NONE, TriangleCount);

	const UStaticMesh* StaticMesh = StaticMeshComponent ? StaticMeshComponent->GetStaticMesh() : nullptr;
	const FStaticMeshRenderData* RenderData = StaticMesh ? StaticMesh->GetRenderData() : nullptr;
	if (!RenderData || RenderData->LODResources.Num() <= RuntimeMeshPaintCacheLODIndex) return;

	const FStaticMeshLODResources& LODResources = RenderData->LODResources[RuntimeMeshPaintCacheLODIndex];
	for (int32 SectionIndex = 0; SectionIndex < LODResources.Sections.Num(); ++SectionIndex)
	{
		const FStaticMeshSection& Section = LODResources.Sections[SectionIndex];
		const int32 FirstTriangleIndex = Section.FirstIndex / 3;
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
bool FRuntimeMeshPaintStaticMesh::BuildStaticMeshUVCacheDescriptor(
	const UStaticMeshComponent* StaticMeshComponent, int32 UVChannel, float UVConnectionTolerance,
	FPaintUVCacheDescriptor& OutDescriptor)
{
	UStaticMesh* StaticMesh = StaticMeshComponent ? StaticMeshComponent->GetStaticMesh() : nullptr;
	const FStaticMeshRenderData* RenderData = StaticMesh ? StaticMesh->GetRenderData() : nullptr;
	if (!RenderData || RenderData->LODResources.Num() <= RuntimeMeshPaintCacheLODIndex || UVChannel < 0) return false;

	const FStaticMeshLODResources& LODResources = RenderData->LODResources[RuntimeMeshPaintCacheLODIndex];
	const int32 NumTexCoords = static_cast<int32>(LODResources.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords());
	if (UVChannel >= NumTexCoords) return false;

	const FIndexArrayView IndexBuffer = LODResources.IndexBuffer.GetArrayView();
	if (IndexBuffer.Num() < 3) return false;

	OutDescriptor = FPaintUVCacheDescriptor();
	OutDescriptor.MeshAssetKey = FObjectKey(StaticMesh);
	OutDescriptor.MeshAsset = StaticMesh;
	OutDescriptor.MeshType = EPaintUVCacheMeshType::StaticMesh;
	OutDescriptor.LODIndex = RuntimeMeshPaintCacheLODIndex;
	OutDescriptor.UVChannel = UVChannel;
	OutDescriptor.UVConnectionToleranceKey = FRuntimeMeshPaintUVCache::MakeUVConnectionToleranceKey(UVConnectionTolerance);
	OutDescriptor.IndexCount = IndexBuffer.Num();
	OutDescriptor.TriangleCount = IndexBuffer.Num() / 3;
	OutDescriptor.UVVertexCount = static_cast<int32>(LODResources.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices());
	OutDescriptor.NumTexCoords = NumTexCoords;
	OutDescriptor.RenderDataPointer = RenderData;
	OutDescriptor.LODDataPointer = &LODResources;
	return OutDescriptor.MeshAsset.IsValid() && OutDescriptor.TriangleCount > 0;
}
static bool GetPaintTriangleRayUV(
	const FPaintTriangleSurfaceData& Triangle,
	const FVector& TraceStart,
	const FVector& TraceEnd,
	FVector2D& OutUV,
	float* OutDistanceSq = nullptr)
{
	if (FVector::DistSquared(TraceStart, TraceEnd) <= UE_DOUBLE_SMALL_NUMBER) return false;

	FVector IntersectionPoint = FVector::ZeroVector;
	FVector TriangleNormal = FVector::ZeroVector;
	if (!FMath::SegmentTriangleIntersection(
		TraceStart,
		TraceEnd,
		Triangle.WorldPosition0,
		Triangle.WorldPosition1,
		Triangle.WorldPosition2,
		IntersectionPoint,
		TriangleNormal))
	{
		return false;
	}

	const FVector Barycentric = FMath::ComputeBaryCentric2D(
		IntersectionPoint,
		Triangle.WorldPosition0,
		Triangle.WorldPosition1,
		Triangle.WorldPosition2);
	OutUV =
		(Triangle.UV0 * Barycentric.X) +
		(Triangle.UV1 * Barycentric.Y) +
		(Triangle.UV2 * Barycentric.Z);
	if (OutDistanceSq) *OutDistanceSq = static_cast<float>(FVector::DistSquared(TraceStart, IntersectionPoint));
	return true;
}

static bool GetPaintTriangleClosestUV(
	const FPaintTriangleSurfaceData& Triangle,
	const FVector& ReferencePosition,
	float CurrentBestDistanceSq,
	FVector2D& OutUV,
	float& OutDistanceSq)
{
	const FVector ClosestPoint = FMath::ClosestPointOnTriangleToPoint(
		ReferencePosition,
		Triangle.WorldPosition0,
		Triangle.WorldPosition1,
		Triangle.WorldPosition2);
	OutDistanceSq = static_cast<float>(FVector::DistSquared(ReferencePosition, ClosestPoint));
	if (OutDistanceSq > CurrentBestDistanceSq) return false;

	const FVector Barycentric = FMath::ComputeBaryCentric2D(
		ClosestPoint,
		Triangle.WorldPosition0,
		Triangle.WorldPosition1,
		Triangle.WorldPosition2);
	OutUV =
		(Triangle.UV0 * Barycentric.X) +
		(Triangle.UV1 * Barycentric.Y) +
		(Triangle.UV2 * Barycentric.Z);
	return true;
}

static bool FindStaticMeshRayOrClosestUV(
	const FStaticMeshLODResources& LODResources,
	const FTransform& ComponentTransform,
	const FHitResult& HitResult,
	int32 UVChannel,
	FVector2D& OutUV,
	int32* OutResolvedFaceIndex = nullptr)
{
	const FIndexArrayView IndexBuffer = LODResources.IndexBuffer.GetArrayView();
	const int32 TriangleCount = IndexBuffer.Num() / 3;
	if (TriangleCount <= 0) return false;

	FSkeletalTriangleDistanceSearchResult BestRayTriangle;
	FSkeletalTriangleDistanceSearchResult BestClosestTriangle;
	FVector2D BestRayUV = FVector2D::ZeroVector;
	FVector2D BestClosestUV = FVector2D::ZeroVector;

	for (int32 TriangleIndex = 0; TriangleIndex < TriangleCount; ++TriangleIndex)
	{
		FPaintTriangleSurfaceData Triangle;
		if (!FRuntimeMeshPaintStaticMesh::GetStaticTriangleData(LODResources, ComponentTransform, TriangleIndex, UVChannel, Triangle)) continue;

		float DistanceSq = 0.0f;
		FVector2D CandidateUV = FVector2D::ZeroVector;
		if (GetPaintTriangleRayUV(Triangle, HitResult.TraceStart, HitResult.TraceEnd, CandidateUV, &DistanceSq))
		{
			const FSkeletalTriangleDistanceSearchResult Candidate{DistanceSq, TriangleIndex};
			if (Candidate.IsBetterThan(BestRayTriangle))
			{
				BestRayTriangle = Candidate;
				BestRayUV = CandidateUV;
			}
		}

		if (GetPaintTriangleClosestUV(
			Triangle,
			HitResult.ImpactPoint,
			BestClosestTriangle.DistanceSq,
			CandidateUV,
			DistanceSq))
		{
			const FSkeletalTriangleDistanceSearchResult Candidate{DistanceSq, TriangleIndex};
			if (Candidate.IsBetterThan(BestClosestTriangle))
			{
				BestClosestTriangle = Candidate;
				BestClosestUV = CandidateUV;
			}
		}
	}

	if (BestRayTriangle.TriangleIndex != INDEX_NONE)
	{
		OutUV = BestRayUV;
		if (OutResolvedFaceIndex) *OutResolvedFaceIndex = BestRayTriangle.TriangleIndex;
		return true;
	}

	if (BestClosestTriangle.TriangleIndex != INDEX_NONE)
	{
		OutUV = BestClosestUV;
		if (OutResolvedFaceIndex) *OutResolvedFaceIndex = BestClosestTriangle.TriangleIndex;
		return true;
	}

	return false;
}

bool FRuntimeMeshPaintStaticMesh::FindStaticMeshFaceUV(const FHitResult& HitResult, int32 UVChannel, FVector2D& OutUV, int32* OutResolvedFaceIndex = nullptr)
{
	if (UVChannel < 0) return false;

	const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(HitResult.GetComponent());
	const UStaticMesh* StaticMesh = StaticMeshComponent ? StaticMeshComponent->GetStaticMesh() : nullptr;
	const FStaticMeshRenderData* RenderData = StaticMesh ? StaticMesh->GetRenderData() : nullptr;
	if (!RenderData || RenderData->LODResources.Num() == 0) return false;

	const FStaticMeshLODResources& LODResources = RenderData->LODResources[0];
	if (static_cast<uint32>(UVChannel) >= LODResources.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords()) return false;

	if (HitResult.FaceIndex != INDEX_NONE)
	{
		const FIndexArrayView IndexBuffer = LODResources.IndexBuffer.GetArrayView();
		const int32 FirstIndex = HitResult.FaceIndex * 3;
		if (FirstIndex >= 0 && FirstIndex + 2 < IndexBuffer.Num())
		{
			const uint32 Index0 = IndexBuffer[FirstIndex];
			const uint32 Index1 = IndexBuffer[FirstIndex + 1];
			const uint32 Index2 = IndexBuffer[FirstIndex + 2];
			const uint32 NumPositionVertices = LODResources.VertexBuffers.PositionVertexBuffer.GetNumVertices();
			const uint32 NumUVVertices = LODResources.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices();
			if (Index0 < NumPositionVertices && Index1 < NumPositionVertices && Index2 < NumPositionVertices &&
				Index0 < NumUVVertices && Index1 < NumUVVertices && Index2 < NumUVVertices)
			{
				const FVector LocalHitPosition = StaticMeshComponent->GetComponentTransform().InverseTransformPosition(HitResult.ImpactPoint);
				const FVector Position0 = FVector(LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(Index0));
				const FVector Position1 = FVector(LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(Index1));
				const FVector Position2 = FVector(LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(Index2));
				const FVector Barycentric = FMath::ComputeBaryCentric2D(LocalHitPosition, Position0, Position1, Position2);

				const FVector2f UV0 = LODResources.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index0, UVChannel);
				const FVector2f UV1 = LODResources.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index1, UVChannel);
				const FVector2f UV2 = LODResources.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index2, UVChannel);
				const FVector2f UV =
					(UV0 * static_cast<float>(Barycentric.X)) +
					(UV1 * static_cast<float>(Barycentric.Y)) +
					(UV2 * static_cast<float>(Barycentric.Z));

				OutUV = FVector2D(UV.X, UV.Y);
				if (OutResolvedFaceIndex) *OutResolvedFaceIndex = HitResult.FaceIndex;
				return true;
			}
		}
	}

	return FindStaticMeshRayOrClosestUV(
		LODResources,
		StaticMeshComponent->GetComponentTransform(),
		HitResult,
		UVChannel,
		OutUV,
		OutResolvedFaceIndex);
}
