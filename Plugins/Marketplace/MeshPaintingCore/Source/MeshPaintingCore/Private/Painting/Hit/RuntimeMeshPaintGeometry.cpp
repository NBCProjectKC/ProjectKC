// Copyright Shared Orbit 2026. All Rights Reserved.
#include "RuntimeMeshPaintHitUtilsInternal.h"

FUVVertexKey FRuntimeMeshPaintGeometry::MakeUVVertexKey(const FVector2D& UV, float Tolerance)
{
	const double SafeTolerance = FMath::Max(static_cast<double>(Tolerance), 1.0e-6);
	const int64 X = FMath::Clamp<int64>(
		FMath::RoundToInt(UV.X / SafeTolerance),
		TNumericLimits<int32>::Min(),
		TNumericLimits<int32>::Max());
	const int64 Y = FMath::Clamp<int64>(
		FMath::RoundToInt(UV.Y / SafeTolerance),
		TNumericLimits<int32>::Min(),
		TNumericLimits<int32>::Max());
	return FUVVertexKey{
		static_cast<int32>(X),
		static_cast<int32>(Y)
	};
}

FUVEdgeKey FRuntimeMeshPaintGeometry::MakeUVEdgeKey(const FVector2D& UV0, const FVector2D& UV1, float Tolerance)
{
	FUVVertexKey A = FRuntimeMeshPaintGeometry::MakeUVVertexKey(UV0, Tolerance);
	FUVVertexKey B = FRuntimeMeshPaintGeometry::MakeUVVertexKey(UV1, Tolerance);
	if (B < A) Swap(A, B);
	return FUVEdgeKey{A, B};
}
bool FRuntimeMeshPaintGeometry::IsValidUVTriangle(const RuntimeMeshPaint::FPaintUVTriangle& Triangle)
{
	const FVector2D Edge0 = Triangle.UV1 - Triangle.UV0;
	const FVector2D Edge1 = Triangle.UV2 - Triangle.UV0;
	const double SignedArea = Edge0.X * Edge1.Y - Edge0.Y * Edge1.X;
	return FMath::Abs(SignedArea) > UE_DOUBLE_SMALL_NUMBER;
}

RuntimeMeshPaint::FPaintUVTriangle FRuntimeMeshPaintGeometry::MakePaintUVTriangle(
	int32 TriangleIndex, const FVector2D& UV0, const FVector2D& UV1, const FVector2D& UV2)
{
	RuntimeMeshPaint::FPaintUVTriangle Triangle;
	Triangle.TriangleIndex = TriangleIndex;
	Triangle.UV0 = UV0;
	Triangle.UV1 = UV1;
	Triangle.UV2 = UV2;
	return Triangle;
}

bool FRuntimeMeshPaintGeometry::IsUVInsideTriangle(const FVector2D& UV, const RuntimeMeshPaint::FPaintUVTriangle& Triangle, float Tolerance)
{
	FVector Barycentric = FVector::ZeroVector;
	if (!FRuntimeMeshPaintGeometry::ComputeUVBarycentric(UV, Triangle.UV0, Triangle.UV1, Triangle.UV2, Barycentric)) return false;

	return Barycentric.X >= -Tolerance &&
		Barycentric.Y >= -Tolerance &&
		Barycentric.Z >= -Tolerance &&
		Barycentric.X <= 1.0f + Tolerance &&
		Barycentric.Y <= 1.0f + Tolerance &&
		Barycentric.Z <= 1.0f + Tolerance;
}

bool FRuntimeMeshPaintGeometry::ComputeUVBarycentric(
	const FVector2D& UV, const FVector2D& UV0, const FVector2D& UV1, const FVector2D& UV2,
	FVector& OutBarycentric)
{
	const double Denominator =
		((UV1.Y - UV2.Y) * (UV0.X - UV2.X)) +
		((UV2.X - UV1.X) * (UV0.Y - UV2.Y));

	if (FMath::IsNearlyZero(Denominator, SMALL_NUMBER)) return false;

	const double Weight0 =
		((UV1.Y - UV2.Y) * (UV.X - UV2.X) +
			(UV2.X - UV1.X) * (UV.Y - UV2.Y)) /
		Denominator;
	const double Weight1 =
		((UV2.Y - UV0.Y) * (UV.X - UV2.X) +
			(UV0.X - UV2.X) * (UV.Y - UV2.Y)) /
		Denominator;
	const double Weight2 = 1.0 - Weight0 - Weight1;

	if (Weight0 < -RuntimeMeshPaintUVBarycentricContainmentTolerance || Weight1 < -RuntimeMeshPaintUVBarycentricContainmentTolerance || Weight2 < -RuntimeMeshPaintUVBarycentricContainmentTolerance ||
		Weight0 > 1.0 + RuntimeMeshPaintUVBarycentricContainmentTolerance || Weight1 > 1.0 + RuntimeMeshPaintUVBarycentricContainmentTolerance || Weight2 > 1.0 + RuntimeMeshPaintUVBarycentricContainmentTolerance)
	{
		return false;
	}

	OutBarycentric = FVector(Weight0, Weight1, Weight2);
	return true;
}

void FRuntimeMeshPaintGeometry::AddTriangleEdgesToMap(
	const RuntimeMeshPaint::FPaintUVTriangle& Triangle,
	int32 TriangleArrayIndex,
	float UVConnectionTolerance,
	TMultiMap<FUVEdgeKey, int32>& EdgeToTriangles)
{
	EdgeToTriangles.Add(FRuntimeMeshPaintGeometry::MakeUVEdgeKey(Triangle.UV0, Triangle.UV1, UVConnectionTolerance), TriangleArrayIndex);
	EdgeToTriangles.Add(FRuntimeMeshPaintGeometry::MakeUVEdgeKey(Triangle.UV1, Triangle.UV2, UVConnectionTolerance), TriangleArrayIndex);
	EdgeToTriangles.Add(FRuntimeMeshPaintGeometry::MakeUVEdgeKey(Triangle.UV2, Triangle.UV0, UVConnectionTolerance), TriangleArrayIndex);
}

FVector FRuntimeMeshPaintGeometry::InterpolateTrianglePosition(const FPaintTriangleSurfaceData& Triangle, const FVector& Barycentric)
{
	return Triangle.WorldPosition0 * Barycentric.X +
		Triangle.WorldPosition1 * Barycentric.Y +
		Triangle.WorldPosition2 * Barycentric.Z;
}

bool FRuntimeMeshPaintGeometry::EstimateTriangleUnitsPerUV(const FPaintTriangleSurfaceData& Triangle, float& OutUnitsPerUV)
{
	const FVector2D UVEdge1 = Triangle.UV1 - Triangle.UV0;
	const FVector2D UVEdge2 = Triangle.UV2 - Triangle.UV0;
	const double Determinant =
		UVEdge1.X * UVEdge2.Y - UVEdge1.Y * UVEdge2.X;

	if (FMath::IsNearlyZero(Determinant, SMALL_NUMBER)) return false;

	const FVector WorldEdge1 = Triangle.WorldPosition1 - Triangle.WorldPosition0;
	const FVector WorldEdge2 = Triangle.WorldPosition2 - Triangle.WorldPosition0;
	const FVector WorldDU = (WorldEdge1 * UVEdge2.Y - WorldEdge2 * UVEdge1.Y) / Determinant;
	const FVector WorldDV = (-WorldEdge1 * UVEdge2.X + WorldEdge2 * UVEdge1.X) / Determinant;
	const double AreaScale = FVector::CrossProduct(WorldDU, WorldDV).Size();
	if (AreaScale <= SMALL_NUMBER) return false;

	OutUnitsPerUV = static_cast<float>(FMath::Sqrt(AreaScale));
	return OutUnitsPerUV > KINDA_SMALL_NUMBER;
}

bool FRuntimeMeshPaintGeometry::EstimateTriangleBrushWorldRadius(const FPaintTriangleSurfaceData& Triangle, float BrushRadius, float& OutWorldRadius)
{
	float UnitsPerUV = 0.0f;
	if (!FRuntimeMeshPaintGeometry::EstimateTriangleUnitsPerUV(Triangle, UnitsPerUV)) return false;

	OutWorldRadius = FMath::Max(0.0f, BrushRadius) * UnitsPerUV;
	return OutWorldRadius > KINDA_SMALL_NUMBER;
}

void FRuntimeMeshPaintGeometry::AccumulateTriangleLocalUVAreaScale(
	const FVector& Position0,
	const FVector& Position1,
	const FVector& Position2,
	const FVector2D& UV0,
	const FVector2D& UV1,
	const FVector2D& UV2,
	double& InOutLocalArea,
	double& InOutUVArea)
{
	const FVector2D UVEdge1 = UV1 - UV0;
	const FVector2D UVEdge2 = UV2 - UV0;
	const double UVArea = FMath::Abs(
		UVEdge1.X * UVEdge2.Y - UVEdge1.Y * UVEdge2.X) * 0.5;
	if (UVArea <= UE_DOUBLE_SMALL_NUMBER) return;

	const double LocalArea = FVector::CrossProduct(Position1 - Position0, Position2 - Position0).Size() * 0.5;
	if (LocalArea <= UE_DOUBLE_SMALL_NUMBER) return;

	InOutLocalArea += LocalArea;
	InOutUVArea += UVArea;
}

float FRuntimeMeshPaintGeometry::MakeAverageUnitsPerUV(double LocalArea, double UVArea)
{
	if (LocalArea <= UE_DOUBLE_SMALL_NUMBER || UVArea <= UE_DOUBLE_SMALL_NUMBER) return 0.0f;
	return static_cast<float>(FMath::Sqrt(LocalArea / UVArea));
}
