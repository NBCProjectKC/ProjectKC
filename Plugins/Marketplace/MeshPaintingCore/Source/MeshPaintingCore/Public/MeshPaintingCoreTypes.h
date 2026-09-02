// Copyright Shared Orbit 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "MeshPaintingCoreTypes.generated.h"

USTRUCT(BlueprintType)
struct MESHPAINTINGCORE_API FMeshPaintBrushMaterialSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Paint")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Paint", meta = (ClampMin = "0.001"))
	float BrushSize = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Paint", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Metallic = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Paint", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Roughness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Paint")
	bool bErase = false;

	void Clamp()
	{
		Color.R = FMath::Clamp(Color.R, 0.0f, 1.0f);
		Color.G = FMath::Clamp(Color.G, 0.0f, 1.0f);
		Color.B = FMath::Clamp(Color.B, 0.0f, 1.0f);
		Color.A = 1.0f;
		BrushSize = FMath::Max(0.001f, BrushSize);
		Metallic = FMath::Clamp(Metallic, 0.0f, 1.0f);
		Roughness = FMath::Clamp(Roughness, 0.0f, 1.0f);
	}
};

UENUM(BlueprintType)
enum class ERuntimeMeshPaintColorSampleMode : uint8
{
	MeshUnlitColor = 0 UMETA(DisplayName = "Mesh Unlit Color"),
	ViewportLitColor = 3 UMETA(DisplayName = "Viewport Lit Color"),
	MeshUnlitColorThenViewport = 4 UMETA(DisplayName = "Mesh Unlit Color Then Viewport")
};

struct MESHPAINTINGCORE_API FRuntimeMeshPaintScreenProjectionData
{
	bool bHasScreenProjection = false;
	FVector ViewOrigin = FVector::ZeroVector;
	FVector ViewForward = FVector::ForwardVector;
	FVector ViewRight = FVector::RightVector;
	FVector ViewUp = FVector::UpVector;
};

USTRUCT(BlueprintType)
struct MESHPAINTINGCORE_API FRuntimeMeshPaintSampleResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Paint")
	bool bSuccess = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Paint")
	FLinearColor Color = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Paint")
	FVector2D UV = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Paint")
	FHitResult HitResult;

	int32 ResolvedTriangleArrayIndex = INDEX_NONE;
	int32 ResolvedUVIslandId = INDEX_NONE;

	FRuntimeMeshPaintScreenProjectionData ProjectionData;
};

namespace RuntimeMeshPaint
{
	struct MESHPAINTINGCORE_API FPaintUVTriangle
	{
		int32 TriangleIndex = INDEX_NONE;
		FVector2D UV0 = FVector2D::ZeroVector;
		FVector2D UV1 = FVector2D::ZeroVector;
		FVector2D UV2 = FVector2D::ZeroVector;
	};
}
