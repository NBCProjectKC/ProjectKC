// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "RuntimeMeshPaintHitUtils.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "RawIndexBuffer.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "StaticMeshResources.h"
#include "UObject/ObjectKey.h"

inline constexpr float RuntimeMeshPaintUVBarycentricContainmentTolerance = 1.0e-4f;
inline constexpr float RuntimeMeshPaintUVIslandContainmentTolerance = 1.0e-4f;
inline constexpr int32 RuntimeMeshPaintCacheLODIndex = 0;
inline constexpr int32 RuntimeMeshPaintUVGridTriangleThreshold = 1024;
inline constexpr int32 RuntimeMeshPaintUVGridMediumTriangleThreshold = 8192;
inline constexpr int32 RuntimeMeshPaintUVGridLargeTriangleThreshold = 65536;
inline constexpr int32 RuntimeMeshPaintUVGridMediumResolution = 32;
inline constexpr int32 RuntimeMeshPaintUVGridLargeResolution = 64;
inline constexpr int32 RuntimeMeshPaintUVGridMaxResolution = 128;
inline constexpr float RuntimeMeshPaintUVToleranceHashScale = 10000000.0f;

struct FPaintTriangleSurfaceData
{
	FVector WorldPosition0 = FVector::ZeroVector;
	FVector WorldPosition1 = FVector::ZeroVector;
	FVector WorldPosition2 = FVector::ZeroVector;
	FVector2D UV0 = FVector2D::ZeroVector;
	FVector2D UV1 = FVector2D::ZeroVector;
	FVector2D UV2 = FVector2D::ZeroVector;
	FVector WorldNormal = FVector::UpVector;
};

enum class EPaintUVCacheMeshType : uint8
{
	StaticMesh,
	SkeletalMesh
};

struct FPaintUVCacheDescriptor
{
	FObjectKey MeshAssetKey;
	TWeakObjectPtr<UObject> MeshAsset;
	EPaintUVCacheMeshType MeshType = EPaintUVCacheMeshType::StaticMesh;
	int32 LODIndex = RuntimeMeshPaintCacheLODIndex;
	int32 UVChannel = 0;
	int32 UVConnectionToleranceKey = 0;
	int32 IndexCount = 0;
	int32 TriangleCount = 0;
	int32 UVVertexCount = 0;
	int32 NumTexCoords = 0;
	const void* RenderDataPointer = nullptr;
	const void* LODDataPointer = nullptr;
};

struct FPaintUVCacheKey
{
	FObjectKey MeshAssetKey;
	EPaintUVCacheMeshType MeshType = EPaintUVCacheMeshType::StaticMesh;
	int32 LODIndex = RuntimeMeshPaintCacheLODIndex;
	int32 UVChannel = 0;
	int32 UVConnectionToleranceKey = 0;

	bool operator==(const FPaintUVCacheKey& Other) const
	{
		return MeshAssetKey == Other.MeshAssetKey &&
			MeshType == Other.MeshType &&
			LODIndex == Other.LODIndex &&
			UVChannel == Other.UVChannel &&
			UVConnectionToleranceKey == Other.UVConnectionToleranceKey;
	}
};

inline uint32 GetTypeHash(const FPaintUVCacheKey& Key)
{
	uint32 Hash = ::GetTypeHash(Key.MeshAssetKey.GetWeakObjectPtr());
	Hash = HashCombineFast(Hash, ::GetTypeHash(static_cast<uint8>(Key.MeshType)));
	Hash = HashCombineFast(Hash, ::GetTypeHash(Key.LODIndex));
	Hash = HashCombineFast(Hash, ::GetTypeHash(Key.UVChannel));
	Hash = HashCombineFast(Hash, ::GetTypeHash(Key.UVConnectionToleranceKey));
	return Hash;
}

struct FPaintUVIsland
{
	TArray<int32> TriangleArrayIndices;
	FVector2D BoundsMin = FVector2D(TNumericLimits<double>::Max(), TNumericLimits<double>::Max());
	FVector2D BoundsMax = FVector2D(TNumericLimits<double>::Lowest(), TNumericLimits<double>::Lowest());
};

struct FPaintUVTriangleBounds
{
	FVector2D Min = FVector2D::ZeroVector;
	FVector2D Max = FVector2D::ZeroVector;
};

struct FPaintUVGrid
{
	int32 Resolution = 0;
	TArray<int32> CellOffsets;
	TArray<int32> TriangleArrayIndices;

	bool IsValid() const
	{
		return Resolution > 0 && CellOffsets.Num() == Resolution * Resolution + 1;
	}
};

struct FPaintUVCache
{
	TWeakObjectPtr<UObject> MeshAsset;
	EPaintUVCacheMeshType MeshType = EPaintUVCacheMeshType::StaticMesh;
	int32 LODIndex = RuntimeMeshPaintCacheLODIndex;
	int32 UVChannel = 0;
	int32 UVConnectionToleranceKey = 0;
	int32 IndexCount = 0;
	int32 TriangleCount = 0;
	int32 UVVertexCount = 0;
	int32 NumTexCoords = 0;
	const void* RenderDataPointer = nullptr;
	const void* LODDataPointer = nullptr;
	float UVConnectionTolerance = 0.0f;
	float AverageLocalUnitsPerUV = 0.0f;
	TArray<RuntimeMeshPaint::FPaintUVTriangle> Triangles;
	TArray<FPaintUVTriangleBounds> TriangleBounds;
	TArray<int32> TriangleSectionIds;
	TArray<FIntVector> TriangleVertexIndices;
	TArray<int32> TriangleArraySectionIds;
	TArray<int32> FaceIndexToTriangleArrayIndex;
	TArray<int32> TriangleIslandIds;
	TArray<FPaintUVIsland> Islands;
	FPaintUVGrid UVGrid;
	mutable TArray<uint32> SkeletalVertexSkinMarks;
	mutable TArray<FVector3f> SkeletalVertexSkinPositions;
	mutable uint32 SkeletalVertexSkinSerial = 1;

	bool IsCompatibleWith(const FPaintUVCacheDescriptor& Descriptor) const
	{
		return MeshAsset.IsValid() &&
			FObjectKey(MeshAsset.Get()) == Descriptor.MeshAssetKey &&
			MeshType == Descriptor.MeshType &&
			LODIndex == Descriptor.LODIndex &&
			UVChannel == Descriptor.UVChannel &&
			UVConnectionToleranceKey == Descriptor.UVConnectionToleranceKey &&
			IndexCount == Descriptor.IndexCount &&
			TriangleCount == Descriptor.TriangleCount &&
			UVVertexCount == Descriptor.UVVertexCount &&
			NumTexCoords == Descriptor.NumTexCoords &&
			RenderDataPointer == Descriptor.RenderDataPointer &&
			LODDataPointer == Descriptor.LODDataPointer;
	}
};

struct FSkeletalTriangleDistanceSearchResult
{
	float DistanceSq = TNumericLimits<float>::Max();
	int32 TriangleIndex = INDEX_NONE;

	bool IsBetterThan(const FSkeletalTriangleDistanceSearchResult& Other) const
	{
		return DistanceSq < Other.DistanceSq ||
			(FMath::IsNearlyEqual(DistanceSq, Other.DistanceSq) && TriangleIndex != INDEX_NONE &&
				(Other.TriangleIndex == INDEX_NONE || TriangleIndex < Other.TriangleIndex));
	}
};

struct FSkeletalMeshPaintVisualHit
{
	FVector2D UV = FVector2D::ZeroVector;
	FVector LocalPosition = FVector::ZeroVector;
	FVector LocalNormal = FVector::UpVector;
	FVector WorldPosition = FVector::ZeroVector;
	FVector WorldNormal = FVector::UpVector;
	int32 FaceIndex = INDEX_NONE;
	int32 TriangleArrayIndex = INDEX_NONE;
	bool bRayIntersection = false;
};

struct FUVVertexKey
{
	int32 X = 0;
	int32 Y = 0;

	bool operator==(const FUVVertexKey& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}

	bool operator<(const FUVVertexKey& Other) const
	{
		return X == Other.X ? Y < Other.Y : X < Other.X;
	}
};

struct FUVEdgeKey
{
	FUVVertexKey A;
	FUVVertexKey B;

	bool operator==(const FUVEdgeKey& Other) const
	{
		return A == Other.A && B == Other.B;
	}
};

inline uint32 GetTypeHash(const FUVVertexKey& Key)
{
	return HashCombine(::GetTypeHash(Key.X), ::GetTypeHash(Key.Y));
}

inline uint32 GetTypeHash(const FUVEdgeKey& Key)
{
	return HashCombine(GetTypeHash(Key.A), GetTypeHash(Key.B));
}

struct FRuntimeMeshPaintGeometry
{
	static FUVVertexKey MakeUVVertexKey(const FVector2D& UV, float Tolerance);
	static FUVEdgeKey MakeUVEdgeKey(const FVector2D& UV0, const FVector2D& UV1, float Tolerance);
	static bool ComputeUVBarycentric(const FVector2D& UV, const FVector2D& UV0, const FVector2D& UV1, const FVector2D& UV2, FVector& OutBarycentric);
	static bool IsValidUVTriangle(const RuntimeMeshPaint::FPaintUVTriangle& Triangle);
	static RuntimeMeshPaint::FPaintUVTriangle MakePaintUVTriangle(int32 TriangleIndex, const FVector2D& UV0, const FVector2D& UV1, const FVector2D& UV2);
	static bool IsUVInsideTriangle(const FVector2D& UV, const RuntimeMeshPaint::FPaintUVTriangle& Triangle, float Tolerance);
	static void AddTriangleEdgesToMap(const RuntimeMeshPaint::FPaintUVTriangle& Triangle, int32 TriangleArrayIndex, float UVConnectionTolerance, TMultiMap<FUVEdgeKey, int32>& EdgeToTriangles);
	static FVector InterpolateTrianglePosition(const FPaintTriangleSurfaceData& Triangle, const FVector& Barycentric);
	static bool EstimateTriangleUnitsPerUV(const FPaintTriangleSurfaceData& Triangle, float& OutUnitsPerUV);
	static bool EstimateTriangleBrushWorldRadius(const FPaintTriangleSurfaceData& Triangle, float BrushRadius, float& OutWorldRadius);
	static void AccumulateTriangleLocalUVAreaScale(const FVector& Position0, const FVector& Position1, const FVector& Position2, const FVector2D& UV0, const FVector2D& UV1, const FVector2D& UV2, double& InOutLocalArea, double& InOutUVArea);
	static float MakeAverageUnitsPerUV(double LocalArea, double UVArea);
};

struct FRuntimeMeshPaintStaticMesh
{
	static bool GetStaticTriangleData(const FStaticMeshLODResources& LODResources, const FTransform& ComponentTransform, int32 TriangleIndex, int32 UVChannel, FPaintTriangleSurfaceData& OutTriangle);
	static bool CollectStaticMeshUVTriangles(const UStaticMeshComponent* StaticMeshComponent, int32 UVChannel, TArray<RuntimeMeshPaint::FPaintUVTriangle>& OutTriangles, float* OutAverageLocalUnitsPerUV);
	static void BuildStaticMeshTriangleSectionIds(const UStaticMeshComponent* StaticMeshComponent, int32 TriangleCount, TArray<int32>& OutTriangleSectionIds);
	static bool BuildStaticMeshUVCacheDescriptor(const UStaticMeshComponent* StaticMeshComponent, int32 UVChannel, float UVConnectionTolerance, FPaintUVCacheDescriptor& OutDescriptor);
	static bool EstimateStaticBrushWorldRadius(const FHitResult& HitResult, int32 UVChannel, float BrushRadius, float& OutWorldRadius);
	static bool FindStaticMeshFaceUV(const FHitResult& HitResult, int32 UVChannel, FVector2D& OutUV, int32* OutResolvedFaceIndex);
};

struct FRuntimeMeshPaintSkeletalMesh
{
	static bool CollectSkeletalMeshUVTriangles(const USkeletalMeshComponent* SkeletalMeshComponent, int32 UVChannel, TArray<RuntimeMeshPaint::FPaintUVTriangle>& OutTriangles, TArray<FIntVector>* OutTriangleVertexIndices, TArray<int32>* OutTriangleArraySectionIds, float* OutAverageLocalUnitsPerUV);
	static void BuildSkeletalMeshTriangleSectionIds(const USkeletalMeshComponent* SkeletalMeshComponent, int32 TriangleCount, TArray<int32>& OutTriangleSectionIds);
	static bool BuildSkeletalMeshUVCacheDescriptor(const USkeletalMeshComponent* SkeletalMeshComponent, int32 UVChannel, float UVConnectionTolerance, FPaintUVCacheDescriptor& OutDescriptor);
	static bool GetSkeletalMeshTriangleVertexIndices(const FSkeletalMeshLODRenderData& LODData, const FRawStaticIndexBuffer16or32Interface& IndexBuffer, int32 TriangleIndex, FIntVector& OutVertexIndices);
	static bool SkinSkeletalTrianglePositions(USkeletalMeshComponent* SkeletalMeshComponent, const FSkeletalMeshLODRenderData& LODData, const FSkinWeightVertexBuffer& SkinWeightBuffer, const FIntVector& VertexIndices, TArray<FMatrix44f>& CachedRefToLocals, FVector& OutPosition0, FVector& OutPosition1, FVector& OutPosition2);
	static bool FindSkeletalMeshFaceUV(
		const FHitResult& HitResult, int32 UVChannel, float MaxFallbackDistance,
		FVector2D& OutUV, int32* OutResolvedFaceIndex, int32* OutResolvedTriangleArrayIndex = nullptr);
	static bool ResolveSkeletalMeshVisualHit(
		const FHitResult& HitResult, int32 UVChannel, float MaxFallbackDistance,
		bool bRequireRayIntersection, FSkeletalMeshPaintVisualHit& OutVisualHit);
};

struct FRuntimeMeshPaintUVCache
{
	static int32 MakeUVConnectionToleranceKey(float UVConnectionTolerance);
	static float MakeUVConnectionToleranceFromKey(int32 UVConnectionToleranceKey);
	static bool BuildPaintUVCacheDescriptor(UMeshComponent* MeshComponent, int32 UVChannel, float UVConnectionTolerance, FPaintUVCacheDescriptor& OutDescriptor);
	static TSharedPtr<FPaintUVCache> FindOrBuildPaintUVCache(UMeshComponent* MeshComponent, int32 UVChannel, float UVConnectionTolerance);
	static bool FindSeedTriangleArrayIndex(const FPaintUVCache& Cache, int32 FaceIndex, const FVector2D& HitUV, int32& OutSeedArrayIndex);
};

struct FRuntimeMeshPaintSurface
{
	static bool EstimateStaticHitTriangleUnitsPerUV(const FHitResult& HitResult, int32 UVChannel, const FVector2D& HitUV, float& OutHitLocalUnitsPerUV, float& OutHitWorldUnitsPerUV, float& OutAverageLocalUnitsPerUV);
	static bool BuildSkeletalTriangleDataLazy(USkeletalMeshComponent* SkeletalMeshComponent, const FSkeletalMeshLODRenderData& LODData, const FSkinWeightVertexBuffer& SkinWeightBuffer, const FRawStaticIndexBuffer16or32Interface& IndexBuffer, int32 TriangleIndex, int32 UVChannel, TArray<FMatrix44f>& CachedRefToLocals, FPaintTriangleSurfaceData& OutLocalTriangle, FPaintTriangleSurfaceData& OutWorldTriangle);
	static bool EstimateSkeletalHitTriangleUnitsPerUV(const FHitResult& HitResult, int32 UVChannel, const FVector2D& HitUV, float& OutHitLocalUnitsPerUV, float& OutHitWorldUnitsPerUV, float& OutAverageLocalUnitsPerUV);
	static bool EstimatePaintHitTriangleUnitsPerUV(const FHitResult& HitResult, int32 UVChannel, const FVector2D& HitUV, float& OutHitLocalUnitsPerUV, float& OutHitWorldUnitsPerUV, float& OutAverageLocalUnitsPerUV);
	static bool ResolveStaticPaintHitSurfaceData(const FHitResult& HitResult, int32 UVChannel, const FVector2D& HitUV, FVector& OutWorldPosition, FVector& OutWorldNormal);
	static bool ResolveSkeletalPaintHitSurfaceData(const FHitResult& HitResult, int32 UVChannel, const FVector2D& HitUV, FVector& OutWorldPosition, FVector& OutWorldNormal);
};
