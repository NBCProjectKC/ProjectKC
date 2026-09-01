// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "MeshPaintingCoreTypes.h"
#include "UObject/Interface.h"
#include "RuntimeMeshPaintTargetInterface.generated.h"

UINTERFACE(BlueprintType)
class MESHPAINTINGCORE_API URuntimeMeshPaintTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class MESHPAINTINGCORE_API IRuntimeMeshPaintTargetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Runtime Mesh Painting")
	void ApplyBrushMaterialSettings(const FMeshPaintBrushMaterialSettings& Settings);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Runtime Mesh Painting")
	bool SamplePaintedSurfaceColor(const FHitResult& HitResult, FRuntimeMeshPaintSampleResult& OutSampleResult);
};
