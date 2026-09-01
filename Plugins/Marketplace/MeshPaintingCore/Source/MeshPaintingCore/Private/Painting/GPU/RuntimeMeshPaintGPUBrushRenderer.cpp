// Copyright Shared Orbit 2026. All Rights Reserved.
#include "RuntimeMeshPaintGPUBrushRenderer.h"

#include "Engine/TextureRenderTarget2D.h"
#include "GlobalShader.h"
#include "PipelineStateCache.h"
#include "RenderResource.h"
#include "RenderingThread.h"
#include "RHIStaticStates.h"
#include "ShaderParameterStruct.h"
#include "TextureResource.h"
#include "../Hit/RuntimeMeshPaintHitUtilsInternal.h"

namespace
{
	constexpr int32 RuntimeMeshPaintProjectedBrushMaxBones = 512;
	constexpr float RuntimeMeshPaintProjectedBrushNormalThreshold = 0.0f;
	constexpr float RuntimeMeshPaintProjectedSkeletalBrushNormalThreshold = -0.25f;
	constexpr float RuntimeMeshPaintProjectedBrushDepthRadiusScale = 1.5f;
	constexpr float RuntimeMeshPaintProjectedBrushMinDepth = 2.0f;
	constexpr float RuntimeMeshPaintProjectedPreviewNormalThreshold = 0.0f;
	constexpr float RuntimeMeshPaintProjectedSkeletalPreviewNormalThreshold = -0.25f;
	constexpr float RuntimeMeshPaintProjectedPreviewDepthRadiusScale = 5.0f;
	constexpr float RuntimeMeshPaintProjectedPreviewMinDepth = 12.0f;
	constexpr float RuntimeMeshPaintProjectedBrushSeamPaddingPixels = 2.0f;
	constexpr int32 RuntimeMeshPaintProjectedBrushVisibilitySize = 512;
	constexpr float RuntimeMeshPaintProjectedBrushVisibilityDepthTolerance = 0.025f;

	struct FRuntimeMeshPaintGPUProjectedBrushVertex
	{
		FVector2f PaintUV = FVector2f::ZeroVector;
		FVector3f LocalPosition = FVector3f::ZeroVector;
		FVector3f LocalNormal = FVector3f::UpVector;
		FVector4f BoneIndices0 = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
		FVector4f BoneWeights0 = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
		FVector4f BoneIndices1 = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
		FVector4f BoneWeights1 = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
		FVector2f PaintUVPaddingDirection = FVector2f::ZeroVector;
	};

	class FRuntimeMeshPaintGPUProjectedBrushMeshResource
	{
	public:
		TWeakObjectPtr<const UObject> SourceMeshAsset;
		EPaintUVCacheMeshType MeshType = EPaintUVCacheMeshType::StaticMesh;
		TArray<FRuntimeMeshPaintGPUProjectedBrushVertex> InitialVertices;
		FBufferRHIRef VertexBufferRHI;

		uint32 GetVertexCount() const
		{
			return VertexBufferRHI.IsValid() ? VertexCount : static_cast<uint32>(InitialVertices.Num());
		}

		uint32 VertexCount = 0;
	};

	class FRuntimeMeshPaintGPUProjectedBrushVertexDeclaration : public FRenderResource
	{
	public:
		FVertexDeclarationRHIRef VertexDeclarationRHI;

		virtual void InitRHI(FRHICommandListBase& RHICmdList) override
		{
			FVertexDeclarationElementList Elements;
			const uint16 Stride = sizeof(FRuntimeMeshPaintGPUProjectedBrushVertex);
			Elements.Add(FVertexElement(0, STRUCT_OFFSET(FRuntimeMeshPaintGPUProjectedBrushVertex, PaintUV), VET_Float2, 0, Stride));
			Elements.Add(FVertexElement(0, STRUCT_OFFSET(FRuntimeMeshPaintGPUProjectedBrushVertex, LocalPosition), VET_Float3, 1, Stride));
			Elements.Add(FVertexElement(0, STRUCT_OFFSET(FRuntimeMeshPaintGPUProjectedBrushVertex, LocalNormal), VET_Float3, 2, Stride));
			Elements.Add(FVertexElement(0, STRUCT_OFFSET(FRuntimeMeshPaintGPUProjectedBrushVertex, BoneIndices0), VET_Float4, 3, Stride));
			Elements.Add(FVertexElement(0, STRUCT_OFFSET(FRuntimeMeshPaintGPUProjectedBrushVertex, BoneWeights0), VET_Float4, 4, Stride));
			Elements.Add(FVertexElement(0, STRUCT_OFFSET(FRuntimeMeshPaintGPUProjectedBrushVertex, BoneIndices1), VET_Float4, 5, Stride));
			Elements.Add(FVertexElement(0, STRUCT_OFFSET(FRuntimeMeshPaintGPUProjectedBrushVertex, BoneWeights1), VET_Float4, 6, Stride));
			Elements.Add(FVertexElement(0, STRUCT_OFFSET(FRuntimeMeshPaintGPUProjectedBrushVertex, PaintUVPaddingDirection), VET_Float2, 7, Stride));
			VertexDeclarationRHI = PipelineStateCache::GetOrCreateVertexDeclaration(Elements);
		}

		virtual void ReleaseRHI() override
		{
			VertexDeclarationRHI.SafeRelease();
		}
	};

	TGlobalResource<FRuntimeMeshPaintGPUProjectedBrushVertexDeclaration> GRuntimeMeshPaintGPUProjectedBrushVertexDeclaration;
	TMap<const FPaintUVCache*, TSharedPtr<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe>> GRuntimeMeshPaintGPUProjectedBrushResources;
	FTextureRHIRef GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthTexture;
	FTextureRHIRef GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthStencilTexture;
	FIntPoint GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthSize = FIntPoint::ZeroValue;

	struct FRuntimeMeshPaintGPUBrushProjectionParameters
	{
		float bUseScreenProjection = 0.0f;
		FVector3f ViewOrigin = FVector3f::ZeroVector;
		FVector3f ViewDirection = FVector3f::ForwardVector;
		FVector3f ViewRight = FVector3f::RightVector;
		FVector3f ViewUp = FVector3f::UpVector;
	};

	class FRuntimeMeshPaintGPUProjectedBrushVS final : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FRuntimeMeshPaintGPUProjectedBrushVS);
		SHADER_USE_PARAMETER_STRUCT(FRuntimeMeshPaintGPUProjectedBrushVS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER(FMatrix44f, LocalToWorld)
			SHADER_PARAMETER(float, bSkeletal)
			SHADER_PARAMETER(FVector2f, PaintUVPadding)
			SHADER_PARAMETER_ARRAY(FMatrix44f, BoneMatrices, [RuntimeMeshPaintProjectedBrushMaxBones])
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		}
	};

	class FRuntimeMeshPaintGPUProjectedBrushPS final : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FRuntimeMeshPaintGPUProjectedBrushPS);
		SHADER_USE_PARAMETER_STRUCT(FRuntimeMeshPaintGPUProjectedBrushPS, FGlobalShader);

		class FWriteMaterialSettings : SHADER_PERMUTATION_BOOL("WRITE_MATERIAL_SETTINGS");
		using FPermutationDomain = TShaderPermutationDomain<FWriteMaterialSettings>;

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER(FVector4f, BrushColor)
			SHADER_PARAMETER(FVector4f, MaterialSettingsColor)
			SHADER_PARAMETER(FVector3f, BrushRayStart)
			SHADER_PARAMETER(FVector3f, BrushRayDirection)
			SHADER_PARAMETER(float, BrushRayLength)
			SHADER_PARAMETER(float, BrushRayCenterT)
			SHADER_PARAMETER(float, BrushProjectionDepth)
			SHADER_PARAMETER(FVector3f, BrushWorldCenter)
			SHADER_PARAMETER(FVector3f, BrushWorldNormal)
			SHADER_PARAMETER(float, BrushWorldRadius)
			SHADER_PARAMETER(float, NormalThreshold)
			SHADER_PARAMETER(float, bUseBrushScreenProjection)
			SHADER_PARAMETER(FVector3f, BrushViewOrigin)
			SHADER_PARAMETER(FVector3f, BrushViewDirection)
			SHADER_PARAMETER(FVector3f, BrushViewRight)
			SHADER_PARAMETER(FVector3f, BrushViewUp)
			SHADER_PARAMETER_TEXTURE(Texture2D, BrushVisibilityDepthTexture)
			SHADER_PARAMETER_SAMPLER(SamplerState, BrushVisibilityDepthSampler)
			SHADER_PARAMETER(FVector2f, BrushVisibilityDepthInvSize)
			SHADER_PARAMETER(float, BrushVisibilityDepthTolerance)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		}
	};

	class FRuntimeMeshPaintGPUBrushVisibilityVS final : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FRuntimeMeshPaintGPUBrushVisibilityVS);
		SHADER_USE_PARAMETER_STRUCT(FRuntimeMeshPaintGPUBrushVisibilityVS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER(FMatrix44f, LocalToWorld)
			SHADER_PARAMETER(float, bSkeletal)
			SHADER_PARAMETER(FVector2f, PaintUVPadding)
			SHADER_PARAMETER_ARRAY(FMatrix44f, BoneMatrices, [RuntimeMeshPaintProjectedBrushMaxBones])
			SHADER_PARAMETER(FVector3f, BrushRayStart)
			SHADER_PARAMETER(FVector3f, BrushRayDirection)
			SHADER_PARAMETER(float, BrushRayLength)
			SHADER_PARAMETER(float, BrushRayCenterT)
			SHADER_PARAMETER(float, BrushProjectionDepth)
			SHADER_PARAMETER(FVector3f, BrushWorldCenter)
			SHADER_PARAMETER(FVector3f, BrushWorldNormal)
			SHADER_PARAMETER(float, BrushWorldRadius)
			SHADER_PARAMETER(float, NormalThreshold)
			SHADER_PARAMETER(float, bUseBrushScreenProjection)
			SHADER_PARAMETER(FVector3f, BrushViewOrigin)
			SHADER_PARAMETER(FVector3f, BrushViewDirection)
			SHADER_PARAMETER(FVector3f, BrushViewRight)
			SHADER_PARAMETER(FVector3f, BrushViewUp)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		}
	};

	class FRuntimeMeshPaintGPUBrushVisibilityPS final : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FRuntimeMeshPaintGPUBrushVisibilityPS);
		SHADER_USE_PARAMETER_STRUCT(FRuntimeMeshPaintGPUBrushVisibilityPS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER(FVector3f, BrushRayStart)
			SHADER_PARAMETER(FVector3f, BrushRayDirection)
			SHADER_PARAMETER(float, BrushRayLength)
			SHADER_PARAMETER(float, BrushRayCenterT)
			SHADER_PARAMETER(float, BrushProjectionDepth)
			SHADER_PARAMETER(FVector3f, BrushWorldCenter)
			SHADER_PARAMETER(FVector3f, BrushWorldNormal)
			SHADER_PARAMETER(float, BrushWorldRadius)
			SHADER_PARAMETER(float, NormalThreshold)
			SHADER_PARAMETER(float, bUseBrushScreenProjection)
			SHADER_PARAMETER(FVector3f, BrushViewOrigin)
			SHADER_PARAMETER(FVector3f, BrushViewDirection)
			SHADER_PARAMETER(FVector3f, BrushViewRight)
			SHADER_PARAMETER(FVector3f, BrushViewUp)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		}
	};

	class FRuntimeMeshPaintGPUProjectedPreviewPS final : public FGlobalShader
	{
	public:
		DECLARE_GLOBAL_SHADER(FRuntimeMeshPaintGPUProjectedPreviewPS);
		SHADER_USE_PARAMETER_STRUCT(FRuntimeMeshPaintGPUProjectedPreviewPS, FGlobalShader);

		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
			SHADER_PARAMETER(FVector3f, BrushRayStart)
			SHADER_PARAMETER(FVector3f, BrushRayDirection)
			SHADER_PARAMETER(float, BrushRayLength)
			SHADER_PARAMETER(float, BrushRayCenterT)
			SHADER_PARAMETER(float, BrushProjectionDepth)
			SHADER_PARAMETER(FVector3f, BrushWorldCenter)
			SHADER_PARAMETER(FVector3f, BrushWorldNormal)
			SHADER_PARAMETER(float, BrushWorldRadius)
			SHADER_PARAMETER(float, NormalThreshold)
			SHADER_PARAMETER(float, bUseBrushScreenProjection)
			SHADER_PARAMETER(FVector3f, BrushViewOrigin)
			SHADER_PARAMETER(FVector3f, BrushViewDirection)
			SHADER_PARAMETER(FVector3f, BrushViewRight)
			SHADER_PARAMETER(FVector3f, BrushViewUp)
			SHADER_PARAMETER(float, PreviewLineThickness)
			SHADER_PARAMETER(FVector4f, PreviewColor)
			SHADER_PARAMETER_TEXTURE(Texture2D, BrushVisibilityDepthTexture)
			SHADER_PARAMETER_SAMPLER(SamplerState, BrushVisibilityDepthSampler)
			SHADER_PARAMETER(FVector2f, BrushVisibilityDepthInvSize)
			SHADER_PARAMETER(float, BrushVisibilityDepthTolerance)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
		{
			return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
		}
	};

	IMPLEMENT_GLOBAL_SHADER(FRuntimeMeshPaintGPUProjectedBrushVS, "/MeshPaintingCore/Private/RuntimeMeshPaintGPUBrush.usf", "ProjectedMainVS", SF_Vertex);
	IMPLEMENT_GLOBAL_SHADER(FRuntimeMeshPaintGPUProjectedBrushPS, "/MeshPaintingCore/Private/RuntimeMeshPaintGPUBrush.usf", "ProjectedMainPS", SF_Pixel);
	IMPLEMENT_GLOBAL_SHADER(FRuntimeMeshPaintGPUBrushVisibilityVS, "/MeshPaintingCore/Private/RuntimeMeshPaintGPUBrush.usf", "ProjectedVisibilityVS", SF_Vertex);
	IMPLEMENT_GLOBAL_SHADER(FRuntimeMeshPaintGPUBrushVisibilityPS, "/MeshPaintingCore/Private/RuntimeMeshPaintGPUBrush.usf", "ProjectedVisibilityPS", SF_Pixel);
	IMPLEMENT_GLOBAL_SHADER(FRuntimeMeshPaintGPUProjectedPreviewPS, "/MeshPaintingCore/Private/RuntimeMeshPaintGPUBrush.usf", "ProjectedPreviewMainPS", SF_Pixel);

	void RemoveInvalidGPUBrushResources()
	{
		for (auto ResourceIt = GRuntimeMeshPaintGPUProjectedBrushResources.CreateIterator(); ResourceIt; ++ResourceIt)
		{
			const TSharedPtr<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe>& Resource = ResourceIt.Value();
			if (!Resource.IsValid() || !Resource->SourceMeshAsset.IsValid())
			{
				ResourceIt.RemoveCurrent();
			}
		}
	}

	FRuntimeMeshPaintGPUProjectedBrushVertex MakeProjectedVertex(
		const FVector2D& UV,
		const FVector3f& LocalPosition,
		const FVector3f& LocalNormal,
		const FVector2f& PaintUVPaddingDirection)
	{
		FRuntimeMeshPaintGPUProjectedBrushVertex Vertex;
		Vertex.PaintUV = FVector2f(UV);
		Vertex.LocalPosition = LocalPosition;
		Vertex.LocalNormal = LocalNormal;
		Vertex.PaintUVPaddingDirection = PaintUVPaddingDirection;
		Vertex.BoneIndices0 = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
		Vertex.BoneWeights0 = FVector4f(1.0f, 0.0f, 0.0f, 0.0f);
		Vertex.BoneIndices1 = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
		Vertex.BoneWeights1 = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
		return Vertex;
	}

	FVector2f MakeProjectedPaintUVEdgePaddingDirection(
		const FVector2D& EdgeUV0,
		const FVector2D& EdgeUV1,
		const FVector2D& OppositeUV)
	{
		const FVector2D Edge = EdgeUV1 - EdgeUV0;
		const double EdgeLength = Edge.Size();
		if (EdgeLength <= SMALL_NUMBER)
		{
			return FVector2f::ZeroVector;
		}

		FVector2D Direction(-Edge.Y / EdgeLength, Edge.X / EdgeLength);
		const FVector2D EdgeCenter = (EdgeUV0 + EdgeUV1) * 0.5;
		if (FVector2D::DotProduct(Direction, OppositeUV - EdgeCenter) > 0.0)
		{
			Direction *= -1.0;
		}

		return FVector2f(Direction);
	}

	void BuildProjectedPaintUVEdgeMap(
		const FPaintUVCache& Cache,
		TMultiMap<FUVEdgeKey, int32>& OutEdgeToTriangles)
	{
		OutEdgeToTriangles.Reset();
		OutEdgeToTriangles.Reserve(Cache.Triangles.Num() * 3);
		for (int32 TriangleArrayIndex = 0; TriangleArrayIndex < Cache.Triangles.Num(); ++TriangleArrayIndex)
		{
			FRuntimeMeshPaintGeometry::AddTriangleEdgesToMap(
				Cache.Triangles[TriangleArrayIndex],
				TriangleArrayIndex,
				Cache.UVConnectionTolerance,
				OutEdgeToTriangles);
		}
	}

	bool IsProjectedPaintUVBoundaryEdge(
		const FPaintUVCache& Cache,
		const TMultiMap<FUVEdgeKey, int32>& EdgeToTriangles,
		int32 TriangleArrayIndex,
		const FVector2D& EdgeUV0,
		const FVector2D& EdgeUV1)
	{
		const int32 IslandId = Cache.TriangleIslandIds.IsValidIndex(TriangleArrayIndex)
			? Cache.TriangleIslandIds[TriangleArrayIndex]
			: INDEX_NONE;
		const FUVEdgeKey EdgeKey = FRuntimeMeshPaintGeometry::MakeUVEdgeKey(
			EdgeUV0,
			EdgeUV1,
			Cache.UVConnectionTolerance);

		TArray<int32> ConnectedTriangleArrayIndices;
		EdgeToTriangles.MultiFind(EdgeKey, ConnectedTriangleArrayIndices);
		for (const int32 ConnectedTriangleArrayIndex : ConnectedTriangleArrayIndices)
		{
			if (ConnectedTriangleArrayIndex == TriangleArrayIndex)
			{
				continue;
			}

			if (IslandId == INDEX_NONE ||
				(Cache.TriangleIslandIds.IsValidIndex(ConnectedTriangleArrayIndex) &&
					Cache.TriangleIslandIds[ConnectedTriangleArrayIndex] == IslandId))
			{
				return false;
			}
		}

		return true;
	}

	void AddProjectedPaintUVBoundaryStrip(
		TArray<FRuntimeMeshPaintGPUProjectedBrushVertex>& Vertices,
		const FRuntimeMeshPaintGPUProjectedBrushVertex& EdgeVertex0,
		const FRuntimeMeshPaintGPUProjectedBrushVertex& EdgeVertex1,
		const FVector2D& EdgeUV0,
		const FVector2D& EdgeUV1,
		const FVector2D& OppositeUV)
	{
		const FVector2f PaddingDirection = MakeProjectedPaintUVEdgePaddingDirection(EdgeUV0, EdgeUV1, OppositeUV);
		if (PaddingDirection.IsNearlyZero())
		{
			return;
		}

		FRuntimeMeshPaintGPUProjectedBrushVertex Inner0 = EdgeVertex0;
		FRuntimeMeshPaintGPUProjectedBrushVertex Inner1 = EdgeVertex1;
		FRuntimeMeshPaintGPUProjectedBrushVertex Outer0 = EdgeVertex0;
		FRuntimeMeshPaintGPUProjectedBrushVertex Outer1 = EdgeVertex1;
		Inner0.PaintUVPaddingDirection = FVector2f::ZeroVector;
		Inner1.PaintUVPaddingDirection = FVector2f::ZeroVector;
		Outer0.PaintUVPaddingDirection = PaddingDirection;
		Outer1.PaintUVPaddingDirection = PaddingDirection;

		Vertices.Add(Inner0);
		Vertices.Add(Inner1);
		Vertices.Add(Outer1);

		Vertices.Add(Inner0);
		Vertices.Add(Outer1);
		Vertices.Add(Outer0);
	}

	void AddProjectedPaintUVBoundaryStripIfNeeded(
		TArray<FRuntimeMeshPaintGPUProjectedBrushVertex>& Vertices,
		const FPaintUVCache& Cache,
		const TMultiMap<FUVEdgeKey, int32>& EdgeToTriangles,
		int32 TriangleArrayIndex,
		const FRuntimeMeshPaintGPUProjectedBrushVertex& EdgeVertex0,
		const FRuntimeMeshPaintGPUProjectedBrushVertex& EdgeVertex1,
		const FVector2D& EdgeUV0,
		const FVector2D& EdgeUV1,
		const FVector2D& OppositeUV)
	{
		if (!IsProjectedPaintUVBoundaryEdge(Cache, EdgeToTriangles, TriangleArrayIndex, EdgeUV0, EdgeUV1))
		{
			return;
		}

		AddProjectedPaintUVBoundaryStrip(Vertices, EdgeVertex0, EdgeVertex1, EdgeUV0, EdgeUV1, OppositeUV);
	}

	FVector3f MakeProjectedFallbackNormal(
		const FVector3f& Position0,
		const FVector3f& Position1,
		const FVector3f& Position2)
	{
		return FVector3f(
			FVector::CrossProduct(FVector(Position1 - Position0), FVector(Position2 - Position0)).GetSafeNormal(
				SMALL_NUMBER,
				FVector::UpVector));
	}

	FVector3f MakeSafeProjectedNormal(const FVector3f& Normal, const FVector3f& FallbackNormal)
	{
		return Normal.GetSafeNormal(SMALL_NUMBER, FallbackNormal);
	}

	bool BuildStaticProjectedVertices(
		const UStaticMeshComponent* StaticMeshComponent,
		const FPaintUVCache& Cache,
		TArray<FRuntimeMeshPaintGPUProjectedBrushVertex>& OutVertices)
	{
		const UStaticMesh* StaticMesh = StaticMeshComponent ? StaticMeshComponent->GetStaticMesh() : nullptr;
		const FStaticMeshRenderData* RenderData = StaticMesh ? StaticMesh->GetRenderData() : nullptr;
		if (!RenderData || RenderData->LODResources.Num() <= RuntimeMeshPaintCacheLODIndex) return false;

		const FStaticMeshLODResources& LODResources = RenderData->LODResources[RuntimeMeshPaintCacheLODIndex];
		const FIndexArrayView IndexBuffer = LODResources.IndexBuffer.GetArrayView();
		const uint32 NumPositionVertices = LODResources.VertexBuffers.PositionVertexBuffer.GetNumVertices();
		const FStaticMeshVertexBuffer& StaticMeshVertexBuffer = LODResources.VertexBuffers.StaticMeshVertexBuffer;
		const uint32 NumStaticMeshVertices = StaticMeshVertexBuffer.GetNumVertices();

		OutVertices.Reset();
		OutVertices.Reserve(Cache.Triangles.Num() * 6);
		TMultiMap<FUVEdgeKey, int32> EdgeToTriangles;
		BuildProjectedPaintUVEdgeMap(Cache, EdgeToTriangles);
		for (int32 TriangleArrayIndex = 0; TriangleArrayIndex < Cache.Triangles.Num(); ++TriangleArrayIndex)
		{
			const RuntimeMeshPaint::FPaintUVTriangle& Triangle = Cache.Triangles[TriangleArrayIndex];
			const int32 FirstIndex = Triangle.TriangleIndex * 3;
			if (FirstIndex < 0 || FirstIndex + 2 >= IndexBuffer.Num()) continue;

			const uint32 Index0 = IndexBuffer[FirstIndex];
			const uint32 Index1 = IndexBuffer[FirstIndex + 1];
			const uint32 Index2 = IndexBuffer[FirstIndex + 2];
			if (Index0 >= NumPositionVertices || Index1 >= NumPositionVertices || Index2 >= NumPositionVertices) continue;

			const FVector3f Position0 = LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(Index0);
			const FVector3f Position1 = LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(Index1);
			const FVector3f Position2 = LODResources.VertexBuffers.PositionVertexBuffer.VertexPosition(Index2);
			const FVector3f FallbackNormal = MakeProjectedFallbackNormal(Position0, Position1, Position2);
			const FVector3f Normal0 = Index0 < NumStaticMeshVertices
				? MakeSafeProjectedNormal(StaticMeshVertexBuffer.VertexTangentZ(Index0), FallbackNormal)
				: FallbackNormal;
			const FVector3f Normal1 = Index1 < NumStaticMeshVertices
				? MakeSafeProjectedNormal(StaticMeshVertexBuffer.VertexTangentZ(Index1), FallbackNormal)
				: FallbackNormal;
			const FVector3f Normal2 = Index2 < NumStaticMeshVertices
				? MakeSafeProjectedNormal(StaticMeshVertexBuffer.VertexTangentZ(Index2), FallbackNormal)
				: FallbackNormal;
			const FRuntimeMeshPaintGPUProjectedBrushVertex Vertex0 =
				MakeProjectedVertex(Triangle.UV0, Position0, Normal0, FVector2f::ZeroVector);
			const FRuntimeMeshPaintGPUProjectedBrushVertex Vertex1 =
				MakeProjectedVertex(Triangle.UV1, Position1, Normal1, FVector2f::ZeroVector);
			const FRuntimeMeshPaintGPUProjectedBrushVertex Vertex2 =
				MakeProjectedVertex(Triangle.UV2, Position2, Normal2, FVector2f::ZeroVector);

			OutVertices.Add(Vertex0);
			OutVertices.Add(Vertex1);
			OutVertices.Add(Vertex2);

			AddProjectedPaintUVBoundaryStripIfNeeded(
				OutVertices, Cache, EdgeToTriangles, TriangleArrayIndex,
				Vertex0, Vertex1, Triangle.UV0, Triangle.UV1, Triangle.UV2);
			AddProjectedPaintUVBoundaryStripIfNeeded(
				OutVertices, Cache, EdgeToTriangles, TriangleArrayIndex,
				Vertex1, Vertex2, Triangle.UV1, Triangle.UV2, Triangle.UV0);
			AddProjectedPaintUVBoundaryStripIfNeeded(
				OutVertices, Cache, EdgeToTriangles, TriangleArrayIndex,
				Vertex2, Vertex0, Triangle.UV2, Triangle.UV0, Triangle.UV1);
		}

		return OutVertices.Num() >= 3;
	}

	void SetSkeletalProjectedInfluences(
		const FSkinWeightVertexBuffer& SkinWeightBuffer,
		const FSkelMeshRenderSection& Section,
		int32 VertexIndex,
		FRuntimeMeshPaintGPUProjectedBrushVertex& Vertex)
	{
		const int32 MaxInfluences = FMath::Min(static_cast<int32>(SkinWeightBuffer.GetMaxBoneInfluences()), 8);
		float BoneIndices[8] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
		float BoneWeights[8] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
		float WeightSum = 0.0f;
		int32 WriteIndex = 0;

		for (int32 InfluenceIndex = 0; InfluenceIndex < MaxInfluences && WriteIndex < 8; ++InfluenceIndex)
		{
			const uint32 Weight = SkinWeightBuffer.GetBoneWeight(VertexIndex, InfluenceIndex);
			if (Weight == 0) continue;

			const int32 BoneMapIndex = static_cast<int32>(SkinWeightBuffer.GetBoneIndex(VertexIndex, InfluenceIndex));
			if (!Section.BoneMap.IsValidIndex(BoneMapIndex)) continue;

			const int32 MeshBoneIndex = static_cast<int32>(Section.BoneMap[BoneMapIndex]);
			if (MeshBoneIndex < 0 || MeshBoneIndex >= RuntimeMeshPaintProjectedBrushMaxBones) continue;

			const float NormalizedWeight = Weight / 255.0f;
			BoneIndices[WriteIndex] = static_cast<float>(MeshBoneIndex);
			BoneWeights[WriteIndex] = NormalizedWeight;
			WeightSum += NormalizedWeight;
			++WriteIndex;
		}

		if (WeightSum <= SMALL_NUMBER)
		{
			BoneIndices[0] = 0.0f;
			BoneWeights[0] = 1.0f;
			WeightSum = 1.0f;
		}

		const float InvWeightSum = 1.0f / WeightSum;
		for (float& BoneWeight : BoneWeights)
		{
			BoneWeight *= InvWeightSum;
		}

		Vertex.BoneIndices0 = FVector4f(BoneIndices[0], BoneIndices[1], BoneIndices[2], BoneIndices[3]);
		Vertex.BoneWeights0 = FVector4f(BoneWeights[0], BoneWeights[1], BoneWeights[2], BoneWeights[3]);
		Vertex.BoneIndices1 = FVector4f(BoneIndices[4], BoneIndices[5], BoneIndices[6], BoneIndices[7]);
		Vertex.BoneWeights1 = FVector4f(BoneWeights[4], BoneWeights[5], BoneWeights[6], BoneWeights[7]);
	}

	FRuntimeMeshPaintGPUProjectedBrushVertex MakeSkeletalProjectedVertex(
		const FSkinWeightVertexBuffer& SkinWeightBuffer,
		const FSkelMeshRenderSection& Section,
		int32 VertexIndex,
		const FVector2D& UV,
		const FVector3f& LocalPosition,
		const FVector3f& LocalNormal,
		const FVector2f& PaintUVPaddingDirection)
	{
		FRuntimeMeshPaintGPUProjectedBrushVertex Vertex;
		Vertex.PaintUV = FVector2f(UV);
		Vertex.LocalPosition = LocalPosition;
		Vertex.LocalNormal = LocalNormal;
		Vertex.PaintUVPaddingDirection = PaintUVPaddingDirection;
		SetSkeletalProjectedInfluences(SkinWeightBuffer, Section, VertexIndex, Vertex);
		return Vertex;
	}

	bool BuildSkeletalProjectedVertices(
		const USkeletalMeshComponent* SkeletalMeshComponent,
		const FPaintUVCache& Cache,
		TArray<FRuntimeMeshPaintGPUProjectedBrushVertex>& OutVertices)
	{
		const FSkeletalMeshRenderData* RenderData = SkeletalMeshComponent ? SkeletalMeshComponent->GetSkeletalMeshRenderData() : nullptr;
		if (!RenderData || RenderData->LODRenderData.Num() <= RuntimeMeshPaintCacheLODIndex) return false;

		const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[RuntimeMeshPaintCacheLODIndex];
		const FSkinWeightVertexBuffer* SkinWeightBuffer = LODData.GetSkinWeightVertexBuffer();
		if (!SkinWeightBuffer ||
			Cache.TriangleVertexIndices.Num() != Cache.Triangles.Num() ||
			Cache.TriangleArraySectionIds.Num() != Cache.Triangles.Num())
		{
			return false;
		}

		const uint32 NumPositionVertices = LODData.StaticVertexBuffers.PositionVertexBuffer.GetNumVertices();
		const FStaticMeshVertexBuffer& StaticMeshVertexBuffer = LODData.StaticVertexBuffers.StaticMeshVertexBuffer;
		const uint32 NumStaticMeshVertices = StaticMeshVertexBuffer.GetNumVertices();
		OutVertices.Reset();
		OutVertices.Reserve(Cache.Triangles.Num() * 6);
		TMultiMap<FUVEdgeKey, int32> EdgeToTriangles;
		BuildProjectedPaintUVEdgeMap(Cache, EdgeToTriangles);
		for (int32 TriangleArrayIndex = 0; TriangleArrayIndex < Cache.Triangles.Num(); ++TriangleArrayIndex)
		{
			if (!Cache.TriangleVertexIndices.IsValidIndex(TriangleArrayIndex) ||
				!Cache.TriangleArraySectionIds.IsValidIndex(TriangleArrayIndex))
			{
				continue;
			}

			const int32 SectionIndex = Cache.TriangleArraySectionIds[TriangleArrayIndex];
			if (!LODData.RenderSections.IsValidIndex(SectionIndex)) continue;

			const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];
			const FIntVector& VertexIndices = Cache.TriangleVertexIndices[TriangleArrayIndex];
			if (VertexIndices.X < 0 || VertexIndices.Y < 0 || VertexIndices.Z < 0 ||
				static_cast<uint32>(VertexIndices.X) >= NumPositionVertices ||
				static_cast<uint32>(VertexIndices.Y) >= NumPositionVertices ||
				static_cast<uint32>(VertexIndices.Z) >= NumPositionVertices)
			{
				continue;
			}

			const RuntimeMeshPaint::FPaintUVTriangle& Triangle = Cache.Triangles[TriangleArrayIndex];
			const FVector3f Position0 = LODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndices.X);
			const FVector3f Position1 = LODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndices.Y);
			const FVector3f Position2 = LODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndices.Z);
			const FVector3f FallbackNormal = MakeProjectedFallbackNormal(Position0, Position1, Position2);
			const FVector3f Normal0 = static_cast<uint32>(VertexIndices.X) < NumStaticMeshVertices
				? MakeSafeProjectedNormal(StaticMeshVertexBuffer.VertexTangentZ(VertexIndices.X), FallbackNormal)
				: FallbackNormal;
			const FVector3f Normal1 = static_cast<uint32>(VertexIndices.Y) < NumStaticMeshVertices
				? MakeSafeProjectedNormal(StaticMeshVertexBuffer.VertexTangentZ(VertexIndices.Y), FallbackNormal)
				: FallbackNormal;
			const FVector3f Normal2 = static_cast<uint32>(VertexIndices.Z) < NumStaticMeshVertices
				? MakeSafeProjectedNormal(StaticMeshVertexBuffer.VertexTangentZ(VertexIndices.Z), FallbackNormal)
				: FallbackNormal;
			const FRuntimeMeshPaintGPUProjectedBrushVertex Vertex0 =
				MakeSkeletalProjectedVertex(*SkinWeightBuffer, Section, VertexIndices.X, Triangle.UV0, Position0, Normal0, FVector2f::ZeroVector);
			const FRuntimeMeshPaintGPUProjectedBrushVertex Vertex1 =
				MakeSkeletalProjectedVertex(*SkinWeightBuffer, Section, VertexIndices.Y, Triangle.UV1, Position1, Normal1, FVector2f::ZeroVector);
			const FRuntimeMeshPaintGPUProjectedBrushVertex Vertex2 =
				MakeSkeletalProjectedVertex(*SkinWeightBuffer, Section, VertexIndices.Z, Triangle.UV2, Position2, Normal2, FVector2f::ZeroVector);

			OutVertices.Add(Vertex0);
			OutVertices.Add(Vertex1);
			OutVertices.Add(Vertex2);

			AddProjectedPaintUVBoundaryStripIfNeeded(
				OutVertices, Cache, EdgeToTriangles, TriangleArrayIndex,
				Vertex0, Vertex1, Triangle.UV0, Triangle.UV1, Triangle.UV2);
			AddProjectedPaintUVBoundaryStripIfNeeded(
				OutVertices, Cache, EdgeToTriangles, TriangleArrayIndex,
				Vertex1, Vertex2, Triangle.UV1, Triangle.UV2, Triangle.UV0);
			AddProjectedPaintUVBoundaryStripIfNeeded(
				OutVertices, Cache, EdgeToTriangles, TriangleArrayIndex,
				Vertex2, Vertex0, Triangle.UV2, Triangle.UV0, Triangle.UV1);
		}

		return OutVertices.Num() >= 3;
	}

	TSharedPtr<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe> FindOrCreateGPUProjectedBrushResource(
		UMeshComponent* MeshComponent,
		const TSharedPtr<const FPaintUVCache>& Cache)
	{
		if (!MeshComponent || !Cache.IsValid() || Cache->Triangles.Num() == 0)
		{
			return nullptr;
		}

		RemoveInvalidGPUBrushResources();

		if (const TSharedPtr<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe>* ExistingResource =
			GRuntimeMeshPaintGPUProjectedBrushResources.Find(Cache.Get()))
		{
			if (ExistingResource->IsValid())
			{
				return *ExistingResource;
			}

			GRuntimeMeshPaintGPUProjectedBrushResources.Remove(Cache.Get());
		}

		TSharedPtr<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe> Resource =
			MakeShared<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe>();
		Resource->SourceMeshAsset = Cache->MeshAsset.Get();
		Resource->MeshType = Cache->MeshType;

		bool bBuiltVertices = false;
		if (Cache->MeshType == EPaintUVCacheMeshType::StaticMesh)
		{
			bBuiltVertices = BuildStaticProjectedVertices(Cast<UStaticMeshComponent>(MeshComponent), *Cache, Resource->InitialVertices);
		}
		else
		{
			bBuiltVertices = BuildSkeletalProjectedVertices(Cast<USkeletalMeshComponent>(MeshComponent), *Cache, Resource->InitialVertices);
		}

		if (!bBuiltVertices || Resource->InitialVertices.Num() < 3)
		{
			return nullptr;
		}

		Resource->VertexCount = static_cast<uint32>(Resource->InitialVertices.Num());
		ENQUEUE_RENDER_COMMAND(RuntimeMeshPaintCreateGPUProjectedBrushResource)(
			[Resource](FRHICommandListImmediate& RHICmdList)
			{
				if (Resource->InitialVertices.Num() < 3) return;

				const FRHIBufferCreateDesc VertexBufferDesc =
					FRHIBufferCreateDesc::CreateVertex<FRuntimeMeshPaintGPUProjectedBrushVertex>(
						TEXT("RuntimeMeshPaintGPUProjectedBrushVB"),
						static_cast<uint32>(Resource->InitialVertices.Num()))
					.AddUsage(EBufferUsageFlags::Static)
					.SetInitialState(ERHIAccess::VertexOrIndexBuffer)
					.SetInitActionInitializer();

				TRHIBufferInitializer<FRuntimeMeshPaintGPUProjectedBrushVertex> VertexInitializer =
					RHICmdList.CreateBufferInitializer(VertexBufferDesc);
				for (int32 VertexIndex = 0; VertexIndex < Resource->InitialVertices.Num(); ++VertexIndex)
				{
					VertexInitializer[VertexIndex] = Resource->InitialVertices[VertexIndex];
				}

				Resource->VertexBufferRHI = VertexInitializer.Finalize();
				Resource->InitialVertices.Empty();
			});

		GRuntimeMeshPaintGPUProjectedBrushResources.Add(Cache.Get(), Resource);
		return Resource;
	}

	FTextureRHIRef GetProjectedBrushVisibilityTexture(FRHICommandListImmediate& RHICmdList, const FIntPoint& VisibilitySize)
	{
		if (!GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthTexture.IsValid() ||
			!GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthStencilTexture.IsValid() ||
			GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthSize != VisibilitySize)
		{
			const FRHITextureCreateDesc VisibilityDepthDesc = FRHITextureCreateDesc::Create2D(TEXT("RuntimeMeshPaintGPUBrushVisibilityDepth"))
				.SetExtent(VisibilitySize)
				.SetFormat(PF_A32B32G32R32F)
				.SetClearValue(FClearValueBinding::White)
				.SetFlags(ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::ShaderResource)
				.SetInitialState(ERHIAccess::SRVMask);

			const FRHITextureCreateDesc VisibilityDepthStencilDesc = FRHITextureCreateDesc::Create2D(TEXT("RuntimeMeshPaintGPUBrushVisibilityDepthStencil"))
				.SetExtent(VisibilitySize)
				.SetFormat(PF_DepthStencil)
				.SetClearValue(FClearValueBinding::DepthOne)
				.SetFlags(ETextureCreateFlags::DepthStencilTargetable)
				.SetInitialState(ERHIAccess::DSVWrite);

			GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthTexture = RHICreateTexture(VisibilityDepthDesc);
			GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthStencilTexture = RHICreateTexture(VisibilityDepthStencilDesc);
			GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthSize =
				GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthTexture.IsValid() &&
				GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthStencilTexture.IsValid()
				? VisibilitySize
				: FIntPoint::ZeroValue;
		}

		if (GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthTexture.IsValid() &&
			GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthStencilTexture.IsValid())
		{
			RHICmdList.Transition(FRHITransitionInfo(
				GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthTexture,
				ERHIAccess::SRVMask,
				ERHIAccess::RTV));
			return GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthTexture;
		}

		return nullptr;
	}

	FRuntimeMeshPaintGPUBrushProjectionParameters MakeGPUBrushProjectionParameters(
		const FRuntimeMeshPaintScreenProjectionData& ProjectionData,
		const FVector& BrushWorldCenter,
		float BrushWorldRadius)
	{
		FRuntimeMeshPaintGPUBrushProjectionParameters Result;
		if (!ProjectionData.bHasScreenProjection || BrushWorldRadius <= KINDA_SMALL_NUMBER)
		{
			return Result;
		}

		const FVector ViewRight = ProjectionData.ViewRight.GetSafeNormal(SMALL_NUMBER, FVector::RightVector);
		const FVector ViewUp = ProjectionData.ViewUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
		const FVector ViewDirection = ProjectionData.ViewForward.GetSafeNormal(SMALL_NUMBER, FVector::ForwardVector);
		const float ViewCenterDepth = static_cast<float>(
			FVector::DotProduct(BrushWorldCenter - ProjectionData.ViewOrigin, ViewDirection));
		if (!FMath::IsFinite(ViewCenterDepth) || ViewCenterDepth <= 0.0f)
		{
			return Result;
		}

		Result.bUseScreenProjection = 1.0f;
		Result.ViewOrigin = FVector3f(ProjectionData.ViewOrigin);
		Result.ViewDirection = FVector3f(ViewDirection);
		Result.ViewRight = FVector3f(ViewRight);
		Result.ViewUp = FVector3f(ViewUp);
		return Result;
	}

	void FillProjectedBrushVisibilityVertexParameters(
		FRuntimeMeshPaintGPUBrushVisibilityVS::FParameters& OutParameters,
		const FMatrix44f& LocalToWorld,
		bool bSkeletal,
		const TArray<FMatrix44f>& BoneMatrices,
		const FVector& BrushRayStart,
		const FVector& RayDirection,
		float RayLength,
		float RayCenterT,
		float ProjectionDepth,
		const FVector& BrushWorldCenter,
		const FVector& BrushWorldNormal,
		float BrushWorldRadius,
		float NormalThreshold,
		const FRuntimeMeshPaintGPUBrushProjectionParameters& BrushProjectionParameters)
	{
		OutParameters.LocalToWorld = LocalToWorld;
		OutParameters.bSkeletal = bSkeletal ? 1.0f : 0.0f;
		OutParameters.PaintUVPadding = FVector2f::ZeroVector;
		for (int32 BoneIndex = 0; BoneIndex < RuntimeMeshPaintProjectedBrushMaxBones; ++BoneIndex)
		{
			OutParameters.BoneMatrices[BoneIndex] = BoneMatrices.IsValidIndex(BoneIndex)
				? BoneMatrices[BoneIndex]
				: FMatrix44f::Identity;
		}
		OutParameters.BrushRayStart = FVector3f(BrushRayStart);
		OutParameters.BrushRayDirection = FVector3f(RayDirection);
		OutParameters.BrushRayLength = RayLength;
		OutParameters.BrushRayCenterT = RayCenterT;
		OutParameters.BrushProjectionDepth = ProjectionDepth;
		OutParameters.BrushWorldCenter = FVector3f(BrushWorldCenter);
		OutParameters.BrushWorldNormal = FVector3f(BrushWorldNormal.GetSafeNormal(SMALL_NUMBER, -RayDirection));
		OutParameters.BrushWorldRadius = FMath::Max(BrushWorldRadius, KINDA_SMALL_NUMBER);
		OutParameters.NormalThreshold = NormalThreshold;
		OutParameters.bUseBrushScreenProjection = BrushProjectionParameters.bUseScreenProjection;
		OutParameters.BrushViewOrigin = BrushProjectionParameters.ViewOrigin;
		OutParameters.BrushViewDirection = BrushProjectionParameters.ViewDirection;
		OutParameters.BrushViewRight = BrushProjectionParameters.ViewRight;
		OutParameters.BrushViewUp = BrushProjectionParameters.ViewUp;
	}

	void FillProjectedBrushVisibilityPixelParameters(
		FRuntimeMeshPaintGPUBrushVisibilityPS::FParameters& OutParameters,
		const FVector& BrushRayStart,
		const FVector& RayDirection,
		float RayLength,
		float RayCenterT,
		float ProjectionDepth,
		const FVector& BrushWorldCenter,
		const FVector& BrushWorldNormal,
		float BrushWorldRadius,
		float NormalThreshold,
		const FRuntimeMeshPaintGPUBrushProjectionParameters& BrushProjectionParameters)
	{
		OutParameters.BrushRayStart = FVector3f(BrushRayStart);
		OutParameters.BrushRayDirection = FVector3f(RayDirection);
		OutParameters.BrushRayLength = RayLength;
		OutParameters.BrushRayCenterT = RayCenterT;
		OutParameters.BrushProjectionDepth = ProjectionDepth;
		OutParameters.BrushWorldCenter = FVector3f(BrushWorldCenter);
		OutParameters.BrushWorldNormal = FVector3f(BrushWorldNormal.GetSafeNormal(SMALL_NUMBER, -RayDirection));
		OutParameters.BrushWorldRadius = FMath::Max(BrushWorldRadius, KINDA_SMALL_NUMBER);
		OutParameters.NormalThreshold = NormalThreshold;
		OutParameters.bUseBrushScreenProjection = BrushProjectionParameters.bUseScreenProjection;
		OutParameters.BrushViewOrigin = BrushProjectionParameters.ViewOrigin;
		OutParameters.BrushViewDirection = BrushProjectionParameters.ViewDirection;
		OutParameters.BrushViewRight = BrushProjectionParameters.ViewRight;
		OutParameters.BrushViewUp = BrushProjectionParameters.ViewUp;
	}

	FTextureRHIRef DrawGPUProjectedBrushVisibilityPrimitives(
		FRHICommandListImmediate& RHICmdList,
		const TSharedPtr<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe>& GPUResource,
		const FIntPoint& VisibilitySize,
		const FMatrix44f& LocalToWorld,
		bool bSkeletal,
		const TArray<FMatrix44f>& BoneMatrices,
		const FVector& BrushRayStart,
		const FVector& RayDirection,
		float RayLength,
		float RayCenterT,
		float ProjectionDepth,
		const FVector& BrushWorldCenter,
		const FVector& BrushWorldNormal,
		float BrushWorldRadius,
		float NormalThreshold,
		const FRuntimeMeshPaintGPUBrushProjectionParameters& BrushProjectionParameters)
	{
		if (!GPUResource.IsValid() || !GPUResource->VertexBufferRHI.IsValid() || GPUResource->VertexCount < 3)
		{
			return nullptr;
		}

		FTextureRHIRef VisibilityDepthTexture = GetProjectedBrushVisibilityTexture(RHICmdList, VisibilitySize);
		if (!VisibilityDepthTexture.IsValid() ||
			!GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthStencilTexture.IsValid())
		{
			return nullptr;
		}

		FRHIRenderPassInfo RenderPassInfo(
			VisibilityDepthTexture,
			ERenderTargetActions::Clear_Store,
			GRuntimeMeshPaintGPUProjectedBrushVisibilityDepthStencilTexture,
			EDepthStencilTargetActions::ClearDepthStencil_DontStoreDepthStencil,
			FExclusiveDepthStencil::DepthWrite_StencilNop);
		RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("RuntimeMeshPaintGPUBrushVisibility"));
		RHICmdList.SetViewport(0.0f, 0.0f, 0.0f, VisibilitySize.X, VisibilitySize.Y, 1.0f);

		FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
		TShaderMapRef<FRuntimeMeshPaintGPUBrushVisibilityVS> VertexShader(ShaderMap);
		TShaderMapRef<FRuntimeMeshPaintGPUBrushVisibilityPS> PixelShader(ShaderMap);

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<true, CF_LessEqual>::GetRHI();
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
			GRuntimeMeshPaintGPUProjectedBrushVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

		FRuntimeMeshPaintGPUBrushVisibilityVS::FParameters VertexParameters;
		FillProjectedBrushVisibilityVertexParameters(
			VertexParameters,
			LocalToWorld,
			bSkeletal,
			BoneMatrices,
			BrushRayStart,
			RayDirection,
			RayLength,
			RayCenterT,
			ProjectionDepth,
			BrushWorldCenter,
			BrushWorldNormal,
			BrushWorldRadius,
			NormalThreshold,
			BrushProjectionParameters);
		SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), VertexParameters);

		FRuntimeMeshPaintGPUBrushVisibilityPS::FParameters PixelParameters;
		FillProjectedBrushVisibilityPixelParameters(
			PixelParameters,
			BrushRayStart,
			RayDirection,
			RayLength,
			RayCenterT,
			ProjectionDepth,
			BrushWorldCenter,
			BrushWorldNormal,
			BrushWorldRadius,
			NormalThreshold,
			BrushProjectionParameters);
		SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PixelParameters);

		RHICmdList.SetStreamSource(0, GPUResource->VertexBufferRHI, 0);
		RHICmdList.DrawPrimitive(0, GPUResource->VertexCount / 3, 1);
		RHICmdList.EndRenderPass();
		RHICmdList.Transition(FRHITransitionInfo(VisibilityDepthTexture, ERHIAccess::RTV, ERHIAccess::SRVMask));

		return VisibilityDepthTexture;
	}

	void DrawGPUProjectedBrushPrimitives(
		FRHICommandListImmediate& RHICmdList,
		const TSharedPtr<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe>& GPUResource,
		FTextureRenderTargetResource* ColorResource,
		FTextureRenderTargetResource* MaterialSettingsResource,
		const FIntPoint& RenderTargetSize,
		const FMatrix44f& LocalToWorld,
		bool bSkeletal,
		const TArray<FMatrix44f>& BoneMatrices,
		const FVector& BrushRayStart,
		const FVector& BrushRayEnd,
		const FVector& BrushWorldCenter,
		const FVector& BrushWorldNormal,
		float BrushWorldRadius,
		const FRuntimeMeshPaintScreenProjectionData& ProjectionData,
		const FLinearColor& Color,
		const FLinearColor& MaterialSettings,
		bool bErase)
	{
		if (!ColorResource || !GPUResource.IsValid() || !GPUResource->VertexBufferRHI.IsValid() || GPUResource->VertexCount < 3) return;

		FRHITexture* ColorTexture = ColorResource->GetRenderTargetTexture();
		FRHITexture* MaterialSettingsTexture = MaterialSettingsResource
			? MaterialSettingsResource->GetRenderTargetTexture()
			: nullptr;
		if (!ColorTexture) return;

		const FVector RayVector = BrushRayEnd - BrushRayStart;
		const float RayLength = FMath::Max(static_cast<float>(RayVector.Size()), KINDA_SMALL_NUMBER);
		const FVector RayDirection = RayVector / RayLength;
		const float RayCenterT = FMath::Clamp(
			static_cast<float>(FVector::DotProduct(BrushWorldCenter - BrushRayStart, RayDirection)),
			0.0f,
			RayLength);
		const float ProjectionDepth = FMath::Max(
			FMath::Max(BrushWorldRadius, KINDA_SMALL_NUMBER) * RuntimeMeshPaintProjectedBrushDepthRadiusScale,
			RuntimeMeshPaintProjectedBrushMinDepth);
		const float EffectiveNormalThreshold = bSkeletal
			? RuntimeMeshPaintProjectedSkeletalBrushNormalThreshold
			: RuntimeMeshPaintProjectedBrushNormalThreshold;
		const FRuntimeMeshPaintGPUBrushProjectionParameters BrushProjectionParameters =
			MakeGPUBrushProjectionParameters(ProjectionData, BrushWorldCenter, BrushWorldRadius);
		const FIntPoint VisibilitySize(
			RuntimeMeshPaintProjectedBrushVisibilitySize,
			RuntimeMeshPaintProjectedBrushVisibilitySize);
		FTextureRHIRef VisibilityDepthTexture = DrawGPUProjectedBrushVisibilityPrimitives(
			RHICmdList,
			GPUResource,
			VisibilitySize,
			LocalToWorld,
			bSkeletal,
			BoneMatrices,
			BrushRayStart,
			RayDirection,
			RayLength,
			RayCenterT,
			ProjectionDepth,
			BrushWorldCenter,
			BrushWorldNormal,
			BrushWorldRadius,
			EffectiveNormalThreshold,
			BrushProjectionParameters);
		if (!VisibilityDepthTexture.IsValid())
		{
			return;
		}

		FRHITexture* RenderTargets[2] = {ColorTexture, MaterialSettingsTexture};
		const int32 NumRenderTargets = MaterialSettingsTexture ? 2 : 1;
		FRHIRenderPassInfo RenderPassInfo(NumRenderTargets, RenderTargets, ERenderTargetActions::Load_Store);
		RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("RuntimeMeshPaintGPUProjectedBrush"));
		RHICmdList.SetViewport(0.0f, 0.0f, 0.0f, RenderTargetSize.X, RenderTargetSize.Y, 1.0f);

		FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
		TShaderMapRef<FRuntimeMeshPaintGPUProjectedBrushVS> VertexShader(ShaderMap);

		FRuntimeMeshPaintGPUProjectedBrushPS::FPermutationDomain PermutationVector;
		PermutationVector.Set<FRuntimeMeshPaintGPUProjectedBrushPS::FWriteMaterialSettings>(MaterialSettingsTexture != nullptr);
		TShaderMapRef<FRuntimeMeshPaintGPUProjectedBrushPS> PixelShader(ShaderMap, PermutationVector);

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.BlendState = bErase
			? TStaticBlendState<
				CW_RGBA, BO_Add, BF_Zero, BF_One, BO_Add, BF_Zero, BF_InverseSourceAlpha,
				CW_BLUE, BO_Add, BF_Zero, BF_InverseSourceAlpha, BO_Add, BF_Zero, BF_One>::GetRHI()
			: TStaticBlendState<
				CW_RGBA, BO_Add, BF_One, BF_Zero, BO_Add, BF_One, BF_InverseSourceAlpha,
				CW_RGB, BO_Add, BF_One, BF_Zero, BO_Add, BF_One, BF_Zero>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
			GRuntimeMeshPaintGPUProjectedBrushVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

		FRuntimeMeshPaintGPUProjectedBrushVS::FParameters VertexParameters;
		VertexParameters.LocalToWorld = LocalToWorld;
		VertexParameters.bSkeletal = bSkeletal ? 1.0f : 0.0f;
		VertexParameters.PaintUVPadding = FVector2f(
			RuntimeMeshPaintProjectedBrushSeamPaddingPixels / FMath::Max(RenderTargetSize.X, 1),
			RuntimeMeshPaintProjectedBrushSeamPaddingPixels / FMath::Max(RenderTargetSize.Y, 1));
		for (int32 BoneIndex = 0; BoneIndex < RuntimeMeshPaintProjectedBrushMaxBones; ++BoneIndex)
		{
			VertexParameters.BoneMatrices[BoneIndex] = BoneMatrices.IsValidIndex(BoneIndex)
				? BoneMatrices[BoneIndex]
				: FMatrix44f::Identity;
		}
		SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), VertexParameters);

		FRuntimeMeshPaintGPUProjectedBrushPS::FParameters PixelParameters;
		PixelParameters.BrushColor = FVector4f(
			Color.R,
			Color.G,
			Color.B,
			Color.A);
		PixelParameters.MaterialSettingsColor = FVector4f(
			MaterialSettings.R,
			MaterialSettings.G,
			MaterialSettings.B,
			MaterialSettings.A);
		PixelParameters.BrushRayStart = FVector3f(BrushRayStart);
		PixelParameters.BrushRayDirection = FVector3f(RayDirection);
		PixelParameters.BrushRayLength = RayLength;
		PixelParameters.BrushRayCenterT = RayCenterT;
		PixelParameters.BrushProjectionDepth = ProjectionDepth;
		PixelParameters.BrushWorldCenter = FVector3f(BrushWorldCenter);
		PixelParameters.BrushWorldNormal = FVector3f(BrushWorldNormal.GetSafeNormal(SMALL_NUMBER, -RayDirection));
		PixelParameters.BrushWorldRadius = FMath::Max(BrushWorldRadius, KINDA_SMALL_NUMBER);
		PixelParameters.NormalThreshold = EffectiveNormalThreshold;
		PixelParameters.bUseBrushScreenProjection = BrushProjectionParameters.bUseScreenProjection;
		PixelParameters.BrushViewOrigin = BrushProjectionParameters.ViewOrigin;
		PixelParameters.BrushViewDirection = BrushProjectionParameters.ViewDirection;
		PixelParameters.BrushViewRight = BrushProjectionParameters.ViewRight;
		PixelParameters.BrushViewUp = BrushProjectionParameters.ViewUp;
		PixelParameters.BrushVisibilityDepthTexture = VisibilityDepthTexture;
		PixelParameters.BrushVisibilityDepthSampler =
			TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PixelParameters.BrushVisibilityDepthInvSize = FVector2f(
			1.0f / FMath::Max(VisibilitySize.X, 1),
			1.0f / FMath::Max(VisibilitySize.Y, 1));
		PixelParameters.BrushVisibilityDepthTolerance = RuntimeMeshPaintProjectedBrushVisibilityDepthTolerance;
		SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PixelParameters);

		RHICmdList.SetStreamSource(0, GPUResource->VertexBufferRHI, 0);
		RHICmdList.DrawPrimitive(0, GPUResource->VertexCount / 3, 1);
		RHICmdList.EndRenderPass();
	}

	void DrawGPUProjectedPreviewMaskPrimitives(
		FRHICommandListImmediate& RHICmdList,
		const TSharedPtr<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe>& GPUResource,
		FTextureRenderTargetResource* PreviewMaskResource,
		const FIntPoint& RenderTargetSize,
		const FMatrix44f& LocalToWorld,
		bool bSkeletal,
		const TArray<FMatrix44f>& BoneMatrices,
		const FVector& BrushRayStart,
		const FVector& BrushRayEnd,
		const FVector& BrushWorldCenter,
		const FVector& BrushWorldNormal,
		float BrushWorldRadius,
		const FRuntimeMeshPaintScreenProjectionData& ProjectionData,
		const FLinearColor& PreviewColor,
		float PreviewLineThickness)
	{
		if (!PreviewMaskResource ||
			!GPUResource.IsValid() ||
			!GPUResource->VertexBufferRHI.IsValid() ||
			GPUResource->VertexCount < 3)
		{
			return;
		}

		FRHITexture* PreviewMaskTexture = PreviewMaskResource->GetRenderTargetTexture();
		if (!PreviewMaskTexture) return;

		const FVector RayVector = BrushRayEnd - BrushRayStart;
		const float RayLength = FMath::Max(static_cast<float>(RayVector.Size()), KINDA_SMALL_NUMBER);
		const FVector RayDirection = RayVector / RayLength;
		const float RayCenterT = FMath::Clamp(
			static_cast<float>(FVector::DotProduct(BrushWorldCenter - BrushRayStart, RayDirection)),
			0.0f,
			RayLength);
		const float ProjectionDepth = FMath::Max(
			FMath::Max(BrushWorldRadius, KINDA_SMALL_NUMBER) * RuntimeMeshPaintProjectedPreviewDepthRadiusScale,
			RuntimeMeshPaintProjectedPreviewMinDepth);
		const float EffectiveNormalThreshold = bSkeletal
			? RuntimeMeshPaintProjectedSkeletalPreviewNormalThreshold
			: RuntimeMeshPaintProjectedPreviewNormalThreshold;
		const FRuntimeMeshPaintGPUBrushProjectionParameters BrushProjectionParameters =
			MakeGPUBrushProjectionParameters(ProjectionData, BrushWorldCenter, BrushWorldRadius);
		const FIntPoint VisibilitySize(
			RuntimeMeshPaintProjectedBrushVisibilitySize,
			RuntimeMeshPaintProjectedBrushVisibilitySize);
		FTextureRHIRef VisibilityDepthTexture = DrawGPUProjectedBrushVisibilityPrimitives(
			RHICmdList,
			GPUResource,
			VisibilitySize,
			LocalToWorld,
			bSkeletal,
			BoneMatrices,
			BrushRayStart,
			RayDirection,
			RayLength,
			RayCenterT,
			ProjectionDepth,
			BrushWorldCenter,
			BrushWorldNormal,
			BrushWorldRadius,
			EffectiveNormalThreshold,
			BrushProjectionParameters);
		if (!VisibilityDepthTexture.IsValid())
		{
			return;
		}

		FRHIRenderPassInfo RenderPassInfo(PreviewMaskTexture, ERenderTargetActions::Clear_Store);
		RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("RuntimeMeshPaintGPUProjectedPreviewMask"));
		RHICmdList.SetViewport(0.0f, 0.0f, 0.0f, RenderTargetSize.X, RenderTargetSize.Y, 1.0f);

		FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
		TShaderMapRef<FRuntimeMeshPaintGPUProjectedBrushVS> VertexShader(ShaderMap);
		TShaderMapRef<FRuntimeMeshPaintGPUProjectedPreviewPS> PixelShader(ShaderMap);

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
			GRuntimeMeshPaintGPUProjectedBrushVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

		FRuntimeMeshPaintGPUProjectedBrushVS::FParameters VertexParameters;
		VertexParameters.LocalToWorld = LocalToWorld;
		VertexParameters.bSkeletal = bSkeletal ? 1.0f : 0.0f;
		VertexParameters.PaintUVPadding = FVector2f(
			RuntimeMeshPaintProjectedBrushSeamPaddingPixels / FMath::Max(RenderTargetSize.X, 1),
			RuntimeMeshPaintProjectedBrushSeamPaddingPixels / FMath::Max(RenderTargetSize.Y, 1));
		for (int32 BoneIndex = 0; BoneIndex < RuntimeMeshPaintProjectedBrushMaxBones; ++BoneIndex)
		{
			VertexParameters.BoneMatrices[BoneIndex] = BoneMatrices.IsValidIndex(BoneIndex)
				? BoneMatrices[BoneIndex]
				: FMatrix44f::Identity;
		}
		SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), VertexParameters);

		FRuntimeMeshPaintGPUProjectedPreviewPS::FParameters PixelParameters;
		PixelParameters.BrushRayStart = FVector3f(BrushRayStart);
		PixelParameters.BrushRayDirection = FVector3f(RayDirection);
		PixelParameters.BrushRayLength = RayLength;
		PixelParameters.BrushRayCenterT = RayCenterT;
		PixelParameters.BrushProjectionDepth = ProjectionDepth;
		PixelParameters.BrushWorldCenter = FVector3f(BrushWorldCenter);
		PixelParameters.BrushWorldNormal = FVector3f(BrushWorldNormal.GetSafeNormal(SMALL_NUMBER, -RayDirection));
		PixelParameters.BrushWorldRadius = FMath::Max(BrushWorldRadius, KINDA_SMALL_NUMBER);
		PixelParameters.NormalThreshold = EffectiveNormalThreshold;
		PixelParameters.bUseBrushScreenProjection = BrushProjectionParameters.bUseScreenProjection;
		PixelParameters.BrushViewOrigin = BrushProjectionParameters.ViewOrigin;
		PixelParameters.BrushViewDirection = BrushProjectionParameters.ViewDirection;
		PixelParameters.BrushViewRight = BrushProjectionParameters.ViewRight;
		PixelParameters.BrushViewUp = BrushProjectionParameters.ViewUp;
		PixelParameters.PreviewLineThickness = PreviewLineThickness;
		PixelParameters.PreviewColor = FVector4f(PreviewColor.R, PreviewColor.G, PreviewColor.B, PreviewColor.A);
		PixelParameters.BrushVisibilityDepthTexture = VisibilityDepthTexture;
		PixelParameters.BrushVisibilityDepthSampler =
			TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PixelParameters.BrushVisibilityDepthInvSize = FVector2f(
			1.0f / FMath::Max(VisibilitySize.X, 1),
			1.0f / FMath::Max(VisibilitySize.Y, 1));
		PixelParameters.BrushVisibilityDepthTolerance = RuntimeMeshPaintProjectedBrushVisibilityDepthTolerance;
		SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PixelParameters);

		RHICmdList.SetStreamSource(0, GPUResource->VertexBufferRHI, 0);
		RHICmdList.DrawPrimitive(0, GPUResource->VertexCount / 3, 1);
		RHICmdList.EndRenderPass();
	}

	void EnqueueProjectedGPUBrushDraw(
		const TSharedPtr<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe>& GPUResource,
		UMeshComponent* MeshComponent,
		UTextureRenderTarget2D* ColorRenderTarget,
		UTextureRenderTarget2D* MaterialSettingsRenderTarget,
		const FVector& BrushRayStart,
		const FVector& BrushRayEnd,
		const FVector& BrushWorldCenter,
		const FVector& BrushWorldNormal,
		float BrushWorldRadius,
		const FRuntimeMeshPaintScreenProjectionData& ProjectionData,
		const FMeshPaintBrushMaterialSettings& BrushSettings)
	{
		FTextureRenderTargetResource* ColorResource = ColorRenderTarget->GameThread_GetRenderTargetResource();
		FTextureRenderTargetResource* MaterialSettingsResource = MaterialSettingsRenderTarget
			? MaterialSettingsRenderTarget->GameThread_GetRenderTargetResource()
			: nullptr;
		const FIntPoint RenderTargetSize(ColorRenderTarget->SizeX, ColorRenderTarget->SizeY);
		const FMatrix44f LocalToWorld(MeshComponent->GetComponentTransform().ToMatrixWithScale());
		const bool bSkeletal = Cast<USkeletalMeshComponent>(MeshComponent) != nullptr;
		TArray<FMatrix44f> BoneMatrices;
		if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
		{
			SkeletalMeshComponent->CacheRefToLocalMatrices(BoneMatrices);
			if (BoneMatrices.Num() > RuntimeMeshPaintProjectedBrushMaxBones)
			{
				BoneMatrices.SetNum(RuntimeMeshPaintProjectedBrushMaxBones, EAllowShrinking::No);
			}
		}
		const FLinearColor Color = BrushSettings.Color;
		const FLinearColor MaterialSettings(BrushSettings.Metallic, BrushSettings.Roughness, 0.0f, 1.0f);
		const float SafeBrushWorldRadius = FMath::Max(BrushWorldRadius, KINDA_SMALL_NUMBER);
		const bool bErase = BrushSettings.bErase;

		ENQUEUE_RENDER_COMMAND(RuntimeMeshPaintProjectedGPUBrushDraw)(
			[GPUResource,
			 ColorResource,
			 MaterialSettingsResource,
			 RenderTargetSize,
			 LocalToWorld,
			 bSkeletal,
			 BoneMatrices = MoveTemp(BoneMatrices),
			 BrushRayStart,
			 BrushRayEnd,
			 BrushWorldCenter,
			 BrushWorldNormal,
			 SafeBrushWorldRadius,
			 ProjectionData,
			 Color,
			 MaterialSettings,
			 bErase](FRHICommandListImmediate& RHICmdList)
			{
				DrawGPUProjectedBrushPrimitives(
					RHICmdList,
					GPUResource,
					ColorResource,
					MaterialSettingsResource,
					RenderTargetSize,
					LocalToWorld,
					bSkeletal,
					BoneMatrices,
					BrushRayStart,
					BrushRayEnd,
					BrushWorldCenter,
					BrushWorldNormal,
					SafeBrushWorldRadius,
					ProjectionData,
					Color,
					MaterialSettings,
					bErase);
			});
	}

	void EnqueueProjectedGPUBrushPreviewMaskDraw(
		const TSharedPtr<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe>& GPUResource,
		UMeshComponent* MeshComponent,
		UTextureRenderTarget2D* PreviewMaskRenderTarget,
		const FVector& BrushRayStart,
		const FVector& BrushRayEnd,
		const FVector& BrushWorldCenter,
		const FVector& BrushWorldNormal,
		float BrushWorldRadius,
		const FRuntimeMeshPaintScreenProjectionData& ProjectionData,
		const FLinearColor& PreviewColor,
		float PreviewLineThickness)
	{
		FTextureRenderTargetResource* PreviewMaskResource = PreviewMaskRenderTarget->GameThread_GetRenderTargetResource();
		const FIntPoint RenderTargetSize(PreviewMaskRenderTarget->SizeX, PreviewMaskRenderTarget->SizeY);
		const FMatrix44f LocalToWorld(MeshComponent->GetComponentTransform().ToMatrixWithScale());
		const bool bSkeletal = Cast<USkeletalMeshComponent>(MeshComponent) != nullptr;
		TArray<FMatrix44f> BoneMatrices;
		if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
		{
			SkeletalMeshComponent->CacheRefToLocalMatrices(BoneMatrices);
			if (BoneMatrices.Num() > RuntimeMeshPaintProjectedBrushMaxBones)
			{
				BoneMatrices.SetNum(RuntimeMeshPaintProjectedBrushMaxBones, EAllowShrinking::No);
			}
		}

		const float SafeBrushWorldRadius = FMath::Max(BrushWorldRadius, KINDA_SMALL_NUMBER);
		const FLinearColor SafePreviewColor(
			PreviewColor.R,
			PreviewColor.G,
			PreviewColor.B,
			FMath::Clamp(PreviewColor.A, 0.0f, 1.0f));
		const float SafePreviewLineThickness = FMath::Max(PreviewLineThickness, 0.1f);

		ENQUEUE_RENDER_COMMAND(RuntimeMeshPaintProjectedGPUPreviewMaskDraw)(
			[GPUResource,
			 PreviewMaskResource,
			 RenderTargetSize,
			 LocalToWorld,
			 bSkeletal,
			 BoneMatrices = MoveTemp(BoneMatrices),
			 BrushRayStart,
			 BrushRayEnd,
			 BrushWorldCenter,
			 BrushWorldNormal,
			 SafeBrushWorldRadius,
			 ProjectionData,
			 SafePreviewColor,
			 SafePreviewLineThickness](FRHICommandListImmediate& RHICmdList)
			{
				DrawGPUProjectedPreviewMaskPrimitives(
					RHICmdList,
					GPUResource,
					PreviewMaskResource,
					RenderTargetSize,
					LocalToWorld,
					bSkeletal,
					BoneMatrices,
					BrushRayStart,
					BrushRayEnd,
					BrushWorldCenter,
					BrushWorldNormal,
					SafeBrushWorldRadius,
					ProjectionData,
					SafePreviewColor,
					SafePreviewLineThickness);
			});
	}
}

bool FRuntimeMeshPaintGPUBrushRenderer::PrecacheProjectedMeshResource(
	UMeshComponent* MeshComponent,
	int32 UVChannel,
	float UVIslandConnectionTolerance)
{
	TSharedPtr<FPaintUVCache> Cache = FRuntimeMeshPaintUVCache::FindOrBuildPaintUVCache(
		MeshComponent,
		UVChannel,
		UVIslandConnectionTolerance);
	if (!Cache.IsValid() || Cache->Triangles.Num() == 0) return false;

	TSharedPtr<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe> GPUResource =
		FindOrCreateGPUProjectedBrushResource(MeshComponent, Cache);
	return GPUResource.IsValid() && GPUResource->VertexCount >= 3;
}

bool FRuntimeMeshPaintGPUBrushRenderer::DrawProjectedBrush(
	UMeshComponent* MeshComponent,
	UTextureRenderTarget2D* ColorRenderTarget,
	UTextureRenderTarget2D* MaterialSettingsRenderTarget,
	int32 UVChannel,
	float UVIslandConnectionTolerance,
	const FVector& BrushRayStart,
	const FVector& BrushRayEnd,
	const FVector& BrushWorldCenter,
	const FVector& BrushWorldNormal,
	float BrushWorldRadius,
	const FRuntimeMeshPaintScreenProjectionData& ProjectionData,
	const FMeshPaintBrushMaterialSettings& BrushSettings)
{
	if (!MeshComponent ||
		!ColorRenderTarget ||
		ColorRenderTarget->SizeX <= 0 ||
		ColorRenderTarget->SizeY <= 0 ||
		BrushWorldRadius <= 0.0f)
	{
		return false;
	}

	if (MaterialSettingsRenderTarget &&
		(MaterialSettingsRenderTarget->SizeX != ColorRenderTarget->SizeX ||
			MaterialSettingsRenderTarget->SizeY != ColorRenderTarget->SizeY))
	{
		return false;
	}

	TSharedPtr<FPaintUVCache> Cache = FRuntimeMeshPaintUVCache::FindOrBuildPaintUVCache(
		MeshComponent,
		UVChannel,
		UVIslandConnectionTolerance);
	if (!Cache.IsValid() || Cache->Triangles.Num() == 0) return false;

	TSharedPtr<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe> GPUResource =
		FindOrCreateGPUProjectedBrushResource(MeshComponent, Cache);
	if (!GPUResource.IsValid() || GPUResource->VertexCount < 3) return false;

	EnqueueProjectedGPUBrushDraw(
		GPUResource,
		MeshComponent,
		ColorRenderTarget,
		MaterialSettingsRenderTarget,
		BrushRayStart,
		BrushRayEnd,
		BrushWorldCenter,
		BrushWorldNormal,
		BrushWorldRadius,
		ProjectionData,
		BrushSettings);
	return true;
}

bool FRuntimeMeshPaintGPUBrushRenderer::DrawProjectedBrushPreviewMask(
	UMeshComponent* MeshComponent,
	UTextureRenderTarget2D* PreviewMaskRenderTarget,
	int32 UVChannel,
	float UVIslandConnectionTolerance,
	const FVector& BrushRayStart,
	const FVector& BrushRayEnd,
	const FVector& BrushWorldCenter,
	const FVector& BrushWorldNormal,
	float BrushWorldRadius,
	const FRuntimeMeshPaintScreenProjectionData& ProjectionData,
	const FLinearColor& PreviewColor,
	float PreviewLineThickness)
{
	if (!MeshComponent ||
		!PreviewMaskRenderTarget ||
		PreviewMaskRenderTarget->SizeX <= 0 ||
		PreviewMaskRenderTarget->SizeY <= 0 ||
		BrushWorldRadius <= 0.0f ||
		PreviewColor.A <= 0.0f)
	{
		return false;
	}

	TSharedPtr<FPaintUVCache> Cache = FRuntimeMeshPaintUVCache::FindOrBuildPaintUVCache(
		MeshComponent,
		UVChannel,
		UVIslandConnectionTolerance);
	if (!Cache.IsValid() || Cache->Triangles.Num() == 0) return false;

	TSharedPtr<FRuntimeMeshPaintGPUProjectedBrushMeshResource, ESPMode::ThreadSafe> GPUResource =
		FindOrCreateGPUProjectedBrushResource(MeshComponent, Cache);
	if (!GPUResource.IsValid() || GPUResource->VertexCount < 3) return false;

	EnqueueProjectedGPUBrushPreviewMaskDraw(
		GPUResource,
		MeshComponent,
		PreviewMaskRenderTarget,
		BrushRayStart,
		BrushRayEnd,
		BrushWorldCenter,
		BrushWorldNormal,
		BrushWorldRadius,
		ProjectionData,
		PreviewColor,
		PreviewLineThickness);
	return true;
}
