// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "MeshPaintingCoreTypes.h"
#include "Painting/RuntimeMeshPaintReplicationTypes.h"
#include "Painting/RuntimeMeshPaintTargetInterface.h"
#include "RuntimeMeshPaintTargetComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;
class UPaintingModeControllerComponent;
class AController;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRuntimeMeshPaintBrushSettingsApplied, const FMeshPaintBrushMaterialSettings&, Settings);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRuntimeMeshPaintApplied, FRuntimeMeshPaintSampleResult, PaintResult);

UENUM(BlueprintType)
enum class ERuntimeMeshPaintPatchTextureType : uint8
{
	Color UMETA(DisplayName = "Color"),
	MaterialSettings UMETA(DisplayName = "Material Settings")
};

UENUM(BlueprintType)
enum class ERuntimeMeshPaintTextureExportFormat : uint8
{
	PNG UMETA(DisplayName = "PNG"),
	EXR UMETA(DisplayName = "EXR"),
	HDR UMETA(DisplayName = "HDR")
};

USTRUCT(BlueprintType)
struct MESHPAINTINGCORE_API FRuntimeMeshPaintPatchHistoryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	FString PaintTargetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	FString MeshTargetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	int32 MeshTargetIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	ERuntimeMeshPaintPatchTextureType TextureType = ERuntimeMeshPaintPatchTextureType::Color;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	int32 Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	int32 Width = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	int32 Height = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	int32 RTWidth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	int32 RTHeight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	TEnumAsByte<ETextureRenderTargetFormat> RTFormat = RTF_RGBA16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	int32 UVChannel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	int32 SequenceId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	bool bCompressed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	int32 UncompressedByteCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	TArray<uint8> PixelBytes;

	bool IsValidPatch() const
	{
		return Width > 0 &&
			Height > 0 &&
			RTWidth > 0 &&
			RTHeight > 0 &&
			UncompressedByteCount == Width * Height * static_cast<int32>(sizeof(FColor)) &&
			PixelBytes.Num() > 0;
	}
};

USTRUCT(BlueprintType)
struct MESHPAINTINGCORE_API FRuntimeMeshPaintPatchHistory
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	int32 Version = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	int32 LastSequenceId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Runtime Mesh Painting|Patch History")
	TArray<FRuntimeMeshPaintPatchHistoryEntry> Entries;

	bool IsEmpty() const
	{
		return Entries.Num() == 0;
	}
};

USTRUCT()
struct FRuntimeMeshPaintTargetMeshRuntimeData
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UMeshComponent> MeshTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> PaintedColorRenderTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> PaintedMaterialSettingsRenderTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> BrushPreviewMaskRenderTarget = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PaintTargetMaterialInstances;
};

UCLASS(ClassGroup = (MeshPaintingCore), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class MESHPAINTINGCORE_API URuntimeMeshPaintTargetComponent : public UActorComponent, public IRuntimeMeshPaintTargetInterface
{
	GENERATED_BODY()

public:
	URuntimeMeshPaintTargetComponent();

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Runtime Mesh Painting")
	TObjectPtr<UTextureRenderTarget2D> PaintedColorRenderTarget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Runtime Mesh Painting")
	TObjectPtr<UTextureRenderTarget2D> PaintedMaterialSettingsRenderTarget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Runtime Mesh Painting")
	TObjectPtr<UTextureRenderTarget2D> BrushPreviewMaskRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Runtime Target", meta = (DisplayName = "Mesh Targets"))
	TArray<FString> MeshTargets;

	UPROPERTY(BlueprintReadWrite, Category = "Runtime Mesh Painting|Runtime Target")
	TObjectPtr<UMeshComponent> TargetMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Runtime Target")
	bool bCreatePaintedMaterialSettingsRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Runtime Target", meta = (ClampMin = "1"))
	int32 RuntimeRenderTargetWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Runtime Target", meta = (ClampMin = "1"))
	int32 RuntimeRenderTargetHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Runtime Target")
	TEnumAsByte<ETextureRenderTargetFormat> RuntimeRenderTargetFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Runtime Target")
	FLinearColor InitialPaintColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Runtime Target")
	FLinearColor InitialMaterialSettingsColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Runtime Target")
	FName PaintedColorTextureParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Runtime Target")
	FName PaintedMaterialSettingsTextureParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Runtime Target")
	FName BrushPreviewMaskTextureParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting", meta = (ClampMin = "0"))
	int32 UVChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting", meta = (ClampMin = "0.0"))
	float MaxSkeletalMeshUVFallbackDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|UV Island Clip")
	bool bClipBrushToUVIsland;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|UV Island Clip", meta = (ClampMin = "0.0", EditCondition = "bClipBrushToUVIsland"))
	float UVIslandConnectionTolerance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Replication")
	bool bReplicateRuntimePaint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Replication", meta = (EditCondition = "bReplicateRuntimePaint"))
	bool bAutoEnableOwnerReplication;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Replication", meta = (ClampMin = "0", EditCondition = "bReplicateRuntimePaint", ToolTip = "0 keeps the full command history for late join. Use a positive value only when an external checkpoint/save system exists."))
	int32 MaxReplicatedPaintCommands;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Replication", meta = (ClampMin = "1", EditCondition = "bReplicateRuntimePaint"))
	int32 ReplicatedPaintReplayCommandsPerTick;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Replication", meta = (ClampMin = "0.0", EditCondition = "bReplicateRuntimePaint", ToolTip = "0 disables distance validation."))
	float MaxReplicatedPaintDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Replication", meta = (ClampMin = "0.001", EditCondition = "bReplicateRuntimePaint"))
	float MaxReplicatedBrushSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Patch History", meta = (DisplayName = "Record Paint Patch History", ToolTip = "When disabled, painting skips all patch history tracking, dirty rect calculation, and save/export patch capture bookkeeping."))
	bool bRecordPaintPatchHistory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Patch History", meta = (ClampMin = "0", EditCondition = "bRecordPaintPatchHistory", ToolTip = "Patch readback is deferred until Export, Compact, or Flush so live painting does not stall."))
	int32 PatchCaptureBudgetPerTick;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Patch History", meta = (ClampMin = "0", EditCondition = "bRecordPaintPatchHistory", ToolTip = "0 keeps all patch entries."))
	int32 MaxPatchHistoryEntries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Patch History", meta = (ClampMin = "0", EditCondition = "bRecordPaintPatchHistory", ToolTip = "0 disables byte trimming."))
	int32 MaxPatchHistoryBytes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Patch History", meta = (ClampMin = "0.1", EditCondition = "bRecordPaintPatchHistory"))
	float PatchHistoryDirtyRectBrushSizeMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Patch History", meta = (ClampMin = "0", EditCondition = "bRecordPaintPatchHistory"))
	int32 PatchHistoryDirtyRectPaddingPixels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Runtime Mesh Painting|Patch History", meta = (EditCondition = "bRecordPaintPatchHistory"))
	bool bCompressPatchHistory;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Runtime Mesh Painting|Patch History")
	int32 PatchHistoryBytes;

	UPROPERTY(BlueprintAssignable, Category = "Runtime Mesh Painting|Events")
	FOnRuntimeMeshPaintBrushSettingsApplied OnBrushSettingsApplied;

	UPROPERTY(BlueprintAssignable, Category = "Runtime Mesh Painting|Events")
	FOnRuntimeMeshPaintApplied OnPaintApplied;

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting")
	void SetPaintedColorRenderTarget(UTextureRenderTarget2D* InRenderTarget);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting")
	void SetPaintedMaterialSettingsRenderTarget(UTextureRenderTarget2D* InRenderTarget);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Runtime Target")
	bool SetTargetMesh(UMeshComponent* NewTargetMesh);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Runtime Target")
	void SetMeshTargets(const TArray<UMeshComponent*>& NewMeshTargets);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Runtime Target")
	bool AddMeshTarget(UMeshComponent* NewMeshTarget);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Runtime Target")
	void ClearMeshTargets();

	UFUNCTION(BlueprintPure, Category = "Runtime Mesh Painting|Runtime Target")
	TArray<UMeshComponent*> GetMeshTargets() const;

	UFUNCTION(BlueprintPure, Category = "Runtime Mesh Painting|Runtime Target")
	bool HasMeshTarget(UMeshComponent* MeshComponent) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Runtime Target")
	bool InitializeRuntimePaintTarget();

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Runtime Target")
	bool ApplyRuntimePaintTexturesToTargetMesh();

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Export", meta = (AdvancedDisplay = "TextureType,ExportFormat,MeshTarget"))
	bool ExportPaintedRenderTargetToFile(
		const FString& DirectoryPath,
		const FString& FileName,
		FString& OutFilePath,
		ERuntimeMeshPaintPatchTextureType TextureType = ERuntimeMeshPaintPatchTextureType::Color,
		ERuntimeMeshPaintTextureExportFormat ExportFormat = ERuntimeMeshPaintTextureExportFormat::PNG,
		UMeshComponent* MeshTarget = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Export", meta = (AdvancedDisplay = "bExportMaterialSettings,ExportFormat"))
	int32 ExportAllPaintedRenderTargetsToFiles(
		const FString& DirectoryPath,
		const FString& FileNamePrefix,
		TArray<FString>& OutFilePaths,
		bool bExportColor = true,
		bool bExportMaterialSettings = true,
		ERuntimeMeshPaintTextureExportFormat ExportFormat = ERuntimeMeshPaintTextureExportFormat::PNG);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting")
	virtual void ApplyBrushMaterialSettings_Implementation(const FMeshPaintBrushMaterialSettings& Settings) override;

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting")
	virtual bool SamplePaintedSurfaceColor_Implementation(const FHitResult& HitResult, FRuntimeMeshPaintSampleResult& OutSampleResult) override;

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Paint")
	bool TracePaintSurfaceUnderCursor(
		APlayerController* PlayerController, FHitResult& OutHitResult,
		ECollisionChannel TraceChannel = ECC_Visibility, bool bTraceComplex = true) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Paint")
	bool ResolvePaintHit(const FHitResult& HitResult, FRuntimeMeshPaintSampleResult& OutPaintResult) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Paint")
	bool IsPaintableHit(const FHitResult& HitResult) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Paint")
	bool ResolvePaintHitUnderCursor(
		APlayerController* PlayerController, FRuntimeMeshPaintSampleResult& OutPaintResult,
		ECollisionChannel TraceChannel = ECC_Visibility, bool bTraceComplex = true) const;

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Preview")
	bool UpdateProjectedBrushPreviewMask(
		const FRuntimeMeshPaintSampleResult& PaintHit, float BrushRadius, float BrushRadiusScale,
		const FLinearColor& PreviewColor, float PreviewLineThickness);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Preview")
	void ClearBrushPreviewMask(UMeshComponent* MeshTarget = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Paint")
	bool PaintUnderCursor(
		APlayerController* PlayerController, UMaterialInterface* BrushMaterial, float BrushRadius,
		FRuntimeMeshPaintSampleResult& OutPaintResult, ECollisionChannel TraceChannel = ECC_Visibility,
		bool bTraceComplex = true);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Paint")
	bool PaintUnderCursorWithSettings(
		APlayerController* PlayerController, UMaterialInterface* BrushMaterial,
		const FMeshPaintBrushMaterialSettings& BrushSettings, FRuntimeMeshPaintSampleResult& OutPaintResult,
		ECollisionChannel TraceChannel = ECC_Visibility, bool bTraceComplex = true);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Paint")
	bool PaintAtHit(
		const FHitResult& HitResult, UMaterialInterface* BrushMaterial,
		float BrushRadius, FRuntimeMeshPaintSampleResult& OutPaintResult);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Paint")
	bool PaintAtHitWithSettings(
		const FHitResult& HitResult, UMaterialInterface* BrushMaterial,
		const FMeshPaintBrushMaterialSettings& BrushSettings, FRuntimeMeshPaintSampleResult& OutPaintResult);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Patch History")
	void FlushPendingPaintPatchCaptures();

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Patch History")
	void ClearPaintPatchHistory();

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Patch History")
	bool ExportPaintPatchHistory(FRuntimeMeshPaintPatchHistory& OutHistory);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Patch History")
	bool ImportPaintPatchHistory(
		const FRuntimeMeshPaintPatchHistory& History,
		bool bClearExistingPaint = true,
		bool bClearHistory = true);

	UFUNCTION(BlueprintCallable, Category = "Runtime Mesh Painting|Patch History")
	bool CompactPaintPatchHistory(FRuntimeMeshPaintPatchHistory& OutCompactedHistory);

	UFUNCTION(BlueprintPure, Category = "Runtime Mesh Painting|Patch History")
	int32 GetPaintPatchHistoryEntryCount() const;

	void HandleReplicatedPaintCommand(const FRuntimeMeshPaintNetCommand& Command);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	friend class UPaintingModeControllerComponent;

	UMeshComponent* ResolveInitialTargetMesh() const;
	void CollectAvailableMeshTargets(TArray<UMeshComponent*>& OutMeshTargets) const;
	UMeshComponent* ResolveMeshTargetString(const FString& MeshTargetString) const;
	FString MakeMeshTargetString(UMeshComponent* MeshComponent) const;
	void ResolveInitialMeshTargets();
	bool EnsureTargetMeshForBeginPlay();
	bool HasValidMeshTarget() const;
	UMeshComponent* GetPrimaryMeshTarget() const;
	void ResetRuntimePaintResources();
	UTextureRenderTarget2D* CreateRuntimePaintRenderTarget(int32 Width, int32 Height, const FLinearColor& ClearColor);
	UTextureRenderTarget2D* CreateRuntimePaintRenderTargetWithFormat(
		int32 Width,
		int32 Height,
		ETextureRenderTargetFormat RenderTargetFormat,
		const FLinearColor& ClearColor);
	UTextureRenderTarget2D* CreateRuntimeBrushPreviewMaskRenderTarget(int32 Width, int32 Height);
	bool ResolveProjectedPaintHit(const FHitResult& HitResult, FRuntimeMeshPaintSampleResult& OutPaintResult) const;
	bool PaintProjectedHitWithSettings(
		const FRuntimeMeshPaintSampleResult& PaintHit, UMaterialInterface* BrushMaterial,
		const FMeshPaintBrushMaterialSettings& BrushSettings, FRuntimeMeshPaintSampleResult& OutPaintResult);
	bool BuildReplicatedPaintCommand(
		const FRuntimeMeshPaintSampleResult& PaintHit,
		const FMeshPaintBrushMaterialSettings& BrushSettings,
		uint16 StrokeId,
		uint32 ClientPredictionKey,
		AActor* SourceActor,
		FRuntimeMeshPaintNetCommand& OutCommand) const;
	void RememberPredictedPaintCommand(const FRuntimeMeshPaintNetCommand& Command);
	bool AcceptReplicatedPaintCommand(
		const FRuntimeMeshPaintNetCommand& Command,
		AController* InstigatorController,
		bool bApplyLocally);
	bool ApplyPaintCommandLocal(const FRuntimeMeshPaintNetCommand& Command, FRuntimeMeshPaintSampleResult* OutPaintResult);
	bool ValidateReplicatedPaintCommand(const FRuntimeMeshPaintNetCommand& Command, const AController* InstigatorController) const;
	bool ShouldSkipPredictedPaintCommand(const FRuntimeMeshPaintNetCommand& Command) const;
	bool IsLocalPaintSourceActor(const AActor* SourceActor) const;
	uint64 MakeClientPredictionDedupKey(const FRuntimeMeshPaintNetCommand& Command) const;
	void AppendAuthoritativePaintCommand(const FRuntimeMeshPaintNetCommand& Command);
	void ProcessPendingReplicatedPaintCommands();
	void QueuePatchCaptureForPaint(
		const FRuntimeMeshPaintSampleResult& PaintHit,
		const FMeshPaintBrushMaterialSettings& BrushSettings,
		const FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData);
	void ProcessPendingPatchCaptures(int32 CaptureBudget);
	bool BuildPatchDirtyRect(
		const FRuntimeMeshPaintSampleResult& PaintHit,
		const FMeshPaintBrushMaterialSettings& BrushSettings,
		const UTextureRenderTarget2D* RenderTarget,
		FIntRect& OutDirtyRect) const;
	bool CapturePatch(
		UTextureRenderTarget2D* RenderTarget,
		ERuntimeMeshPaintPatchTextureType TextureType,
		const FIntRect& DirtyRect,
		UMeshComponent* MeshTarget,
		int32 MeshTargetIndex);
	void AddPatchHistoryEntry(FRuntimeMeshPaintPatchHistoryEntry&& Entry);
	bool ApplyPatchHistoryEntry(
		const FRuntimeMeshPaintPatchHistoryEntry& Entry,
		bool bPreserveExistingPixelsForNeutralPatchPixels = false);
	UTextureRenderTarget2D* ResolvePatchRenderTarget(const FRuntimeMeshPaintPatchHistoryEntry& Entry);
	bool ResolvePatchPixels(const FRuntimeMeshPaintPatchHistoryEntry& Entry, TArray<FColor>& OutPixels) const;
	void TrimPatchHistory();
	void RecalculatePatchHistoryBytes();
	bool ImportPaintPatchHistoryLocal(
		const FRuntimeMeshPaintPatchHistory& History,
		bool bClearExistingPaint,
		bool bClearHistory);
	UTextureRenderTarget2D* ResolvePaintedRenderTargetForExport(
		ERuntimeMeshPaintPatchTextureType TextureType,
		UMeshComponent* MeshTarget);
	bool ExportRenderTargetToImageFile(
		UTextureRenderTarget2D* RenderTarget,
		ERuntimeMeshPaintPatchTextureType TextureType,
		const FString& DirectoryPath,
		const FString& FileName,
		ERuntimeMeshPaintTextureExportFormat ExportFormat,
		FString& OutFilePath) const;
	int32 FindMeshTargetIndex(UMeshComponent* MeshTarget) const;
	UMeshComponent* GetMeshTargetByIndex(int32 MeshTargetIndex) const;
	FRuntimeMeshPaintTargetMeshRuntimeData* FindRuntimeDataForMesh(UMeshComponent* MeshTarget);
	const FRuntimeMeshPaintTargetMeshRuntimeData* FindRuntimeDataForMesh(UMeshComponent* MeshTarget) const;
	UMeshComponent* ResolveMeshTargetForHit(const FHitResult& HitResult) const;
	FRuntimeMeshPaintTargetMeshRuntimeData* FindRuntimeDataForHit(const FHitResult& HitResult);
	const FRuntimeMeshPaintTargetMeshRuntimeData* FindRuntimeDataForHit(const FHitResult& HitResult) const;
	FRuntimeMeshPaintTargetMeshRuntimeData& FindOrAddRuntimeDataForMesh(UMeshComponent* MeshTarget);
	bool InitializeRuntimeDataForMesh(FRuntimeMeshPaintTargetMeshRuntimeData& RuntimeData);
	void UpdatePrimaryRenderTargetAliases();

	UPROPERTY(Transient)
	TArray<FRuntimeMeshPaintTargetMeshRuntimeData> MeshRuntimeData;

	UPROPERTY(Replicated)
	FRuntimeMeshPaintReplicatedCommandHistory ReplicatedPaintHistory;

	TArray<FRuntimeMeshPaintNetCommand> PendingReplicatedPaintCommands;
	TSet<uint32> AppliedReplicatedPaintCommandIds;
	TSet<uint32> PredictedPaintCommandKeys;
	TSet<uint64> AcceptedClientPaintPredictionKeys;
	uint32 NextReplicatedPaintCommandId;
	int32 PendingReplicatedPaintCommandReadIndex;
	int32 NextPatchHistorySequenceId;

	FMeshPaintBrushMaterialSettings BrushMaterialSettings;

	struct FPendingPatchCaptureRequest
	{
		TWeakObjectPtr<UTextureRenderTarget2D> RenderTarget;
		TWeakObjectPtr<UMeshComponent> MeshTarget;
		ERuntimeMeshPaintPatchTextureType TextureType = ERuntimeMeshPaintPatchTextureType::Color;
		FIntRect DirtyRect;
		int32 MeshTargetIndex = INDEX_NONE;
	};

	TArray<FPendingPatchCaptureRequest> PendingPatchCaptureRequests;
	TArray<FRuntimeMeshPaintPatchHistoryEntry> PatchHistory;
};
