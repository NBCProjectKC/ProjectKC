// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Painting/RuntimeMeshPaintTargetComponent.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Compression.h"
#include "Misc/Paths.h"
#include "GPU/RuntimeMeshPaintGPUBrushRenderer.h"
#include "Hit/RuntimeMeshPaintHitUtils.h"
#include "Hit/RuntimeMeshPaintHitUtilsInternal.h"
#include "Net/UnrealNetwork.h"
#include "Serialization/BufferArchive.h"
#include "TextureResource.h"
#include "../Core/MeshPaintingCoreStats.h"

namespace
{
	constexpr float RuntimeMeshPaintTraceDistance = 100000.0f;
	constexpr float RuntimeMeshPaintTargetBrushSizeToWorldRadiusScale = 100.0f;
	constexpr int32 RuntimeMeshPaintPatchHistoryVersion = 1;

	void ConfigurePaintRenderTarget(UTextureRenderTarget2D* RenderTarget)
	{
		if (!RenderTarget) return;

		RenderTarget->AddressX = TA_Clamp;
		RenderTarget->AddressY = TA_Clamp;
		RenderTarget->bAutoGenerateMips = false;
	}

	FLinearColor MakeNeutralTransparentPaintClearColor()
	{
		return FLinearColor(1.0f, 1.0f, 1.0f, 0.0f);
	}

	FLinearColor MakeMaterialSettingsClearColor(const FLinearColor& InitialMaterialSettingsColor)
	{
		FLinearColor MaterialSettingsClearColor = InitialMaterialSettingsColor;
		MaterialSettingsClearColor.B = 0.0f;
		MaterialSettingsClearColor.A = 0.0f;
		return MaterialSettingsClearColor;
	}

	int32 GetPatchStoredBytes(const FRuntimeMeshPaintPatchHistoryEntry& Entry)
	{
		return Entry.PixelBytes.Num();
	}

	bool CopyColorsToBytes(const TArray<FColor>& Colors, TArray<uint8>& OutBytes)
	{
		const int64 ByteCount64 = static_cast<int64>(Colors.Num()) * static_cast<int64>(sizeof(FColor));
		if (ByteCount64 < 0 || ByteCount64 > MAX_int32) return false;

		OutBytes.Reset();
		OutBytes.SetNumUninitialized(static_cast<int32>(ByteCount64));
		if (OutBytes.Num() > 0)
		{
			FMemory::Memcpy(OutBytes.GetData(), Colors.GetData(), OutBytes.Num());
		}
		return true;
	}

	bool CompressPatchColorBytes(const TArray<FColor>& Colors, bool bAllowCompression, TArray<uint8>& OutBytes, bool& bOutCompressed)
	{
		bOutCompressed = false;
		TArray<uint8> RawBytes;
		if (!CopyColorsToBytes(Colors, RawBytes)) return false;

		if (!bAllowCompression || RawBytes.Num() == 0)
		{
			OutBytes = MoveTemp(RawBytes);
			return true;
		}

		int64 CompressedCapacity = 0;
		if (!FCompression::CompressMemoryBound(NAME_Zlib, CompressedCapacity, RawBytes.Num()) ||
			CompressedCapacity <= 0 ||
			CompressedCapacity > MAX_int32)
		{
			OutBytes = MoveTemp(RawBytes);
			return true;
		}

		TArray<uint8> CompressedBytes;
		CompressedBytes.SetNumUninitialized(static_cast<int32>(CompressedCapacity));
		int64 CompressedSize = CompressedBytes.Num();
		if (!FCompression::CompressMemory(
			NAME_Zlib,
			CompressedBytes.GetData(),
			CompressedSize,
			RawBytes.GetData(),
			RawBytes.Num(),
			COMPRESS_BiasMemory) ||
			CompressedSize <= 0 ||
			CompressedSize >= RawBytes.Num())
		{
			OutBytes = MoveTemp(RawBytes);
			return true;
		}

		CompressedBytes.SetNum(static_cast<int32>(CompressedSize), EAllowShrinking::Yes);
		OutBytes = MoveTemp(CompressedBytes);
		bOutCompressed = true;
		return true;
	}

	bool IsColorNearlyEqual(const FColor& A, const FColor& B, int32 Tolerance = 1)
	{
		return FMath::Abs(static_cast<int32>(A.R) - static_cast<int32>(B.R)) <= Tolerance &&
			FMath::Abs(static_cast<int32>(A.G) - static_cast<int32>(B.G)) <= Tolerance &&
			FMath::Abs(static_cast<int32>(A.B) - static_cast<int32>(B.B)) <= Tolerance &&
			FMath::Abs(static_cast<int32>(A.A) - static_cast<int32>(B.A)) <= Tolerance;
	}

	bool IsNeutralPatchPixel(
		const FColor& Pixel,
		ERuntimeMeshPaintPatchTextureType TextureType,
		const FLinearColor& InitialMaterialSettingsColor)
	{
		if (TextureType == ERuntimeMeshPaintPatchTextureType::Color)
		{
			return Pixel.A <= 1;
		}

		return IsColorNearlyEqual(
			Pixel,
			MakeMaterialSettingsClearColor(InitialMaterialSettingsColor).ToFColor(false));
	}

	const TCHAR* GetPaintTextureExportFormatExtension(ERuntimeMeshPaintTextureExportFormat ExportFormat)
	{
		switch (ExportFormat)
		{
		case ERuntimeMeshPaintTextureExportFormat::EXR:
			return TEXT("exr");
		case ERuntimeMeshPaintTextureExportFormat::HDR:
			return TEXT("hdr");
		case ERuntimeMeshPaintTextureExportFormat::PNG:
		default:
			return TEXT("png");
		}
	}

	const TCHAR* GetPaintTextureTypeSuffix(ERuntimeMeshPaintPatchTextureType TextureType)
	{
		return TextureType == ERuntimeMeshPaintPatchTextureType::MaterialSettings
			? TEXT("MaterialSettings")
			: TEXT("Color");
	}

	FString MakeSafePaintTextureFileName(
		const FString& FileName,
		ERuntimeMeshPaintTextureExportFormat ExportFormat)
	{
		FString CleanBaseName = FPaths::GetBaseFilename(FPaths::GetCleanFilename(FileName)).TrimStartAndEnd();
		if (CleanBaseName.IsEmpty())
		{
			CleanBaseName = TEXT("PaintedTexture");
		}

		CleanBaseName = FPaths::MakeValidFileName(CleanBaseName, TEXT('_'));
		return FString::Printf(
			TEXT("%s.%s"),
			*CleanBaseName,
			GetPaintTextureExportFormatExtension(ExportFormat));
	}

	FString ResolvePaintTextureExportDirectory(const FString& DirectoryPath)
	{
		FString ResolvedDirectory = DirectoryPath.TrimStartAndEnd();
		if (ResolvedDirectory.IsEmpty())
		{
			ResolvedDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeMeshPainting"), TEXT("Exports"));
		}

		FPaths::NormalizeDirectoryName(ResolvedDirectory);
		return ResolvedDirectory;
	}

	uint16 QuantizeRuntimeMeshPaintUV(float Value)
	{
		return static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(Value * 65535.0f), 0, 65535));
	}

	float DequantizeRuntimeMeshPaintUV(uint16 Value)
	{
		return Value / 65535.0f;
	}

	bool IsFiniteRuntimeMeshPaintVector(const FVector& Vector)
	{
		return FMath::IsFinite(Vector.X) &&
			FMath::IsFinite(Vector.Y) &&
			FMath::IsFinite(Vector.Z);
	}

	bool IsValidRuntimeMeshPaintScreenProjectionData(const FRuntimeMeshPaintNetCommand& Command)
	{
		if (!Command.ProjectionData.bHasScreenProjection) return false;

		const FVector RayVector = FVector(Command.BrushRayEnd) - FVector(Command.BrushRayStart);
		const FVector RayDirection = RayVector.GetSafeNormal();
		if (RayDirection.IsNearlyZero()) return false;

		const FRuntimeMeshPaintScreenProjectionData& ProjectionData = Command.ProjectionData;
		if (!IsFiniteRuntimeMeshPaintVector(ProjectionData.ViewOrigin) ||
			!IsFiniteRuntimeMeshPaintVector(ProjectionData.ViewForward) ||
			!IsFiniteRuntimeMeshPaintVector(ProjectionData.ViewRight) ||
			!IsFiniteRuntimeMeshPaintVector(ProjectionData.ViewUp))
		{
			return false;
		}

		const FVector ViewForward = ProjectionData.ViewForward.GetSafeNormal();
		const FVector ViewRight = ProjectionData.ViewRight.GetSafeNormal();
		const FVector ViewUp = ProjectionData.ViewUp.GetSafeNormal();
		if (ViewForward.IsNearlyZero() || ViewRight.IsNearlyZero() || ViewUp.IsNearlyZero()) return false;
		if (FVector::DotProduct(ViewForward, RayDirection) < 0.5f) return false;

		constexpr double MaxProjectionOriginDistance = 1000.0;
		if (FVector::DistSquared(ProjectionData.ViewOrigin, FVector(Command.BrushRayStart)) >
			FMath::Square(MaxProjectionOriginDistance))
		{
			return false;
		}

		return true;
	}

	bool BuildRuntimeMeshPaintScreenProjectionData(
		APlayerController* PlayerController,
		FRuntimeMeshPaintScreenProjectionData& OutProjectionData)
	{
		OutProjectionData = FRuntimeMeshPaintScreenProjectionData();
		if (!PlayerController) return false;

		float MouseX = 0.0f;
		float MouseY = 0.0f;
		if (!PlayerController->GetMousePosition(MouseX, MouseY)) return false;

		FVector ViewLocation = FVector::ZeroVector;
		FRotator ViewRotation = FRotator::ZeroRotator;
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
		const FRotationMatrix ViewRotationMatrix(ViewRotation);
		FVector CursorRayOrigin = FVector::ZeroVector;
		FVector CursorRayDirection = FVector::ForwardVector;
		if (!PlayerController->DeprojectMousePositionToWorld(CursorRayOrigin, CursorRayDirection))
		{
			return false;
		}

		CursorRayDirection = CursorRayDirection.GetSafeNormal(SMALL_NUMBER, ViewRotationMatrix.GetScaledAxis(EAxis::X));
		const FVector CameraRight = ViewRotationMatrix.GetScaledAxis(EAxis::Y);
		const FVector CameraUp = ViewRotationMatrix.GetScaledAxis(EAxis::Z);
		FVector BrushRight = CameraRight - CursorRayDirection * FVector::DotProduct(CameraRight, CursorRayDirection);
		BrushRight = BrushRight.GetSafeNormal();
		if (BrushRight.IsNearlyZero())
		{
			FVector FallbackUp = FVector::UpVector;
			CursorRayDirection.FindBestAxisVectors(BrushRight, FallbackUp);
		}

		FVector BrushUp = FVector::CrossProduct(CursorRayDirection, BrushRight).GetSafeNormal(SMALL_NUMBER, CameraUp);
		if (FVector::DotProduct(BrushUp, CameraUp) < 0.0)
		{
			BrushUp *= -1.0;
			BrushRight *= -1.0;
		}

		OutProjectionData.bHasScreenProjection = true;
		OutProjectionData.ViewOrigin = CursorRayOrigin;
		OutProjectionData.ViewForward = CursorRayDirection;
		OutProjectionData.ViewRight = BrushRight;
		OutProjectionData.ViewUp = BrushUp;
		return true;
	}

	bool ResolvePaintUVIslandCache(
		UMeshComponent* MeshComponent,
		int32 UVChannel,
		float UVConnectionTolerance,
		int32 ResolvedTriangleArrayIndex,
		int32 FaceIndex,
		const FVector2D& HitUV,
		TSharedPtr<FPaintUVCache>& OutCache,
		int32& OutIslandId)
	{
		OutCache.Reset();
		OutIslandId = INDEX_NONE;

		TSharedPtr<FPaintUVCache> Cache = FRuntimeMeshPaintUVCache::FindOrBuildPaintUVCache(
			MeshComponent,
			UVChannel,
			UVConnectionTolerance);
		if (!Cache.IsValid()) return false;

		int32 SeedTriangleArrayIndex = INDEX_NONE;
		if (Cache->Triangles.IsValidIndex(ResolvedTriangleArrayIndex))
		{
			SeedTriangleArrayIndex = ResolvedTriangleArrayIndex;
		}
		else if (!FRuntimeMeshPaintUVCache::FindSeedTriangleArrayIndex(*Cache, FaceIndex, HitUV, SeedTriangleArrayIndex))
		{
			return false;
		}

		if (!Cache->TriangleIslandIds.IsValidIndex(SeedTriangleArrayIndex))
		{
			return false;
		}

		const int32 IslandId = Cache->TriangleIslandIds[SeedTriangleArrayIndex];
		if (!Cache->Islands.IsValidIndex(IslandId))
		{
			return false;
		}

		OutCache = Cache;
		OutIslandId = IslandId;
		return true;
	}

}

URuntimeMeshPaintTargetComponent::URuntimeMeshPaintTargetComponent()
	: PaintedColorRenderTarget(nullptr)
	, PaintedMaterialSettingsRenderTarget(nullptr)
	, BrushPreviewMaskRenderTarget(nullptr)
	, TargetMesh(nullptr)
	, bCreatePaintedMaterialSettingsRenderTarget(true)
	, RuntimeRenderTargetWidth(1024)
	, RuntimeRenderTargetHeight(1024)
	, RuntimeRenderTargetFormat(RTF_RGBA16f)
	, InitialPaintColor(1.0f, 1.0f, 1.0f, 0.0f)
	, InitialMaterialSettingsColor(0.0f, 0.5f, 0.0f, 0.0f)
	, PaintedColorTextureParameterName(TEXT("PaintedColorTexture"))
	, PaintedMaterialSettingsTextureParameterName(TEXT("PaintedMaterialSettingsTexture"))
	, BrushPreviewMaskTextureParameterName(TEXT("BrushPreviewMaskTexture"))
	, UVChannel(0)
	, MaxSkeletalMeshUVFallbackDistance(0.0f)
	, bClipBrushToUVIsland(true)
	, UVIslandConnectionTolerance(0.0001f)
	, bReplicateRuntimePaint(true)
	, bAutoEnableOwnerReplication(true)
	, MaxReplicatedPaintCommands(0)
	, ReplicatedPaintReplayCommandsPerTick(8)
	, MaxReplicatedPaintDistance(0.0f)
	, MaxReplicatedBrushSize(4.0f)
	, bRecordPaintPatchHistory(true)
	, PatchCaptureBudgetPerTick(0)
	, MaxPatchHistoryEntries(0)
	, MaxPatchHistoryBytes(0)
	, PatchHistoryDirtyRectBrushSizeMultiplier(1.25f)
	, PatchHistoryDirtyRectPaddingPixels(8)
	, bCompressPatchHistory(true)
	, PatchHistoryBytes(0)
	, NextReplicatedPaintCommandId(0)
	, PendingReplicatedPaintCommandReadIndex(0)
	, NextPatchHistorySequenceId(0)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
	ReplicatedPaintHistory.SetOwner(this);
	BrushMaterialSettings.Clamp();
}

void URuntimeMeshPaintTargetComponent::BeginPlay()
{
	Super::BeginPlay();

	ReplicatedPaintHistory.SetOwner(this);

	if (bReplicateRuntimePaint)
	{
		SetIsReplicated(true);
		if (bAutoEnableOwnerReplication)
		{
			if (AActor* OwnerActor = GetOwner())
			{
				if (OwnerActor->HasAuthority())
				{
					OwnerActor->SetReplicates(true);
				}
			}
		}
	}

	if (!EnsureTargetMeshForBeginPlay()) return;
	if (GetNetMode() != NM_DedicatedServer) InitializeRuntimePaintTarget();
}

void URuntimeMeshPaintTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PendingReplicatedPaintCommands.Reset();
	AppliedReplicatedPaintCommandIds.Reset();
	PredictedPaintCommandKeys.Reset();
	AcceptedClientPaintPredictionKeys.Reset();
	PendingReplicatedPaintCommandReadIndex = 0;
	PendingPatchCaptureRequests.Reset();
	Super::EndPlay(EndPlayReason);
}

void URuntimeMeshPaintTargetComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ProcessPendingReplicatedPaintCommands();

	if (PendingReplicatedPaintCommands.Num() == 0)
	{
		SetComponentTickEnabled(false);
	}
}

void URuntimeMeshPaintTargetComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URuntimeMeshPaintTargetComponent, ReplicatedPaintHistory);
}

void URuntimeMeshPaintTargetComponent::SetPaintedColorRenderTarget(UTextureRenderTarget2D* InRenderTarget)
{
	ConfigurePaintRenderTarget(InRenderTarget);
	if (InRenderTarget) InRenderTarget->UpdateResourceImmediate(false);

	if (UMeshComponent* PrimaryMeshTarget = GetPrimaryMeshTarget())
	{
		FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData = FindOrAddRuntimeDataForMesh(PrimaryMeshTarget);
		RuntimeData.PaintedColorRenderTarget = InRenderTarget;
		UpdatePrimaryRenderTargetAliases();
	}
	else
	{
		PaintedColorRenderTarget = InRenderTarget;
	}

	ApplyRuntimePaintTexturesToTargetMesh();
}

void URuntimeMeshPaintTargetComponent::SetPaintedMaterialSettingsRenderTarget(UTextureRenderTarget2D* InRenderTarget)
{
	ConfigurePaintRenderTarget(InRenderTarget);
	if (InRenderTarget) InRenderTarget->UpdateResourceImmediate(false);

	if (UMeshComponent* PrimaryMeshTarget = GetPrimaryMeshTarget())
	{
		FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData = FindOrAddRuntimeDataForMesh(PrimaryMeshTarget);
		RuntimeData.PaintedMaterialSettingsRenderTarget = InRenderTarget;
		UpdatePrimaryRenderTargetAliases();
	}
	else
	{
		PaintedMaterialSettingsRenderTarget = InRenderTarget;
	}

	ApplyRuntimePaintTexturesToTargetMesh();
}

bool URuntimeMeshPaintTargetComponent::SetTargetMesh(UMeshComponent* NewTargetMesh)
{
	TargetMesh = NewTargetMesh;
	MeshTargets.Reset();
	if (NewTargetMesh) MeshTargets.Add(MakeMeshTargetString(NewTargetMesh));
	ResetRuntimePaintResources();

	if (!HasValidMeshTarget())
	{
		PaintedColorRenderTarget = nullptr;
		PaintedMaterialSettingsRenderTarget = nullptr;
		BrushPreviewMaskRenderTarget = nullptr;
		return false;
	}

	return InitializeRuntimePaintTarget();
}

void URuntimeMeshPaintTargetComponent::SetMeshTargets(const TArray<UMeshComponent*>& NewMeshTargets)
{
	MeshTargets.Reset(NewMeshTargets.Num());
	for (UMeshComponent* NewMeshTarget : NewMeshTargets)
	{
		if (NewMeshTarget) MeshTargets.Add(MakeMeshTargetString(NewMeshTarget));
	}

	TargetMesh = GetPrimaryMeshTarget();
	ResetRuntimePaintResources();
	if (!HasValidMeshTarget())
	{
		PaintedColorRenderTarget = nullptr;
		PaintedMaterialSettingsRenderTarget = nullptr;
		BrushPreviewMaskRenderTarget = nullptr;
		return;
	}

	InitializeRuntimePaintTarget();
}

bool URuntimeMeshPaintTargetComponent::AddMeshTarget(UMeshComponent* NewMeshTarget)
{
	if (!NewMeshTarget) return false;

	for (const FString& ExistingMeshTarget : MeshTargets)
	{
		if (ResolveMeshTargetString(ExistingMeshTarget) == NewMeshTarget)
		{
			TargetMesh = GetPrimaryMeshTarget();
			return true;
		}
	}

	MeshTargets.Add(MakeMeshTargetString(NewMeshTarget));
	TargetMesh = GetPrimaryMeshTarget();
	ResetRuntimePaintResources();
	return InitializeRuntimePaintTarget();
}

void URuntimeMeshPaintTargetComponent::ClearMeshTargets()
{
	MeshTargets.Reset();
	TargetMesh = nullptr;
	ResetRuntimePaintResources();
	PaintedColorRenderTarget = nullptr;
	PaintedMaterialSettingsRenderTarget = nullptr;
	BrushPreviewMaskRenderTarget = nullptr;
}

TArray<UMeshComponent*> URuntimeMeshPaintTargetComponent::GetMeshTargets() const
{
	TArray<UMeshComponent*> Result;
	Result.Reserve(MeshTargets.Num() + 1);
	for (const FString& MeshTarget : MeshTargets)
	{
		if (UMeshComponent* ResolvedMeshTarget = ResolveMeshTargetString(MeshTarget))
		{
			Result.Add(ResolvedMeshTarget);
		}
	}

	if (Result.Num() == 0 && TargetMesh) Result.Add(TargetMesh.Get());
	return Result;
}

bool URuntimeMeshPaintTargetComponent::HasMeshTarget(UMeshComponent* MeshComponent) const
{
	if (!MeshComponent) return false;

	if (MeshTargets.Num() > 0)
	{
		for (const FString& MeshTarget : MeshTargets)
		{
			UMeshComponent* ResolvedMeshTarget = ResolveMeshTargetString(MeshTarget);
			if (IsValid(ResolvedMeshTarget) && ResolvedMeshTarget == MeshComponent) return true;
		}

		return false;
	}

	return IsValid(TargetMesh.Get()) && TargetMesh.Get() == MeshComponent;
}

bool URuntimeMeshPaintTargetComponent::InitializeRuntimePaintTarget()
{
	if (!HasValidMeshTarget()) return false;
	if (RuntimeRenderTargetWidth <= 0 || RuntimeRenderTargetHeight <= 0) return false;

	MeshRuntimeData.Reset();

	bool bInitializedAnyTarget = false;
	for (UMeshComponent* MeshTarget : GetMeshTargets())
	{
		if (!IsValid(MeshTarget)) continue;

		FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData = FindOrAddRuntimeDataForMesh(MeshTarget);
		bInitializedAnyTarget |= InitializeRuntimeDataForMesh(RuntimeData);
	}

	UpdatePrimaryRenderTargetAliases();
	return bInitializedAnyTarget && ApplyRuntimePaintTexturesToTargetMesh();
}

bool URuntimeMeshPaintTargetComponent::ApplyRuntimePaintTexturesToTargetMesh()
{
	bool bAppliedAnyMaterial = false;

	for (UMeshComponent* MeshTarget : GetMeshTargets())
	{
		if (!IsValid(MeshTarget)) continue;

		FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData = FindOrAddRuntimeDataForMesh(MeshTarget);
		if (!InitializeRuntimeDataForMesh(RuntimeData)) continue;

		RuntimeData.PaintTargetMaterialInstances.Reset();

		const int32 NumMaterials = MeshTarget->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
		{
			UMaterialInterface* CurrentMaterial = MeshTarget->GetMaterial(MaterialIndex);
			UMaterialInstanceDynamic* MaterialInstance = Cast<UMaterialInstanceDynamic>(CurrentMaterial);
			if (!MaterialInstance) MaterialInstance = MeshTarget->CreateDynamicMaterialInstance(MaterialIndex, CurrentMaterial);
			if (!MaterialInstance) continue;
			if (!PaintedColorTextureParameterName.IsNone())
				MaterialInstance->SetTextureParameterValue(PaintedColorTextureParameterName, RuntimeData.PaintedColorRenderTarget);
			if (RuntimeData.PaintedMaterialSettingsRenderTarget && !PaintedMaterialSettingsTextureParameterName.IsNone())
				MaterialInstance->SetTextureParameterValue(
					PaintedMaterialSettingsTextureParameterName, RuntimeData.PaintedMaterialSettingsRenderTarget);
			if (RuntimeData.BrushPreviewMaskRenderTarget && !BrushPreviewMaskTextureParameterName.IsNone())
				MaterialInstance->SetTextureParameterValue(
					BrushPreviewMaskTextureParameterName, RuntimeData.BrushPreviewMaskRenderTarget);

			RuntimeData.PaintTargetMaterialInstances.Add(MaterialInstance);
			bAppliedAnyMaterial = true;
		}
	}

	UpdatePrimaryRenderTargetAliases();
	return bAppliedAnyMaterial;
}

UTextureRenderTarget2D* URuntimeMeshPaintTargetComponent::ResolvePaintedRenderTargetForExport(
	ERuntimeMeshPaintPatchTextureType TextureType,
	UMeshComponent* MeshTarget)
{
	if (GetNetMode() == NM_DedicatedServer) return nullptr;
	if (!HasValidMeshTarget())
	{
		ResolveInitialMeshTargets();
	}

	UMeshComponent* ResolvedMeshTarget = MeshTarget ? MeshTarget : GetPrimaryMeshTarget();
	if (!IsValid(ResolvedMeshTarget)) return nullptr;

	FRuntimeMeshPaintTargetMeshRuntimeData* RuntimeData = FindRuntimeDataForMesh(ResolvedMeshTarget);
	if (!RuntimeData)
	{
		if (!HasMeshTarget(ResolvedMeshTarget)) return nullptr;
		RuntimeData = &FindOrAddRuntimeDataForMesh(ResolvedMeshTarget);
		if (!InitializeRuntimeDataForMesh(*RuntimeData)) return nullptr;
	}

	return TextureType == ERuntimeMeshPaintPatchTextureType::MaterialSettings
		? RuntimeData->PaintedMaterialSettingsRenderTarget
		: RuntimeData->PaintedColorRenderTarget;
}

bool URuntimeMeshPaintTargetComponent::ExportRenderTargetToImageFile(
	UTextureRenderTarget2D* RenderTarget,
	ERuntimeMeshPaintPatchTextureType TextureType,
	const FString& DirectoryPath,
	const FString& FileName,
	ERuntimeMeshPaintTextureExportFormat ExportFormat,
	FString& OutFilePath) const
{
	OutFilePath.Reset();
	if (GetNetMode() == NM_DedicatedServer) return false;
	if (!RenderTarget || !RenderTarget->GetResource()) return false;

	const FString ResolvedDirectory = ResolvePaintTextureExportDirectory(DirectoryPath);
	if (!IFileManager::Get().MakeDirectory(*ResolvedDirectory, true))
	{
		return false;
	}

	const FString SafeFileName = MakeSafePaintTextureFileName(FileName, ExportFormat);
	OutFilePath = FPaths::Combine(ResolvedDirectory, SafeFileName);

	FText PathError;
	if (!FPaths::ValidatePath(OutFilePath, &PathError))
	{
		OutFilePath.Reset();
		return false;
	}

	FBufferArchive Buffer;
	bool bExported = false;
	switch (ExportFormat)
	{
	case ERuntimeMeshPaintTextureExportFormat::EXR:
		bExported = FImageUtils::ExportRenderTarget2DAsEXR(RenderTarget, Buffer);
		break;
	case ERuntimeMeshPaintTextureExportFormat::HDR:
		bExported = FImageUtils::ExportRenderTarget2DAsHDR(RenderTarget, Buffer);
		break;
	case ERuntimeMeshPaintTextureExportFormat::PNG:
	default:
		{
			FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
			if (!RenderTargetResource)
			{
				OutFilePath.Reset();
				return false;
			}

			TArray<FColor> Pixels;
			FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
			ReadFlags.SetLinearToGamma(false);
			if (!RenderTargetResource->ReadPixels(Pixels, ReadFlags) ||
				Pixels.Num() != RenderTarget->SizeX * RenderTarget->SizeY)
			{
				OutFilePath.Reset();
				return false;
			}

			if (TextureType == ERuntimeMeshPaintPatchTextureType::MaterialSettings)
			{
				for (FColor& Pixel : Pixels)
				{
					Pixel.A = 255;
				}
			}

			TArray64<uint8> CompressedPngBytes;
			FImageUtils::PNGCompressImageArray(
				RenderTarget->SizeX,
				RenderTarget->SizeY,
				TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()),
				CompressedPngBytes);

			if (CompressedPngBytes.Num() > 0 && CompressedPngBytes.Num() <= MAX_int32)
			{
				Buffer.Serialize(CompressedPngBytes.GetData(), static_cast<int64>(CompressedPngBytes.Num()));
				bExported = true;
			}
		}
		break;
	}

	if (!bExported || Buffer.Num() <= 0)
	{
		OutFilePath.Reset();
		return false;
	}

	FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*OutFilePath);
	if (!FileWriter)
	{
		OutFilePath.Reset();
		return false;
	}

	FileWriter->Serialize(Buffer.GetData(), Buffer.Num());
	const bool bWriteSucceeded = !FileWriter->IsError();
	FileWriter->Close();
	delete FileWriter;

	if (!bWriteSucceeded)
	{
		IFileManager::Get().Delete(*OutFilePath, false, true);
		OutFilePath.Reset();
		return false;
	}

	return true;
}

bool URuntimeMeshPaintTargetComponent::ExportPaintedRenderTargetToFile(
	const FString& DirectoryPath,
	const FString& FileName,
	FString& OutFilePath,
	ERuntimeMeshPaintPatchTextureType TextureType,
	ERuntimeMeshPaintTextureExportFormat ExportFormat,
	UMeshComponent* MeshTarget)
{
	UTextureRenderTarget2D* RenderTarget = ResolvePaintedRenderTargetForExport(TextureType, MeshTarget);
	return ExportRenderTargetToImageFile(RenderTarget, TextureType, DirectoryPath, FileName, ExportFormat, OutFilePath);
}

int32 URuntimeMeshPaintTargetComponent::ExportAllPaintedRenderTargetsToFiles(
	const FString& DirectoryPath,
	const FString& FileNamePrefix,
	TArray<FString>& OutFilePaths,
	bool bExportColor,
	bool bExportMaterialSettings,
	ERuntimeMeshPaintTextureExportFormat ExportFormat)
{
	OutFilePaths.Reset();
	if (GetNetMode() == NM_DedicatedServer) return 0;
	if (!bExportColor && !bExportMaterialSettings) return 0;
	if (!HasValidMeshTarget())
	{
		ResolveInitialMeshTargets();
	}

	FString SafePrefix = FPaths::MakeValidFileName(FileNamePrefix.TrimStartAndEnd(), TEXT('_'));
	if (SafePrefix.IsEmpty())
	{
		SafePrefix = TEXT("PaintedTexture");
	}

	int32 ExportedCount = 0;
	int32 MeshIndex = 0;
	for (UMeshComponent* MeshTarget : GetMeshTargets())
	{
		if (!IsValid(MeshTarget)) continue;

		FRuntimeMeshPaintTargetMeshRuntimeData* RuntimeData = FindRuntimeDataForMesh(MeshTarget);
		if (!RuntimeData)
		{
			RuntimeData = &FindOrAddRuntimeDataForMesh(MeshTarget);
			if (!InitializeRuntimeDataForMesh(*RuntimeData)) continue;
		}

		const FString SafeMeshName = FPaths::MakeValidFileName(MakeMeshTargetString(MeshTarget), TEXT('_'));
		auto TryExportTextureType = [&](ERuntimeMeshPaintPatchTextureType TextureType, UTextureRenderTarget2D* RenderTarget)
		{
			if (!RenderTarget) return;

			const FString ExportFileName = FString::Printf(
				TEXT("%s_%02d_%s_%s"),
				*SafePrefix,
				MeshIndex,
				SafeMeshName.IsEmpty() ? TEXT("Mesh") : *SafeMeshName,
				GetPaintTextureTypeSuffix(TextureType));

			FString ExportedFilePath;
			if (ExportRenderTargetToImageFile(
				RenderTarget,
				TextureType,
				DirectoryPath,
				ExportFileName,
				ExportFormat,
				ExportedFilePath))
			{
				OutFilePaths.Add(ExportedFilePath);
				++ExportedCount;
			}
		};

		if (bExportColor)
		{
			TryExportTextureType(ERuntimeMeshPaintPatchTextureType::Color, RuntimeData->PaintedColorRenderTarget);
		}

		if (bExportMaterialSettings)
		{
			TryExportTextureType(
				ERuntimeMeshPaintPatchTextureType::MaterialSettings,
				RuntimeData->PaintedMaterialSettingsRenderTarget);
		}

		++MeshIndex;
	}

	UpdatePrimaryRenderTargetAliases();
	return ExportedCount;
}

UMeshComponent* URuntimeMeshPaintTargetComponent::ResolveInitialTargetMesh() const
{
	if (UMeshComponent* PrimaryMeshTarget = GetPrimaryMeshTarget()) return PrimaryMeshTarget;

	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
		return OwnerCharacter->GetMesh();

	TArray<UMeshComponent*> MeshComponents;
	if (const AActor* OwnerActor = GetOwner()) OwnerActor->GetComponents<UMeshComponent>(MeshComponents);
	return MeshComponents.Num() == 1 ? MeshComponents[0] : nullptr;
}

void URuntimeMeshPaintTargetComponent::CollectAvailableMeshTargets(TArray<UMeshComponent*>& OutMeshTargets) const
{
	OutMeshTargets.Reset();
	TSet<UMeshComponent*> UniqueMeshTargets;

	auto AddMeshTarget = [&OutMeshTargets, &UniqueMeshTargets](UMeshComponent* MeshTarget)
	{
		if (!IsValid(MeshTarget) || UniqueMeshTargets.Contains(MeshTarget)) return;

		UniqueMeshTargets.Add(MeshTarget);
		OutMeshTargets.Add(MeshTarget);
	};

	if (AActor* OwnerActor = GetOwner())
	{
		TArray<UMeshComponent*> OwnerMeshComponents;
		OwnerActor->GetComponents<UMeshComponent>(OwnerMeshComponents);
		for (UMeshComponent* OwnerMeshComponent : OwnerMeshComponents)
		{
			AddMeshTarget(OwnerMeshComponent);
		}
	}

	for (UObject* SearchOuter = GetOuter(); SearchOuter; SearchOuter = SearchOuter->GetOuter())
	{
		ForEachObjectWithOuter(SearchOuter, [&AddMeshTarget](UObject* Object)
		{
			AddMeshTarget(Cast<UMeshComponent>(Object));
		}, true);
	}
}

UMeshComponent* URuntimeMeshPaintTargetComponent::ResolveMeshTargetString(const FString& MeshTargetString) const
{
	const FString DesiredMeshTarget = MeshTargetString.TrimStartAndEnd();
	if (DesiredMeshTarget.IsEmpty()) return nullptr;

	TArray<UMeshComponent*> AvailableMeshTargets;
	CollectAvailableMeshTargets(AvailableMeshTargets);
	for (UMeshComponent* AvailableMeshTarget : AvailableMeshTargets)
	{
		if (!IsValid(AvailableMeshTarget)) continue;

		FString ComponentName = AvailableMeshTarget->GetName();
		FString NormalizedComponentName = ComponentName;
		NormalizedComponentName.RemoveFromEnd(TEXT("_GEN_VARIABLE"));

		if (ComponentName.Equals(DesiredMeshTarget, ESearchCase::IgnoreCase) ||
			NormalizedComponentName.Equals(DesiredMeshTarget, ESearchCase::IgnoreCase) ||
			AvailableMeshTarget->GetFName().ToString().Equals(DesiredMeshTarget, ESearchCase::IgnoreCase))
		{
			return AvailableMeshTarget;
		}
	}

	return nullptr;
}

FString URuntimeMeshPaintTargetComponent::MakeMeshTargetString(UMeshComponent* MeshComponent) const
{
	if (!IsValid(MeshComponent)) return FString();

	FString MeshTargetString = MeshComponent->GetName();
	MeshTargetString.RemoveFromEnd(TEXT("_GEN_VARIABLE"));
	return MeshTargetString;
}

void URuntimeMeshPaintTargetComponent::ResolveInitialMeshTargets()
{
	if (MeshTargets.Num() == 0 && IsValid(TargetMesh.Get()))
	{
		MeshTargets.Add(MakeMeshTargetString(TargetMesh.Get()));
	}

	if (HasValidMeshTarget())
	{
		TargetMesh = GetPrimaryMeshTarget();
		return;
	}

	if (UMeshComponent* InitialTargetMesh = ResolveInitialTargetMesh())
	{
		MeshTargets.Add(MakeMeshTargetString(InitialTargetMesh));
		TargetMesh = InitialTargetMesh;
	}
}

bool URuntimeMeshPaintTargetComponent::EnsureTargetMeshForBeginPlay()
{
	ResolveInitialMeshTargets();
	return HasValidMeshTarget();
}

bool URuntimeMeshPaintTargetComponent::HasValidMeshTarget() const
{
	for (const FString& MeshTarget : MeshTargets)
	{
		if (IsValid(ResolveMeshTargetString(MeshTarget))) return true;
	}

	return MeshTargets.Num() == 0 && IsValid(TargetMesh.Get());
}

UMeshComponent* URuntimeMeshPaintTargetComponent::GetPrimaryMeshTarget() const
{
	for (const FString& MeshTarget : MeshTargets)
	{
		if (UMeshComponent* ResolvedMeshTarget = ResolveMeshTargetString(MeshTarget))
		{
			return ResolvedMeshTarget;
		}
	}

	return MeshTargets.Num() == 0 && IsValid(TargetMesh.Get()) ? TargetMesh.Get() : nullptr;
}

void URuntimeMeshPaintTargetComponent::ResetRuntimePaintResources()
{
	MeshRuntimeData.Reset();
	PaintedColorRenderTarget = nullptr;
	PaintedMaterialSettingsRenderTarget = nullptr;
	BrushPreviewMaskRenderTarget = nullptr;
	PendingPatchCaptureRequests.Reset();
	PatchHistory.Reset();
	PatchHistoryBytes = 0;
	NextPatchHistorySequenceId = 0;
}

FRuntimeMeshPaintTargetMeshRuntimeData* URuntimeMeshPaintTargetComponent::FindRuntimeDataForMesh(UMeshComponent* MeshTarget)
{
	if (!IsValid(MeshTarget)) return nullptr;

	for (FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData : MeshRuntimeData)
	{
		if (RuntimeData.MeshTarget == MeshTarget) return &RuntimeData;
	}

	return nullptr;
}

const FRuntimeMeshPaintTargetMeshRuntimeData* URuntimeMeshPaintTargetComponent::FindRuntimeDataForMesh(UMeshComponent* MeshTarget) const
{
	if (!IsValid(MeshTarget)) return nullptr;

	for (const FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData : MeshRuntimeData)
	{
		if (RuntimeData.MeshTarget == MeshTarget) return &RuntimeData;
	}

	return nullptr;
}

UMeshComponent* URuntimeMeshPaintTargetComponent::ResolveMeshTargetForHit(const FHitResult& HitResult) const
{
	if (!HitResult.bBlockingHit) return nullptr;

	if (UMeshComponent* HitMesh = Cast<UMeshComponent>(HitResult.GetComponent()))
	{
		if (FindRuntimeDataForMesh(HitMesh)) return HitMesh;
		if (HasMeshTarget(HitMesh)) return HitMesh;
	}

	UPrimitiveComponent* HitComponent = HitResult.GetComponent();
	AActor* HitActor = HitResult.GetActor();
	AActor* OwnerActor = GetOwner();
	if (!HitComponent && !HitActor && !OwnerActor) return nullptr;

	auto IsCompatibleHitTarget = [HitComponent, HitActor, OwnerActor](UMeshComponent* MeshTarget)
	{
		if (!IsValid(MeshTarget)) return false;

		AActor* MeshOwner = MeshTarget->GetOwner();
		if (HitActor && MeshOwner == HitActor) return true;
		if (HitComponent && MeshOwner == HitComponent->GetOwner()) return true;
		return OwnerActor && MeshOwner == OwnerActor &&
			(HitActor == OwnerActor || (HitComponent && HitComponent->GetOwner() == OwnerActor));
	};

	UMeshComponent* BestMeshTarget = nullptr;
	double BestDistanceSq = TNumericLimits<double>::Max();
	for (const FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData : MeshRuntimeData)
	{
		UMeshComponent* MeshTarget = RuntimeData.MeshTarget.Get();
		if (!IsCompatibleHitTarget(MeshTarget)) continue;

		const double DistanceSq = FVector::DistSquared(MeshTarget->Bounds.Origin, HitResult.ImpactPoint);
		if (!BestMeshTarget || DistanceSq < BestDistanceSq)
		{
			BestMeshTarget = MeshTarget;
			BestDistanceSq = DistanceSq;
		}
	}

	if (BestMeshTarget) return BestMeshTarget;

	for (UMeshComponent* MeshTarget : GetMeshTargets())
	{
		if (!IsCompatibleHitTarget(MeshTarget)) continue;

		const double DistanceSq = FVector::DistSquared(MeshTarget->Bounds.Origin, HitResult.ImpactPoint);
		if (!BestMeshTarget || DistanceSq < BestDistanceSq)
		{
			BestMeshTarget = MeshTarget;
			BestDistanceSq = DistanceSq;
		}
	}

	return BestMeshTarget;
}

FRuntimeMeshPaintTargetMeshRuntimeData* URuntimeMeshPaintTargetComponent::FindRuntimeDataForHit(const FHitResult& HitResult)
{
	return FindRuntimeDataForMesh(ResolveMeshTargetForHit(HitResult));
}

const FRuntimeMeshPaintTargetMeshRuntimeData* URuntimeMeshPaintTargetComponent::FindRuntimeDataForHit(const FHitResult& HitResult) const
{
	return FindRuntimeDataForMesh(ResolveMeshTargetForHit(HitResult));
}

FRuntimeMeshPaintTargetMeshRuntimeData& URuntimeMeshPaintTargetComponent::FindOrAddRuntimeDataForMesh(UMeshComponent* MeshTarget)
{
	if (FRuntimeMeshPaintTargetMeshRuntimeData* RuntimeData = FindRuntimeDataForMesh(MeshTarget))
	{
		return *RuntimeData;
	}

	const int32 NewIndex = MeshRuntimeData.AddDefaulted();
	MeshRuntimeData[NewIndex].MeshTarget = MeshTarget;
	return MeshRuntimeData[NewIndex];
}

bool URuntimeMeshPaintTargetComponent::InitializeRuntimeDataForMesh(FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData)
{
	if (!IsValid(RuntimeData.MeshTarget.Get())) return false;
	if (RuntimeRenderTargetWidth <= 0 || RuntimeRenderTargetHeight <= 0) return false;

	if (!RuntimeData.PaintedColorRenderTarget)
	{
		RuntimeData.PaintedColorRenderTarget =
			CreateRuntimePaintRenderTarget(
				RuntimeRenderTargetWidth,
				RuntimeRenderTargetHeight,
				MakeNeutralTransparentPaintClearColor());
	}

	if (bCreatePaintedMaterialSettingsRenderTarget)
	{
		if (!RuntimeData.PaintedMaterialSettingsRenderTarget)
		{
			RuntimeData.PaintedMaterialSettingsRenderTarget =
				CreateRuntimePaintRenderTarget(
					RuntimeRenderTargetWidth,
					RuntimeRenderTargetHeight,
					MakeMaterialSettingsClearColor(InitialMaterialSettingsColor));
		}
	}
	else
	{
		RuntimeData.PaintedMaterialSettingsRenderTarget = nullptr;
	}

	const int32 PreviewTargetWidth = RuntimeData.PaintedColorRenderTarget
		? RuntimeData.PaintedColorRenderTarget->SizeX
		: RuntimeRenderTargetWidth;
	const int32 PreviewTargetHeight = RuntimeData.PaintedColorRenderTarget
		? RuntimeData.PaintedColorRenderTarget->SizeY
		: RuntimeRenderTargetHeight;
	if (!RuntimeData.BrushPreviewMaskRenderTarget ||
		RuntimeData.BrushPreviewMaskRenderTarget->SizeX != PreviewTargetWidth ||
		RuntimeData.BrushPreviewMaskRenderTarget->SizeY != PreviewTargetHeight)
	{
		RuntimeData.BrushPreviewMaskRenderTarget =
			CreateRuntimeBrushPreviewMaskRenderTarget(PreviewTargetWidth, PreviewTargetHeight);
	}

	FRuntimeMeshPaintGPUBrushRenderer::PrecacheProjectedMeshResource(
		RuntimeData.MeshTarget.Get(),
		UVChannel,
		UVIslandConnectionTolerance);

	return RuntimeData.PaintedColorRenderTarget != nullptr;
}

void URuntimeMeshPaintTargetComponent::UpdatePrimaryRenderTargetAliases()
{
	PaintedColorRenderTarget = nullptr;
	PaintedMaterialSettingsRenderTarget = nullptr;
	BrushPreviewMaskRenderTarget = nullptr;

	if (const FRuntimeMeshPaintTargetMeshRuntimeData* PrimaryRuntimeData = FindRuntimeDataForMesh(GetPrimaryMeshTarget()))
	{
		if (PrimaryRuntimeData->PaintedColorRenderTarget)
		{
			PaintedColorRenderTarget = PrimaryRuntimeData->PaintedColorRenderTarget;
			PaintedMaterialSettingsRenderTarget = PrimaryRuntimeData->PaintedMaterialSettingsRenderTarget;
			BrushPreviewMaskRenderTarget = PrimaryRuntimeData->BrushPreviewMaskRenderTarget;
			return;
		}
	}

	for (const FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData : MeshRuntimeData)
	{
		if (!RuntimeData.PaintedColorRenderTarget) continue;

		PaintedColorRenderTarget = RuntimeData.PaintedColorRenderTarget;
		PaintedMaterialSettingsRenderTarget = RuntimeData.PaintedMaterialSettingsRenderTarget;
		BrushPreviewMaskRenderTarget = RuntimeData.BrushPreviewMaskRenderTarget;
		return;
	}
}

int32 URuntimeMeshPaintTargetComponent::FindMeshTargetIndex(UMeshComponent* MeshTarget) const
{
	if (!IsValid(MeshTarget)) return INDEX_NONE;

	const TArray<UMeshComponent*> ResolvedMeshTargets = GetMeshTargets();
	for (int32 MeshTargetIndex = 0; MeshTargetIndex < ResolvedMeshTargets.Num(); ++MeshTargetIndex)
	{
		if (ResolvedMeshTargets[MeshTargetIndex] == MeshTarget) return MeshTargetIndex;
	}

	return INDEX_NONE;
}

UMeshComponent* URuntimeMeshPaintTargetComponent::GetMeshTargetByIndex(int32 MeshTargetIndex) const
{
	if (MeshTargetIndex < 0) return nullptr;

	const TArray<UMeshComponent*> ResolvedMeshTargets = GetMeshTargets();
	return ResolvedMeshTargets.IsValidIndex(MeshTargetIndex) ? ResolvedMeshTargets[MeshTargetIndex] : nullptr;
}

bool URuntimeMeshPaintTargetComponent::BuildReplicatedPaintCommand(
	const FRuntimeMeshPaintSampleResult& PaintHit,
	const FMeshPaintBrushMaterialSettings& BrushSettings,
	uint16 StrokeId,
	uint32 ClientPredictionKey,
	AActor* SourceActor,
	FRuntimeMeshPaintNetCommand& OutCommand) const
{
	OutCommand = FRuntimeMeshPaintNetCommand();
	if (!PaintHit.bSuccess) return false;
	if (UVChannel < 0 || UVChannel > MAX_uint8) return false;

	UMeshComponent* PaintMeshTarget = ResolveMeshTargetForHit(PaintHit.HitResult);
	const int32 MeshTargetIndex = FindMeshTargetIndex(PaintMeshTarget);
	if (MeshTargetIndex == INDEX_NONE || MeshTargetIndex > MAX_uint8) return false;
	if (Cast<USkeletalMeshComponent>(PaintMeshTarget) && !PaintHit.ProjectionData.bHasScreenProjection) return false;

	OutCommand.TargetComponent = const_cast<URuntimeMeshPaintTargetComponent*>(this);
	OutCommand.SourceActor = SourceActor;
	OutCommand.ClientPredictionKey = ClientPredictionKey;
	OutCommand.StrokeId = StrokeId;
	OutCommand.MeshTargetIndex = static_cast<uint8>(MeshTargetIndex);
	OutCommand.LOD = 0;
	OutCommand.UVChannel = static_cast<uint8>(UVChannel);
	OutCommand.BrushRayStart = PaintHit.HitResult.TraceStart;
	OutCommand.BrushRayEnd = PaintHit.HitResult.TraceEnd;
	OutCommand.BrushWorldCenter = PaintHit.HitResult.ImpactPoint;
	OutCommand.BrushWorldNormal = PaintHit.HitResult.ImpactNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
	OutCommand.SetBrushSettings(BrushSettings, bCreatePaintedMaterialSettingsRenderTarget);
	FVector2D PaintUV = FVector2D::ZeroVector;
	int32 ResolvedFaceIndex = INDEX_NONE;
	int32 ResolvedTriangleArrayIndex = INDEX_NONE;
	if (RuntimeMeshPaint::FindPaintHitUV(
		PaintHit.HitResult,
		UVChannel,
		MaxSkeletalMeshUVFallbackDistance,
		PaintUV,
		&ResolvedFaceIndex,
		&ResolvedTriangleArrayIndex))
	{
		OutCommand.BrushUVX = QuantizeRuntimeMeshPaintUV(PaintUV.X);
		OutCommand.BrushUVY = QuantizeRuntimeMeshPaintUV(PaintUV.Y);
		OutCommand.CommandFlags |= static_cast<uint8>(ERuntimeMeshPaintNetCommandFlags::PaintUV);
	}
	if (PaintHit.ProjectionData.bHasScreenProjection)
	{
		OutCommand.CommandFlags |= static_cast<uint8>(ERuntimeMeshPaintNetCommandFlags::ScreenProjection);
		OutCommand.ProjectionData = PaintHit.ProjectionData;
	}
	return true;
}

void URuntimeMeshPaintTargetComponent::RememberPredictedPaintCommand(const FRuntimeMeshPaintNetCommand& Command)
{
	if (Command.ClientPredictionKey != 0)
	{
		PredictedPaintCommandKeys.Add(Command.ClientPredictionKey);
	}
}

bool URuntimeMeshPaintTargetComponent::ValidateReplicatedPaintCommand(
	const FRuntimeMeshPaintNetCommand& Command,
	const AController* InstigatorController) const
{
	if (!bReplicateRuntimePaint) return false;
	if (Command.TargetComponent.Get() != this) return false;
	if (Command.UVChannel != UVChannel) return false;
	if (Command.LOD != 0) return false;

	UMeshComponent* MeshTarget = GetMeshTargetByIndex(Command.MeshTargetIndex);
	if (!IsValid(MeshTarget) || !HasMeshTarget(MeshTarget)) return false;

	const bool bRequiresScreenProjection = Cast<USkeletalMeshComponent>(MeshTarget) != nullptr;
	const bool bHasScreenProjectionFlag =
		(Command.CommandFlags & static_cast<uint8>(ERuntimeMeshPaintNetCommandFlags::ScreenProjection)) != 0;
	if (bRequiresScreenProjection && !bHasScreenProjectionFlag) return false;
	if (bHasScreenProjectionFlag && !IsValidRuntimeMeshPaintScreenProjectionData(Command)) return false;

	const FMeshPaintBrushMaterialSettings BrushSettings = Command.ToBrushSettings();
	if (BrushSettings.BrushSize <= 0.0f || BrushSettings.BrushSize > MaxReplicatedBrushSize) return false;

	if (Command.BrushWorldNormal.IsNearlyZero()) return false;
	if (Command.BrushRayStart.Equals(Command.BrushRayEnd)) return false;

	if (MaxReplicatedPaintDistance > 0.0f && InstigatorController)
	{
		const APawn* InstigatorPawn = InstigatorController->GetPawn();
		if (InstigatorPawn)
		{
			const double MaxDistanceSq = FMath::Square(static_cast<double>(MaxReplicatedPaintDistance));
			if (FVector::DistSquared(InstigatorPawn->GetActorLocation(), Command.BrushWorldCenter) > MaxDistanceSq)
			{
				return false;
			}
		}
	}

	return true;
}

bool URuntimeMeshPaintTargetComponent::ApplyPaintCommandLocal(
	const FRuntimeMeshPaintNetCommand& Command,
	FRuntimeMeshPaintSampleResult* OutPaintResult)
{
	if (OutPaintResult) *OutPaintResult = FRuntimeMeshPaintSampleResult();
	if (GetNetMode() == NM_DedicatedServer) return true;

	UMeshComponent* MeshTarget = GetMeshTargetByIndex(Command.MeshTargetIndex);
	if (!IsValid(MeshTarget)) return false;
	if (Cast<USkeletalMeshComponent>(MeshTarget) && !Command.ProjectionData.bHasScreenProjection) return false;

	FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData = FindOrAddRuntimeDataForMesh(MeshTarget);
	if (!InitializeRuntimeDataForMesh(RuntimeData) || !RuntimeData.PaintedColorRenderTarget) return false;

	FMeshPaintBrushMaterialSettings BrushSettings = Command.ToBrushSettings();
	BrushSettings.Clamp();

	const float BrushWorldRadius =
		FMath::Max(BrushSettings.BrushSize, KINDA_SMALL_NUMBER) * RuntimeMeshPaintTargetBrushSizeToWorldRadiusScale;

	SCOPE_CYCLE_COUNTER(STAT_MeshPaintingCore_DrawBrushToRenderTarget);
	if (!FRuntimeMeshPaintGPUBrushRenderer::DrawProjectedBrush(
		RuntimeData.MeshTarget.Get(),
		RuntimeData.PaintedColorRenderTarget,
		RuntimeData.PaintedMaterialSettingsRenderTarget,
		Command.UVChannel,
		UVIslandConnectionTolerance,
		Command.BrushRayStart,
		Command.BrushRayEnd,
		Command.BrushWorldCenter,
		Command.BrushWorldNormal,
		BrushWorldRadius,
		Command.ProjectionData,
		BrushSettings))
	{
		return false;
	}

	FRuntimeMeshPaintSampleResult PaintResult;
	PaintResult.bSuccess = true;
	PaintResult.Color = BrushSettings.Color;
	PaintResult.HitResult.bBlockingHit = true;
	PaintResult.HitResult.Component = MeshTarget;
	PaintResult.HitResult.Location = Command.BrushWorldCenter;
	PaintResult.HitResult.ImpactPoint = Command.BrushWorldCenter;
	PaintResult.HitResult.Normal = Command.BrushWorldNormal;
	PaintResult.HitResult.ImpactNormal = Command.BrushWorldNormal;
	PaintResult.HitResult.TraceStart = Command.BrushRayStart;
	PaintResult.HitResult.TraceEnd = Command.BrushRayEnd;
	if ((Command.CommandFlags & static_cast<uint8>(ERuntimeMeshPaintNetCommandFlags::PaintUV)) != 0)
	{
		PaintResult.UV = FVector2D(
			DequantizeRuntimeMeshPaintUV(Command.BrushUVX),
			DequantizeRuntimeMeshPaintUV(Command.BrushUVY));
	}

	if (OutPaintResult) *OutPaintResult = PaintResult;

	INC_DWORD_STAT(STAT_MeshPaintingCore_RenderTargetDraws);
	INC_DWORD_STAT(STAT_MeshPaintingCore_SuccessfulPaintCalls);
	QueuePatchCaptureForPaint(PaintResult, BrushSettings, RuntimeData);
	OnPaintApplied.Broadcast(PaintResult);
	return true;
}

bool URuntimeMeshPaintTargetComponent::AcceptReplicatedPaintCommand(
	const FRuntimeMeshPaintNetCommand& Command,
	AController* InstigatorController,
	bool bApplyLocally)
{
	if (!ValidateReplicatedPaintCommand(Command, InstigatorController)) return false;

	const uint64 ClientDedupKey = MakeClientPredictionDedupKey(Command);
	if (ClientDedupKey != 0 && AcceptedClientPaintPredictionKeys.Contains(ClientDedupKey))
	{
		return true;
	}

	FRuntimeMeshPaintNetCommand AcceptedCommand = Command;
	AcceptedCommand.TargetComponent = this;
	AcceptedCommand.CommandId = ++NextReplicatedPaintCommandId;
	if (NextReplicatedPaintCommandId == 0)
	{
		AcceptedCommand.CommandId = ++NextReplicatedPaintCommandId;
	}

	if (bApplyLocally)
	{
		ApplyPaintCommandLocal(AcceptedCommand, nullptr);
	}

	AppendAuthoritativePaintCommand(AcceptedCommand);
	if (ClientDedupKey != 0) AcceptedClientPaintPredictionKeys.Add(ClientDedupKey);
	return true;
}

bool URuntimeMeshPaintTargetComponent::IsLocalPaintSourceActor(const AActor* SourceActor) const
{
	if (!SourceActor) return false;

	const UWorld* World = GetWorld();
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		const APlayerController* PlayerController = Iterator->Get();
		if (!PlayerController || !PlayerController->IsLocalPlayerController()) continue;

		if (SourceActor == PlayerController || SourceActor == PlayerController->GetPawn()) return true;
	}

	return false;
}

bool URuntimeMeshPaintTargetComponent::ShouldSkipPredictedPaintCommand(
	const FRuntimeMeshPaintNetCommand& Command) const
{
	return Command.ClientPredictionKey != 0 &&
		PredictedPaintCommandKeys.Contains(Command.ClientPredictionKey) &&
		IsLocalPaintSourceActor(Command.SourceActor.Get());
}

uint64 URuntimeMeshPaintTargetComponent::MakeClientPredictionDedupKey(
	const FRuntimeMeshPaintNetCommand& Command) const
{
	const AActor* SourceActor = Command.SourceActor.Get();
	if (!SourceActor || Command.ClientPredictionKey == 0) return 0;

	return (static_cast<uint64>(SourceActor->GetUniqueID()) << 32) |
		Command.ClientPredictionKey;
}

void URuntimeMeshPaintTargetComponent::AppendAuthoritativePaintCommand(const FRuntimeMeshPaintNetCommand& Command)
{
	if (!bReplicateRuntimePaint) return;

	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->FlushNetDormancy();
		OwnerActor->ForceNetUpdate();
	}

	ReplicatedPaintHistory.AddCommand(Command, MaxReplicatedPaintCommands);
}

void URuntimeMeshPaintTargetComponent::HandleReplicatedPaintCommand(const FRuntimeMeshPaintNetCommand& Command)
{
	if (!bReplicateRuntimePaint || Command.CommandId == 0) return;
	if (AppliedReplicatedPaintCommandIds.Contains(Command.CommandId)) return;
	if (PendingReplicatedPaintCommands.ContainsByPredicate([CommandId = Command.CommandId](const FRuntimeMeshPaintNetCommand& PendingCommand)
	{
		return PendingCommand.CommandId == CommandId;
	}))
	{
		return;
	}

	if (ShouldSkipPredictedPaintCommand(Command))
	{
		AppliedReplicatedPaintCommandIds.Add(Command.CommandId);
		PredictedPaintCommandKeys.Remove(Command.ClientPredictionKey);
		return;
	}

	int32 InsertIndex = PendingReplicatedPaintCommands.Num();
	while (InsertIndex > PendingReplicatedPaintCommandReadIndex &&
		PendingReplicatedPaintCommands[InsertIndex - 1].CommandId > Command.CommandId)
	{
		--InsertIndex;
	}

	PendingReplicatedPaintCommands.Insert(Command, InsertIndex);
	SetComponentTickEnabled(true);
}

void URuntimeMeshPaintTargetComponent::ProcessPendingReplicatedPaintCommands()
{
	const int32 CommandBudget = FMath::Max(1, ReplicatedPaintReplayCommandsPerTick);
	int32 CommandsApplied = 0;

	while (PendingReplicatedPaintCommands.IsValidIndex(PendingReplicatedPaintCommandReadIndex) &&
		CommandsApplied < CommandBudget)
	{
		const FRuntimeMeshPaintNetCommand& Command =
			PendingReplicatedPaintCommands[PendingReplicatedPaintCommandReadIndex];

		if (Command.CommandId != 0 && AppliedReplicatedPaintCommandIds.Contains(Command.CommandId))
		{
			++PendingReplicatedPaintCommandReadIndex;
			continue;
		}

		if (!ApplyPaintCommandLocal(Command, nullptr))
		{
			break;
		}

		if (Command.CommandId != 0) AppliedReplicatedPaintCommandIds.Add(Command.CommandId);
		++PendingReplicatedPaintCommandReadIndex;
		++CommandsApplied;
	}

	if (PendingReplicatedPaintCommandReadIndex >= PendingReplicatedPaintCommands.Num())
	{
		PendingReplicatedPaintCommands.Reset();
		PendingReplicatedPaintCommandReadIndex = 0;
		SetComponentTickEnabled(false);
	}
}

void URuntimeMeshPaintTargetComponent::QueuePatchCaptureForPaint(
	const FRuntimeMeshPaintSampleResult& PaintHit,
	const FMeshPaintBrushMaterialSettings& BrushSettings,
	const FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData)
{
	if (!bRecordPaintPatchHistory || GetNetMode() == NM_DedicatedServer) return;
	if (!PaintHit.bSuccess || !RuntimeData.PaintedColorRenderTarget || !IsValid(RuntimeData.MeshTarget.Get())) return;

	FIntRect DirtyRect;
	if (!BuildPatchDirtyRect(PaintHit, BrushSettings, RuntimeData.PaintedColorRenderTarget, DirtyRect)) return;

	const int32 MeshTargetIndex = FindMeshTargetIndex(RuntimeData.MeshTarget.Get());
	if (MeshTargetIndex == INDEX_NONE) return;

	auto AddOrMergeRequest = [this, &RuntimeData, &DirtyRect, MeshTargetIndex](
		UTextureRenderTarget2D* RenderTarget,
		ERuntimeMeshPaintPatchTextureType TextureType)
	{
		if (!RenderTarget) return;

		for (FPendingPatchCaptureRequest& Request : PendingPatchCaptureRequests)
		{
			if (Request.RenderTarget.Get() == RenderTarget &&
				Request.MeshTarget.Get() == RuntimeData.MeshTarget.Get() &&
				Request.TextureType == TextureType)
			{
				Request.DirtyRect.Min.X = FMath::Min(Request.DirtyRect.Min.X, DirtyRect.Min.X);
				Request.DirtyRect.Min.Y = FMath::Min(Request.DirtyRect.Min.Y, DirtyRect.Min.Y);
				Request.DirtyRect.Max.X = FMath::Max(Request.DirtyRect.Max.X, DirtyRect.Max.X);
				Request.DirtyRect.Max.Y = FMath::Max(Request.DirtyRect.Max.Y, DirtyRect.Max.Y);
				return;
			}
		}

		FPendingPatchCaptureRequest& Request = PendingPatchCaptureRequests.AddDefaulted_GetRef();
		Request.RenderTarget = RenderTarget;
		Request.MeshTarget = RuntimeData.MeshTarget.Get();
		Request.TextureType = TextureType;
		Request.DirtyRect = DirtyRect;
		Request.MeshTargetIndex = MeshTargetIndex;
	};

	AddOrMergeRequest(RuntimeData.PaintedColorRenderTarget, ERuntimeMeshPaintPatchTextureType::Color);
	if (RuntimeData.PaintedMaterialSettingsRenderTarget)
	{
		AddOrMergeRequest(RuntimeData.PaintedMaterialSettingsRenderTarget, ERuntimeMeshPaintPatchTextureType::MaterialSettings);
	}

	// Patch pixels are read back only when export/compact/flush is requested.
	// Doing ReadPixels while painting stalls the GPU and makes brush strokes hitch.
}

void URuntimeMeshPaintTargetComponent::ProcessPendingPatchCaptures(int32 CaptureBudget)
{
	if (PendingPatchCaptureRequests.Num() == 0) return;

	const int32 SafeBudget = FMath::Max(1, CaptureBudget);
	int32 CapturesProcessed = 0;
	while (PendingPatchCaptureRequests.Num() > 0 && CapturesProcessed < SafeBudget)
	{
		FPendingPatchCaptureRequest Request = PendingPatchCaptureRequests[0];
		PendingPatchCaptureRequests.RemoveAt(0, 1, EAllowShrinking::No);

		UTextureRenderTarget2D* RenderTarget = Request.RenderTarget.Get();
		UMeshComponent* MeshTarget = Request.MeshTarget.Get();
		if (RenderTarget && MeshTarget)
		{
			CapturePatch(RenderTarget, Request.TextureType, Request.DirtyRect, MeshTarget, Request.MeshTargetIndex);
		}

		++CapturesProcessed;
	}
}

bool URuntimeMeshPaintTargetComponent::BuildPatchDirtyRect(
	const FRuntimeMeshPaintSampleResult& PaintHit,
	const FMeshPaintBrushMaterialSettings& BrushSettings,
	const UTextureRenderTarget2D* RenderTarget,
	FIntRect& OutDirtyRect) const
{
	OutDirtyRect = FIntRect();
	if (!RenderTarget || RenderTarget->SizeX <= 0 || RenderTarget->SizeY <= 0) return false;

	FVector2D HitUV = PaintHit.UV;
	bool bHasHitUV =
		HitUV.X >= 0.0 &&
		HitUV.X <= 1.0 &&
		HitUV.Y >= 0.0 &&
		HitUV.Y <= 1.0 &&
		!HitUV.IsNearlyZero();
	if (!bHasHitUV)
	{
		int32 ResolvedFaceIndex = INDEX_NONE;
		int32 ResolvedTriangleArrayIndex = INDEX_NONE;
		bHasHitUV = RuntimeMeshPaint::FindPaintHitUV(
			PaintHit.HitResult,
			UVChannel,
			MaxSkeletalMeshUVFallbackDistance,
			HitUV,
			&ResolvedFaceIndex,
			&ResolvedTriangleArrayIndex);
	}

	if (!bHasHitUV)
	{
		OutDirtyRect = FIntRect(0, 0, RenderTarget->SizeX, RenderTarget->SizeY);
		return true;
	}

	HitUV.X = FMath::Clamp(HitUV.X, 0.0, 1.0);
	HitUV.Y = FMath::Clamp(HitUV.Y, 0.0, 1.0);

	const float SafetyMultiplier = FMath::Max(0.1f, PatchHistoryDirtyRectBrushSizeMultiplier);
	const float MinUVRadius = 1.0f / FMath::Max(RenderTarget->SizeX, RenderTarget->SizeY);
	float UVRadius = MinUVRadius;

	float HitUVRadius = 0.0f;
	float HitWorldRadius = 0.0f;
	if (RuntimeMeshPaint::ResolvePaintBrushRadii(
		PaintHit.HitResult,
		UVChannel,
		MaxSkeletalMeshUVFallbackDistance,
		HitUV,
		BrushSettings.BrushSize,
		HitUVRadius,
		HitWorldRadius))
	{
		UVRadius = FMath::Max(HitUVRadius * SafetyMultiplier, MinUVRadius);
	}
	else
	{
		// Last resort for unusual meshes where triangle UV scale cannot be resolved.
		// This stays conservative for correctness, but no longer bloats the common path.
		UVRadius = FMath::Max(BrushSettings.BrushSize * SafetyMultiplier, MinUVRadius);
	}

	const int32 PaddingPixels = FMath::Max(0, PatchHistoryDirtyRectPaddingPixels);
	const int32 RadiusX = FMath::CeilToInt(UVRadius * RenderTarget->SizeX) + PaddingPixels;
	const int32 RadiusY = FMath::CeilToInt(UVRadius * RenderTarget->SizeY) + PaddingPixels;
	const int32 CenterX = FMath::Clamp(FMath::FloorToInt(HitUV.X * RenderTarget->SizeX), 0, RenderTarget->SizeX - 1);
	const int32 CenterY = FMath::Clamp(FMath::FloorToInt(HitUV.Y * RenderTarget->SizeY), 0, RenderTarget->SizeY - 1);

	OutDirtyRect.Min.X = FMath::Clamp(CenterX - RadiusX, 0, RenderTarget->SizeX);
	OutDirtyRect.Min.Y = FMath::Clamp(CenterY - RadiusY, 0, RenderTarget->SizeY);
	OutDirtyRect.Max.X = FMath::Clamp(CenterX + RadiusX + 1, 0, RenderTarget->SizeX);
	OutDirtyRect.Max.Y = FMath::Clamp(CenterY + RadiusY + 1, 0, RenderTarget->SizeY);
	return OutDirtyRect.Width() > 0 && OutDirtyRect.Height() > 0;
}

bool URuntimeMeshPaintTargetComponent::CapturePatch(
	UTextureRenderTarget2D* RenderTarget,
	ERuntimeMeshPaintPatchTextureType TextureType,
	const FIntRect& DirtyRect,
	UMeshComponent* MeshTarget,
	int32 MeshTargetIndex)
{
	if (!RenderTarget || !MeshTarget || DirtyRect.Width() <= 0 || DirtyRect.Height() <= 0) return false;
	if (DirtyRect.Min.X < 0 || DirtyRect.Min.Y < 0 || DirtyRect.Max.X > RenderTarget->SizeX || DirtyRect.Max.Y > RenderTarget->SizeY) return false;

	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	if (!RenderTargetResource) return false;

	TArray<FColor> Pixels;
	FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
	ReadFlags.SetLinearToGamma(false);
	if (!RenderTargetResource->ReadPixels(Pixels, ReadFlags, DirtyRect))
	{
		return false;
	}

	if (Pixels.Num() != DirtyRect.Width() * DirtyRect.Height()) return false;

	FRuntimeMeshPaintPatchHistoryEntry Entry;
	Entry.PaintTargetName = GetOwner()
		? FString::Printf(TEXT("%s.%s"), *GetOwner()->GetName(), *GetName())
		: GetName();
	Entry.MeshTargetName = MakeMeshTargetString(MeshTarget);
	Entry.MeshTargetIndex = MeshTargetIndex;
	Entry.TextureType = TextureType;
	Entry.X = DirtyRect.Min.X;
	Entry.Y = DirtyRect.Min.Y;
	Entry.Width = DirtyRect.Width();
	Entry.Height = DirtyRect.Height();
	Entry.RTWidth = RenderTarget->SizeX;
	Entry.RTHeight = RenderTarget->SizeY;
	Entry.RTFormat = RenderTarget->RenderTargetFormat;
	Entry.UVChannel = UVChannel;
	Entry.UncompressedByteCount = Pixels.Num() * static_cast<int32>(sizeof(FColor));
	if (!CompressPatchColorBytes(Pixels, bCompressPatchHistory, Entry.PixelBytes, Entry.bCompressed))
	{
		return false;
	}

	AddPatchHistoryEntry(MoveTemp(Entry));
	return true;
}

void URuntimeMeshPaintTargetComponent::AddPatchHistoryEntry(FRuntimeMeshPaintPatchHistoryEntry&& Entry)
{
	if (!Entry.IsValidPatch()) return;
	if (Entry.SequenceId <= 0)
	{
		Entry.SequenceId = ++NextPatchHistorySequenceId;
	}
	else
	{
		NextPatchHistorySequenceId = FMath::Max(NextPatchHistorySequenceId, Entry.SequenceId);
	}

	PatchHistoryBytes += GetPatchStoredBytes(Entry);
	PatchHistory.Add(MoveTemp(Entry));
	TrimPatchHistory();
}

void URuntimeMeshPaintTargetComponent::TrimPatchHistory()
{
	while (MaxPatchHistoryEntries > 0 && PatchHistory.Num() > MaxPatchHistoryEntries)
	{
		PatchHistoryBytes -= GetPatchStoredBytes(PatchHistory[0]);
		PatchHistory.RemoveAt(0, 1, EAllowShrinking::No);
	}

	while (MaxPatchHistoryBytes > 0 && PatchHistoryBytes > MaxPatchHistoryBytes && PatchHistory.Num() > 0)
	{
		PatchHistoryBytes -= GetPatchStoredBytes(PatchHistory[0]);
		PatchHistory.RemoveAt(0, 1, EAllowShrinking::No);
	}

	PatchHistoryBytes = FMath::Max(0, PatchHistoryBytes);
}

void URuntimeMeshPaintTargetComponent::RecalculatePatchHistoryBytes()
{
	PatchHistoryBytes = 0;
	NextPatchHistorySequenceId = 0;
	for (const FRuntimeMeshPaintPatchHistoryEntry& Entry : PatchHistory)
	{
		PatchHistoryBytes += GetPatchStoredBytes(Entry);
		NextPatchHistorySequenceId = FMath::Max(NextPatchHistorySequenceId, Entry.SequenceId);
	}
}

void URuntimeMeshPaintTargetComponent::FlushPendingPaintPatchCaptures()
{
	while (PendingPatchCaptureRequests.Num() > 0)
	{
		ProcessPendingPatchCaptures(PendingPatchCaptureRequests.Num());
	}
}

void URuntimeMeshPaintTargetComponent::ClearPaintPatchHistory()
{
	PendingPatchCaptureRequests.Reset();
	PatchHistory.Reset();
	PatchHistoryBytes = 0;
	NextPatchHistorySequenceId = 0;
}

bool URuntimeMeshPaintTargetComponent::ExportPaintPatchHistory(FRuntimeMeshPaintPatchHistory& OutHistory)
{
	FlushPendingPaintPatchCaptures();

	OutHistory = FRuntimeMeshPaintPatchHistory();
	OutHistory.Version = RuntimeMeshPaintPatchHistoryVersion;
	OutHistory.LastSequenceId = NextPatchHistorySequenceId;
	OutHistory.Entries = PatchHistory;
	return OutHistory.Entries.Num() > 0;
}

bool URuntimeMeshPaintTargetComponent::ImportPaintPatchHistory(
	const FRuntimeMeshPaintPatchHistory& History,
	bool bClearExistingPaint,
	bool bClearHistory)
{
	if (History.Version <= 0 || History.Entries.Num() == 0) return false;
	return ImportPaintPatchHistoryLocal(History, bClearExistingPaint, bClearHistory);
}

bool URuntimeMeshPaintTargetComponent::ImportPaintPatchHistoryLocal(
	const FRuntimeMeshPaintPatchHistory& History,
	bool bClearExistingPaint,
	bool bClearHistory)
{
	if (GetNetMode() == NM_DedicatedServer) return false;
	if (History.Version <= 0 || History.Entries.Num() == 0) return false;

	PendingPatchCaptureRequests.Reset();
	if (!HasValidMeshTarget())
	{
		ResolveInitialMeshTargets();
	}

	if (bClearExistingPaint)
	{
		InitializeRuntimePaintTarget();
		for (FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData : MeshRuntimeData)
		{
			if (RuntimeData.PaintedColorRenderTarget)
			{
				UKismetRenderingLibrary::ClearRenderTarget2D(
					this,
					RuntimeData.PaintedColorRenderTarget,
					MakeNeutralTransparentPaintClearColor());
			}

			if (RuntimeData.PaintedMaterialSettingsRenderTarget)
			{
				UKismetRenderingLibrary::ClearRenderTarget2D(
					this,
					RuntimeData.PaintedMaterialSettingsRenderTarget,
					MakeMaterialSettingsClearColor(InitialMaterialSettingsColor));
			}
		}
	}

	TArray<FRuntimeMeshPaintPatchHistoryEntry> AppliedEntries;
	AppliedEntries.Reserve(History.Entries.Num());
	int32 AppliedCount = 0;
	const bool bPreserveExistingPixelsForNeutralPatchPixels = !bClearExistingPaint;
	for (const FRuntimeMeshPaintPatchHistoryEntry& Entry : History.Entries)
	{
		if (!ApplyPatchHistoryEntry(Entry, bPreserveExistingPixelsForNeutralPatchPixels)) continue;
		AppliedEntries.Add(Entry);
		++AppliedCount;
	}

	if (AppliedCount <= 0) return false;

	if (bClearHistory)
	{
		PatchHistory = MoveTemp(AppliedEntries);
		RecalculatePatchHistoryBytes();
		TrimPatchHistory();
	}
	else
	{
		for (FRuntimeMeshPaintPatchHistoryEntry& Entry : AppliedEntries)
		{
			AddPatchHistoryEntry(MoveTemp(Entry));
		}
	}

	ApplyRuntimePaintTexturesToTargetMesh();
	UpdatePrimaryRenderTargetAliases();
	return true;
}

bool URuntimeMeshPaintTargetComponent::CompactPaintPatchHistory(FRuntimeMeshPaintPatchHistory& OutCompactedHistory)
{
	FlushPendingPaintPatchCaptures();

	OutCompactedHistory = FRuntimeMeshPaintPatchHistory();
	OutCompactedHistory.Version = RuntimeMeshPaintPatchHistoryVersion;
	OutCompactedHistory.LastSequenceId = NextPatchHistorySequenceId;
	if (PatchHistory.Num() == 0) return false;

	struct FCompactionGroup
	{
		FString PaintTargetName;
		FString MeshTargetName;
		int32 MeshTargetIndex = INDEX_NONE;
		ERuntimeMeshPaintPatchTextureType TextureType = ERuntimeMeshPaintPatchTextureType::Color;
		int32 RTWidth = 0;
		int32 RTHeight = 0;
		TEnumAsByte<ETextureRenderTargetFormat> RTFormat = RTF_RGBA16f;
		int32 UVChannel = 0;
		FIntRect Rect;
		int32 LastSequenceId = 0;
		TArray<const FRuntimeMeshPaintPatchHistoryEntry*> Entries;

		bool Matches(const FRuntimeMeshPaintPatchHistoryEntry& Entry) const
		{
			return PaintTargetName == Entry.PaintTargetName &&
				MeshTargetName == Entry.MeshTargetName &&
				MeshTargetIndex == Entry.MeshTargetIndex &&
				TextureType == Entry.TextureType &&
				RTWidth == Entry.RTWidth &&
				RTHeight == Entry.RTHeight &&
				RTFormat == Entry.RTFormat &&
				UVChannel == Entry.UVChannel;
		}

		void Add(const FRuntimeMeshPaintPatchHistoryEntry& Entry)
		{
			if (Entries.Num() == 0)
			{
				PaintTargetName = Entry.PaintTargetName;
				MeshTargetName = Entry.MeshTargetName;
				MeshTargetIndex = Entry.MeshTargetIndex;
				TextureType = Entry.TextureType;
				RTWidth = Entry.RTWidth;
				RTHeight = Entry.RTHeight;
				RTFormat = Entry.RTFormat;
				UVChannel = Entry.UVChannel;
				Rect = FIntRect(Entry.X, Entry.Y, Entry.X + Entry.Width, Entry.Y + Entry.Height);
			}
			else
			{
				Rect.Min.X = FMath::Min(Rect.Min.X, Entry.X);
				Rect.Min.Y = FMath::Min(Rect.Min.Y, Entry.Y);
				Rect.Max.X = FMath::Max(Rect.Max.X, Entry.X + Entry.Width);
				Rect.Max.Y = FMath::Max(Rect.Max.Y, Entry.Y + Entry.Height);
			}

			LastSequenceId = FMath::Max(LastSequenceId, Entry.SequenceId);
			Entries.Add(&Entry);
		}
	};

	TArray<FCompactionGroup> Groups;
	for (const FRuntimeMeshPaintPatchHistoryEntry& Entry : PatchHistory)
	{
		if (!Entry.IsValidPatch()) continue;

		FCompactionGroup* MatchingGroup = nullptr;
		for (FCompactionGroup& Group : Groups)
		{
			if (Group.Matches(Entry))
			{
				MatchingGroup = &Group;
				break;
			}
		}

		if (!MatchingGroup)
		{
			MatchingGroup = &Groups.AddDefaulted_GetRef();
		}
		MatchingGroup->Add(Entry);
	}

	TArray<FRuntimeMeshPaintPatchHistoryEntry> CompactedEntries;
	CompactedEntries.Reserve(Groups.Num());
	for (const FCompactionGroup& Group : Groups)
	{
		const int32 GroupWidth = Group.Rect.Width();
		const int32 GroupHeight = Group.Rect.Height();
		if (GroupWidth <= 0 || GroupHeight <= 0) continue;

		const FColor ClearColor =
			Group.TextureType == ERuntimeMeshPaintPatchTextureType::MaterialSettings
				? MakeMaterialSettingsClearColor(InitialMaterialSettingsColor).ToFColor(false)
				: MakeNeutralTransparentPaintClearColor().ToFColor(false);

		TArray<FColor> GroupPixels;
		GroupPixels.Init(ClearColor, GroupWidth * GroupHeight);

		for (const FRuntimeMeshPaintPatchHistoryEntry* EntryPtr : Group.Entries)
		{
			if (!EntryPtr) continue;

			TArray<FColor> EntryPixels;
			if (!ResolvePatchPixels(*EntryPtr, EntryPixels) ||
				EntryPixels.Num() != EntryPtr->Width * EntryPtr->Height)
			{
				continue;
			}

			const int32 DestStartX = EntryPtr->X - Group.Rect.Min.X;
			const int32 DestStartY = EntryPtr->Y - Group.Rect.Min.Y;
			for (int32 RowIndex = 0; RowIndex < EntryPtr->Height; ++RowIndex)
			{
				const int32 SourceOffset = RowIndex * EntryPtr->Width;
				const int32 DestOffset = (DestStartY + RowIndex) * GroupWidth + DestStartX;
				FMemory::Memcpy(
					GroupPixels.GetData() + DestOffset,
					EntryPixels.GetData() + SourceOffset,
					EntryPtr->Width * sizeof(FColor));
			}
		}

		FRuntimeMeshPaintPatchHistoryEntry CompactedEntry;
		CompactedEntry.PaintTargetName = Group.PaintTargetName;
		CompactedEntry.MeshTargetName = Group.MeshTargetName;
		CompactedEntry.MeshTargetIndex = Group.MeshTargetIndex;
		CompactedEntry.TextureType = Group.TextureType;
		CompactedEntry.X = Group.Rect.Min.X;
		CompactedEntry.Y = Group.Rect.Min.Y;
		CompactedEntry.Width = GroupWidth;
		CompactedEntry.Height = GroupHeight;
		CompactedEntry.RTWidth = Group.RTWidth;
		CompactedEntry.RTHeight = Group.RTHeight;
		CompactedEntry.RTFormat = Group.RTFormat;
		CompactedEntry.UVChannel = Group.UVChannel;
		CompactedEntry.SequenceId = Group.LastSequenceId;
		CompactedEntry.UncompressedByteCount = GroupPixels.Num() * static_cast<int32>(sizeof(FColor));
		if (CompressPatchColorBytes(GroupPixels, bCompressPatchHistory, CompactedEntry.PixelBytes, CompactedEntry.bCompressed))
		{
			CompactedEntries.Add(MoveTemp(CompactedEntry));
		}
	}

	CompactedEntries.Sort([](
		const FRuntimeMeshPaintPatchHistoryEntry& Left,
		const FRuntimeMeshPaintPatchHistoryEntry& Right)
	{
		return Left.SequenceId < Right.SequenceId;
	});

	OutCompactedHistory.Entries = MoveTemp(CompactedEntries);
	return OutCompactedHistory.Entries.Num() > 0;
}

int32 URuntimeMeshPaintTargetComponent::GetPaintPatchHistoryEntryCount() const
{
	return PatchHistory.Num();
}

bool URuntimeMeshPaintTargetComponent::ResolvePatchPixels(
	const FRuntimeMeshPaintPatchHistoryEntry& Entry,
	TArray<FColor>& OutPixels) const
{
	OutPixels.Reset();
	if (!Entry.IsValidPatch()) return false;

	TArray<uint8> RawBytes;
	if (Entry.bCompressed)
	{
		RawBytes.SetNumUninitialized(Entry.UncompressedByteCount);
		if (!FCompression::UncompressMemory(
			NAME_Zlib,
			RawBytes.GetData(),
			Entry.UncompressedByteCount,
			Entry.PixelBytes.GetData(),
			Entry.PixelBytes.Num()))
		{
			return false;
		}
	}
	else
	{
		if (Entry.PixelBytes.Num() != Entry.UncompressedByteCount) return false;
		RawBytes = Entry.PixelBytes;
	}

	OutPixels.SetNumUninitialized(Entry.Width * Entry.Height);
	FMemory::Memcpy(OutPixels.GetData(), RawBytes.GetData(), Entry.UncompressedByteCount);
	return true;
}

UTextureRenderTarget2D* URuntimeMeshPaintTargetComponent::ResolvePatchRenderTarget(const FRuntimeMeshPaintPatchHistoryEntry& Entry)
{
	UMeshComponent* MeshTarget = ResolveMeshTargetString(Entry.MeshTargetName);
	if (!MeshTarget && Entry.MeshTargetIndex != INDEX_NONE)
	{
		MeshTarget = GetMeshTargetByIndex(Entry.MeshTargetIndex);
	}

	if (!IsValid(MeshTarget)) return nullptr;

	FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData = FindOrAddRuntimeDataForMesh(MeshTarget);
	if (!InitializeRuntimeDataForMesh(RuntimeData)) return nullptr;

	TObjectPtr<UTextureRenderTarget2D>* TargetRenderTargetPtr =
		Entry.TextureType == ERuntimeMeshPaintPatchTextureType::MaterialSettings
			? &RuntimeData.PaintedMaterialSettingsRenderTarget
			: &RuntimeData.PaintedColorRenderTarget;

	UTextureRenderTarget2D* TargetRenderTarget = TargetRenderTargetPtr ? TargetRenderTargetPtr->Get() : nullptr;
	if (!TargetRenderTarget ||
		TargetRenderTarget->SizeX != Entry.RTWidth ||
		TargetRenderTarget->SizeY != Entry.RTHeight ||
		TargetRenderTarget->RenderTargetFormat != Entry.RTFormat)
	{
		const FLinearColor ClearColor =
			Entry.TextureType == ERuntimeMeshPaintPatchTextureType::MaterialSettings
				? MakeMaterialSettingsClearColor(InitialMaterialSettingsColor)
				: MakeNeutralTransparentPaintClearColor();
		TargetRenderTarget = CreateRuntimePaintRenderTargetWithFormat(
			Entry.RTWidth,
			Entry.RTHeight,
			Entry.RTFormat,
			ClearColor);
		if (TargetRenderTargetPtr)
		{
			*TargetRenderTargetPtr = TargetRenderTarget;
		}
	}

	return TargetRenderTarget;
}

bool URuntimeMeshPaintTargetComponent::ApplyPatchHistoryEntry(
	const FRuntimeMeshPaintPatchHistoryEntry& Entry,
	bool bPreserveExistingPixelsForNeutralPatchPixels)
{
	UTextureRenderTarget2D* RenderTarget = ResolvePatchRenderTarget(Entry);
	if (!RenderTarget) return false;
	if (Entry.X < 0 || Entry.Y < 0 || Entry.X + Entry.Width > RenderTarget->SizeX || Entry.Y + Entry.Height > RenderTarget->SizeY)
	{
		return false;
	}

	TArray<FColor> Pixels;
	if (!ResolvePatchPixels(Entry, Pixels)) return false;
	if (Pixels.Num() != Entry.Width * Entry.Height) return false;

	if (bPreserveExistingPixelsForNeutralPatchPixels)
	{
		FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
		if (!RenderTargetResource) return false;

		TArray<FColor> ExistingPixels;
		FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
		ReadFlags.SetLinearToGamma(false);
		const FIntRect PatchRect(Entry.X, Entry.Y, Entry.X + Entry.Width, Entry.Y + Entry.Height);
		if (!RenderTargetResource->ReadPixels(ExistingPixels, ReadFlags, PatchRect) ||
			ExistingPixels.Num() != Pixels.Num())
		{
			return false;
		}

		for (int32 PixelIndex = 0; PixelIndex < Pixels.Num(); ++PixelIndex)
		{
			if (IsNeutralPatchPixel(Pixels[PixelIndex], Entry.TextureType, InitialMaterialSettingsColor))
			{
				Pixels[PixelIndex] = ExistingPixels[PixelIndex];
			}
		}
	}

	TArray<uint8> RawBytes;
	if (!CopyColorsToBytes(Pixels, RawBytes)) return false;

	UTexture2D* PatchTexture = UTexture2D::CreateTransient(Entry.Width, Entry.Height, PF_B8G8R8A8);
	if (!PatchTexture || !PatchTexture->GetPlatformData() || PatchTexture->GetPlatformData()->Mips.Num() == 0)
	{
		return false;
	}

	PatchTexture->SRGB = false;
	PatchTexture->Filter = TF_Nearest;
	PatchTexture->AddressX = TA_Clamp;
	PatchTexture->AddressY = TA_Clamp;
	PatchTexture->NeverStream = true;
	PatchTexture->CompressionSettings = TC_VectorDisplacementmap;
#if WITH_EDITORONLY_DATA
	PatchTexture->MipGenSettings = TMGS_NoMipmaps;
#endif

	FTexture2DMipMap& Mip = PatchTexture->GetPlatformData()->Mips[0];
	void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	if (!TextureData)
	{
		Mip.BulkData.Unlock();
		return false;
	}
	FMemory::Memcpy(TextureData, RawBytes.GetData(), RawBytes.Num());
	Mip.BulkData.Unlock();
	PatchTexture->UpdateResource();

	UCanvas* Canvas = nullptr;
	FVector2D RenderTargetSize = FVector2D::ZeroVector;
	FDrawToRenderTargetContext DrawContext;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, RenderTargetSize, DrawContext);
	if (!Canvas)
	{
		UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, DrawContext);
		return false;
	}

	Canvas->K2_DrawTexture(
		PatchTexture,
		FVector2D(Entry.X, Entry.Y),
		FVector2D(Entry.Width, Entry.Height),
		FVector2D::ZeroVector,
		FVector2D::UnitVector,
		FLinearColor::White,
		BLEND_Opaque,
		0.0f,
		FVector2D::ZeroVector);
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, DrawContext);
	return true;
}

void URuntimeMeshPaintTargetComponent::ApplyBrushMaterialSettings_Implementation(const FMeshPaintBrushMaterialSettings& Settings)
{
	BrushMaterialSettings = Settings;
	BrushMaterialSettings.Clamp();
	OnBrushSettingsApplied.Broadcast(BrushMaterialSettings);
}

bool URuntimeMeshPaintTargetComponent::SamplePaintedSurfaceColor_Implementation(const FHitResult& HitResult, FRuntimeMeshPaintSampleResult& OutSampleResult)
{
	if (!ResolvePaintHit(HitResult, OutSampleResult)) return false;

	const FRuntimeMeshPaintTargetMeshRuntimeData* RuntimeData = FindRuntimeDataForHit(OutSampleResult.HitResult);
	if (!RuntimeData || !RuntimeData->PaintedColorRenderTarget)
	{
		OutSampleResult.bSuccess = false;
		return false;
	}

	const FLinearColor SampledColor = UKismetRenderingLibrary::ReadRenderTargetRawUV(
		this, RuntimeData->PaintedColorRenderTarget, OutSampleResult.UV.X, OutSampleResult.UV.Y, true);
	if (SampledColor.A <= KINDA_SMALL_NUMBER)
	{
		OutSampleResult.bSuccess = false;
		return false;
	}

	OutSampleResult.bSuccess = true;
	OutSampleResult.Color = SampledColor;
	OutSampleResult.Color.A = 1.0f;
	return true;
}

bool URuntimeMeshPaintTargetComponent::TracePaintSurfaceUnderCursor(
	APlayerController* PlayerController, FHitResult& OutHitResult,
	ECollisionChannel TraceChannel, bool bTraceComplex) const
{
	SCOPE_CYCLE_COUNTER(STAT_MeshPaintingCore_TracePaintUnderCursor);

	OutHitResult = FHitResult();
	if (!PlayerController) return false;

	FVector TraceStart = FVector::ZeroVector;
	FVector TraceDirection = FVector::ForwardVector;
	if (!PlayerController->DeprojectMousePositionToWorld(TraceStart, TraceDirection)) return false;

	const FVector TraceEnd = TraceStart + (TraceDirection * RuntimeMeshPaintTraceDistance);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RuntimeMeshPaintUnderCursor), bTraceComplex);
	QueryParams.bReturnFaceIndex = true;
	QueryParams.bReturnPhysicalMaterial = true;

	const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, TraceChannel, QueryParams);
	return bHit;
}

bool URuntimeMeshPaintTargetComponent::ResolvePaintHit(
	const FHitResult& HitResult, FRuntimeMeshPaintSampleResult& OutPaintResult) const
{
	SCOPE_CYCLE_COUNTER(STAT_MeshPaintingCore_ResolvePaintHit);

	OutPaintResult = FRuntimeMeshPaintSampleResult();
	OutPaintResult.HitResult = HitResult;

	UMeshComponent* PaintMeshTarget = ResolveMeshTargetForHit(HitResult);
	if (!PaintMeshTarget) return false;
	const FRuntimeMeshPaintTargetMeshRuntimeData* RuntimeData = FindRuntimeDataForMesh(PaintMeshTarget);
	if (!RuntimeData || !RuntimeData->PaintedColorRenderTarget) return false;

	FHitResult PaintHitResult = HitResult;
	if (PaintHitResult.GetComponent() != PaintMeshTarget)
	{
		PaintHitResult.Component = PaintMeshTarget;
		PaintHitResult.FaceIndex = INDEX_NONE;
		PaintHitResult.Item = INDEX_NONE;

		USkeletalMeshComponent* SkeletalMeshTarget = Cast<USkeletalMeshComponent>(PaintMeshTarget);
		if (!SkeletalMeshTarget || PaintHitResult.BoneName.IsNone() ||
			SkeletalMeshTarget->GetBoneIndex(PaintHitResult.BoneName) == INDEX_NONE)
		{
			PaintHitResult.BoneName = NAME_None;
		}
	}

	FVector2D UV;
	int32 ResolvedFaceIndex = INDEX_NONE;
	int32 ResolvedTriangleArrayIndex = INDEX_NONE;
	if (!RuntimeMeshPaint::FindPaintHitUV(
		PaintHitResult,
		UVChannel,
		MaxSkeletalMeshUVFallbackDistance,
		UV,
		&ResolvedFaceIndex,
		&ResolvedTriangleArrayIndex))
	{
		return false;
	}

	UV.X = FMath::Clamp(UV.X, 0.0, 1.0);
	UV.Y = FMath::Clamp(UV.Y, 0.0, 1.0);
	OutPaintResult.HitResult = PaintHitResult;

	if (ResolvedFaceIndex != INDEX_NONE)
	{
		OutPaintResult.HitResult.FaceIndex = ResolvedFaceIndex;
	}

	if (bClipBrushToUVIsland)
	{
		TSharedPtr<FPaintUVCache> CoverageCache;
		int32 CoverageIslandId = INDEX_NONE;
		if (!ResolvePaintUVIslandCache(
			PaintMeshTarget,
			UVChannel,
			UVIslandConnectionTolerance,
			ResolvedTriangleArrayIndex,
			OutPaintResult.HitResult.FaceIndex,
			UV,
			CoverageCache,
			CoverageIslandId))
		{
			return false;
		}

		OutPaintResult.ResolvedTriangleArrayIndex = ResolvedTriangleArrayIndex;
		OutPaintResult.ResolvedUVIslandId = CoverageIslandId;
	}
	else
	{
		OutPaintResult.ResolvedTriangleArrayIndex = ResolvedTriangleArrayIndex;
		OutPaintResult.ResolvedUVIslandId = INDEX_NONE;
	}

	FVector SurfaceWorldPosition = FVector::ZeroVector;
	FVector SurfaceWorldNormal = FVector::ZeroVector;
	if (RuntimeMeshPaint::ResolvePaintHitSurfaceData(
		OutPaintResult.HitResult,
		UVChannel,
		UV,
		SurfaceWorldPosition,
		SurfaceWorldNormal))
	{
		OutPaintResult.HitResult.ImpactPoint = SurfaceWorldPosition;
		OutPaintResult.HitResult.Location = SurfaceWorldPosition;
		OutPaintResult.HitResult.ImpactNormal = SurfaceWorldNormal;
		OutPaintResult.HitResult.Normal = SurfaceWorldNormal;
	}

	OutPaintResult.bSuccess = true;
	OutPaintResult.UV = UV;
	return true;
}

bool URuntimeMeshPaintTargetComponent::IsPaintableHit(const FHitResult& HitResult) const
{
	if (!HitResult.bBlockingHit) return false;

	const FRuntimeMeshPaintTargetMeshRuntimeData* RuntimeData = FindRuntimeDataForHit(HitResult);
	return RuntimeData &&
		RuntimeData->PaintedColorRenderTarget &&
		HasMeshTarget(RuntimeData->MeshTarget.Get());
}

bool URuntimeMeshPaintTargetComponent::ResolvePaintHitUnderCursor(
	APlayerController* PlayerController, FRuntimeMeshPaintSampleResult& OutPaintResult,
	ECollisionChannel TraceChannel, bool bTraceComplex) const
{
	OutPaintResult = FRuntimeMeshPaintSampleResult();

	if (!HasValidMeshTarget()) return false;

	FHitResult HitResult;
	if (!TracePaintSurfaceUnderCursor(PlayerController, HitResult, TraceChannel, bTraceComplex)) return false;

	if (!ResolveProjectedPaintHit(HitResult, OutPaintResult)) return false;

	if (Cast<USkeletalMeshComponent>(OutPaintResult.HitResult.GetComponent()))
	{
		if (!BuildRuntimeMeshPaintScreenProjectionData(PlayerController, OutPaintResult.ProjectionData))
		{
			OutPaintResult.bSuccess = false;
			return false;
		}
	}
	return true;
}

bool URuntimeMeshPaintTargetComponent::ResolveProjectedPaintHit(
	const FHitResult& HitResult,
	FRuntimeMeshPaintSampleResult& OutPaintResult) const
{
	OutPaintResult = FRuntimeMeshPaintSampleResult();
	OutPaintResult.HitResult = HitResult;

	UMeshComponent* PaintMeshTarget = ResolveMeshTargetForHit(HitResult);
	if (!PaintMeshTarget) return false;

	const FRuntimeMeshPaintTargetMeshRuntimeData* RuntimeData = FindRuntimeDataForMesh(PaintMeshTarget);
	if (!RuntimeData || !RuntimeData->PaintedColorRenderTarget) return false;

	FHitResult PaintHitResult = HitResult;
	if (PaintHitResult.GetComponent() != PaintMeshTarget)
	{
		PaintHitResult.Component = PaintMeshTarget;
		PaintHitResult.FaceIndex = INDEX_NONE;
		PaintHitResult.Item = INDEX_NONE;

		USkeletalMeshComponent* SkeletalMeshTarget = Cast<USkeletalMeshComponent>(PaintMeshTarget);
		if (!SkeletalMeshTarget || PaintHitResult.BoneName.IsNone() ||
			SkeletalMeshTarget->GetBoneIndex(PaintHitResult.BoneName) == INDEX_NONE)
		{
			PaintHitResult.BoneName = NAME_None;
		}
	}

	FVector PreviewNormal = PaintHitResult.ImpactNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
	const FVector ToTraceStart = (PaintHitResult.TraceStart - PaintHitResult.ImpactPoint).GetSafeNormal();
	if (!ToTraceStart.IsNearlyZero() && FVector::DotProduct(PreviewNormal, ToTraceStart) < 0.0)
	{
		PreviewNormal *= -1.0;
	}
	PaintHitResult.ImpactNormal = PreviewNormal;
	PaintHitResult.Normal = PreviewNormal;
	if (PaintHitResult.TraceStart.Equals(PaintHitResult.TraceEnd))
	{
		PaintHitResult.TraceStart = PaintHitResult.ImpactPoint + PreviewNormal * RuntimeMeshPaintTraceDistance;
		PaintHitResult.TraceEnd = PaintHitResult.ImpactPoint - PreviewNormal * RuntimeMeshPaintTraceDistance;
	}

	OutPaintResult.HitResult = PaintHitResult;
	OutPaintResult.bSuccess = true;
	return true;
}

bool URuntimeMeshPaintTargetComponent::UpdateProjectedBrushPreviewMask(
	const FRuntimeMeshPaintSampleResult& PaintHit,
	float BrushRadius,
	float BrushRadiusScale,
	const FLinearColor& PreviewColor,
	float PreviewLineThickness)
{
	if (!PaintHit.bSuccess || GetNetMode() == NM_DedicatedServer) return false;

	FRuntimeMeshPaintTargetMeshRuntimeData* RuntimeData = FindRuntimeDataForHit(PaintHit.HitResult);
	if (!RuntimeData) return false;

	if (!InitializeRuntimeDataForMesh(*RuntimeData) || !RuntimeData->BrushPreviewMaskRenderTarget) return false;

	const float BrushWorldRadius =
		FMath::Max(BrushRadius * FMath::Max(0.0f, BrushRadiusScale), KINDA_SMALL_NUMBER) *
		RuntimeMeshPaintTargetBrushSizeToWorldRadiusScale;
	const float PreviewLineWorldThickness = FMath::Max(
		PreviewLineThickness,
		BrushWorldRadius * 0.025f);

	return FRuntimeMeshPaintGPUBrushRenderer::DrawProjectedBrushPreviewMask(
		RuntimeData->MeshTarget.Get(),
		RuntimeData->BrushPreviewMaskRenderTarget,
		UVChannel,
		UVIslandConnectionTolerance,
		PaintHit.HitResult.TraceStart,
		PaintHit.HitResult.TraceEnd,
		PaintHit.HitResult.ImpactPoint,
		PaintHit.HitResult.ImpactNormal,
		BrushWorldRadius,
		PaintHit.ProjectionData,
		PreviewColor,
		PreviewLineWorldThickness);
}

void URuntimeMeshPaintTargetComponent::ClearBrushPreviewMask(UMeshComponent* MeshTarget)
{
	auto ClearRuntimeData = [this](FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData)
	{
		if (RuntimeData.BrushPreviewMaskRenderTarget)
		{
			UKismetRenderingLibrary::ClearRenderTarget2D(
				this,
				RuntimeData.BrushPreviewMaskRenderTarget,
				FLinearColor::Transparent);
		}
	};

	if (MeshTarget)
	{
		if (FRuntimeMeshPaintTargetMeshRuntimeData* RuntimeData = FindRuntimeDataForMesh(MeshTarget))
		{
			ClearRuntimeData(*RuntimeData);
		}
		return;
	}

	for (FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData : MeshRuntimeData)
	{
		ClearRuntimeData(RuntimeData);
	}
}

bool URuntimeMeshPaintTargetComponent::PaintUnderCursor(
	APlayerController* PlayerController, UMaterialInterface* BrushMaterial, float BrushRadius,
	FRuntimeMeshPaintSampleResult& OutPaintResult, ECollisionChannel TraceChannel,
	bool bTraceComplex)
{
	FMeshPaintBrushMaterialSettings BrushSettings = BrushMaterialSettings;
	BrushSettings.BrushSize = BrushRadius;
	return PaintUnderCursorWithSettings(PlayerController, BrushMaterial, BrushSettings, OutPaintResult, TraceChannel, bTraceComplex);
}

bool URuntimeMeshPaintTargetComponent::PaintUnderCursorWithSettings(
	APlayerController* PlayerController, UMaterialInterface* BrushMaterial,
	const FMeshPaintBrushMaterialSettings& BrushSettings, FRuntimeMeshPaintSampleResult& OutPaintResult,
	ECollisionChannel TraceChannel, bool bTraceComplex)
{
	OutPaintResult = FRuntimeMeshPaintSampleResult();

	if (!HasValidMeshTarget()) return false;

	FHitResult HitResult;
	if (!TracePaintSurfaceUnderCursor(PlayerController, HitResult, TraceChannel, bTraceComplex)) return false;

	FRuntimeMeshPaintSampleResult PaintHit;
	if (!ResolveProjectedPaintHit(HitResult, PaintHit))
	{
		OutPaintResult = PaintHit;
		return false;
	}

	if (Cast<USkeletalMeshComponent>(PaintHit.HitResult.GetComponent()))
	{
		if (!BuildRuntimeMeshPaintScreenProjectionData(PlayerController, PaintHit.ProjectionData))
		{
			OutPaintResult = PaintHit;
			OutPaintResult.bSuccess = false;
			return false;
		}
	}
	return PaintProjectedHitWithSettings(PaintHit, BrushMaterial, BrushSettings, OutPaintResult);
}

bool URuntimeMeshPaintTargetComponent::PaintAtHit(
	const FHitResult& HitResult, UMaterialInterface* BrushMaterial,
	float BrushRadius, FRuntimeMeshPaintSampleResult& OutPaintResult)
{
	FMeshPaintBrushMaterialSettings BrushSettings = BrushMaterialSettings;
	BrushSettings.BrushSize = BrushRadius;
	return PaintAtHitWithSettings(HitResult, BrushMaterial, BrushSettings, OutPaintResult);
}

bool URuntimeMeshPaintTargetComponent::PaintAtHitWithSettings(
	const FHitResult& HitResult, UMaterialInterface* BrushMaterial,
	const FMeshPaintBrushMaterialSettings& BrushSettings, FRuntimeMeshPaintSampleResult& OutPaintResult)
{
	OutPaintResult = FRuntimeMeshPaintSampleResult();

	FRuntimeMeshPaintSampleResult PaintHit;
	if (!ResolveProjectedPaintHit(HitResult, PaintHit))
	{
		OutPaintResult = PaintHit;
		return false;
	}

	return PaintProjectedHitWithSettings(PaintHit, BrushMaterial, BrushSettings, OutPaintResult);
}

bool URuntimeMeshPaintTargetComponent::PaintProjectedHitWithSettings(
	const FRuntimeMeshPaintSampleResult& PaintHit, UMaterialInterface* /*BrushMaterial*/,
	const FMeshPaintBrushMaterialSettings& BrushSettings, FRuntimeMeshPaintSampleResult& OutPaintResult)
{
	SCOPE_CYCLE_COUNTER(STAT_MeshPaintingCore_PaintAtHitWithSettings);
	INC_DWORD_STAT(STAT_MeshPaintingCore_PaintCalls);

	OutPaintResult = PaintHit;
	if (!PaintHit.bSuccess) return false;
	OutPaintResult.bSuccess = false;

	FRuntimeMeshPaintTargetMeshRuntimeData* RuntimeData = FindRuntimeDataForHit(OutPaintResult.HitResult);
	if (!RuntimeData || !RuntimeData->PaintedColorRenderTarget) return false;

	FMeshPaintBrushMaterialSettings SafeBrushSettings = BrushSettings;
	SafeBrushSettings.Clamp();

	const float BrushWorldRadius =
		FMath::Max(SafeBrushSettings.BrushSize, KINDA_SMALL_NUMBER) * RuntimeMeshPaintTargetBrushSizeToWorldRadiusScale;

	{
		SCOPE_CYCLE_COUNTER(STAT_MeshPaintingCore_DrawBrushToRenderTarget);
		if (!FRuntimeMeshPaintGPUBrushRenderer::DrawProjectedBrush(
			RuntimeData->MeshTarget.Get(),
			RuntimeData->PaintedColorRenderTarget,
			RuntimeData->PaintedMaterialSettingsRenderTarget,
			UVChannel,
			UVIslandConnectionTolerance,
			OutPaintResult.HitResult.TraceStart,
			OutPaintResult.HitResult.TraceEnd,
			OutPaintResult.HitResult.ImpactPoint,
			OutPaintResult.HitResult.ImpactNormal,
			BrushWorldRadius,
			OutPaintResult.ProjectionData,
			SafeBrushSettings))
		{
			return false;
		}
	}

	INC_DWORD_STAT(STAT_MeshPaintingCore_RenderTargetDraws);
	OutPaintResult.bSuccess = true;
	OutPaintResult.Color = SafeBrushSettings.Color;
	INC_DWORD_STAT(STAT_MeshPaintingCore_SuccessfulPaintCalls);
	QueuePatchCaptureForPaint(OutPaintResult, SafeBrushSettings, *RuntimeData);
	OnPaintApplied.Broadcast(OutPaintResult);
	return true;
}

UTextureRenderTarget2D* URuntimeMeshPaintTargetComponent::CreateRuntimePaintRenderTarget(int32 Width, int32 Height, const FLinearColor& ClearColor)
{
	return CreateRuntimePaintRenderTargetWithFormat(Width, Height, RuntimeRenderTargetFormat, ClearColor);
}

UTextureRenderTarget2D* URuntimeMeshPaintTargetComponent::CreateRuntimePaintRenderTargetWithFormat(
	int32 Width,
	int32 Height,
	ETextureRenderTargetFormat RenderTargetFormat,
	const FLinearColor& ClearColor)
{
	if (Width <= 0 || Height <= 0) return nullptr;

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	if (!RenderTarget) return nullptr;

	RenderTarget->RenderTargetFormat = RenderTargetFormat;
	RenderTarget->ClearColor = ClearColor;
	ConfigurePaintRenderTarget(RenderTarget);
	RenderTarget->InitAutoFormat(Width, Height);
	RenderTarget->UpdateResourceImmediate(true);
	return RenderTarget;
}

UTextureRenderTarget2D* URuntimeMeshPaintTargetComponent::CreateRuntimeBrushPreviewMaskRenderTarget(int32 Width, int32 Height)
{
	if (Width <= 0 || Height <= 0) return nullptr;

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	if (!RenderTarget) return nullptr;

	RenderTarget->RenderTargetFormat = RTF_RGBA16f;
	RenderTarget->ClearColor = FLinearColor::Transparent;
	ConfigurePaintRenderTarget(RenderTarget);
	RenderTarget->InitAutoFormat(Width, Height);
	RenderTarget->UpdateResourceImmediate(true);
	return RenderTarget;
}
