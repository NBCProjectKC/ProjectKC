// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GenericPlatform/ICursor.h"
#include "MeshPaintingCoreTypes.h"
#include "Painting/RuntimeMeshPaintReplicationTypes.h"
#include "Painting/RuntimeMeshPaintTargetComponent.h"
#include "Painting/RuntimeMeshPaintTargetInterface.h"
#include "PaintingModeControllerComponent.generated.h"

class ACharacter;
class AController;
class APawn;
class APlayerController;
class UCameraComponent;
class UCharacterMovementComponent;
class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UInputMappingContext;
class UMaterialInterface;
class UPrimitiveComponent;
class UColorPickerPanelWidget;
class USpringArmComponent;
class UTexture2D;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPaintingModeSimpleEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPaintingModeBrushSizeChangedEvent, float, NewSize);

UENUM(BlueprintType)
enum class EPaintingModeControllerControlMode : uint8
{
	CharacterLock = 0 UMETA(DisplayName = "Third Person Controller"),
	DroneController = 1 UMETA(DisplayName = "Drone Controller"),
	FirstPersonController = 4 UMETA(DisplayName = "First Person Controller"),
	Simple = 3 UMETA(DisplayName = "Simple"),
	None = 2 UMETA(DisplayName = "None")
};

UCLASS(ClassGroup = (MeshPaintingCore), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class MESHPAINTINGCORE_API UPaintingModeControllerComponent : public UActorComponent, public IRuntimeMeshPaintTargetInterface
{
	GENERATED_BODY()

public:
	UPaintingModeControllerComponent();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Controls")
	EPaintingModeControllerControlMode ControlMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	TObjectPtr<UInputMappingContext> PaintingInputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	TObjectPtr<UInputAction> PaintingMoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	TObjectPtr<UInputAction> PaintAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	TObjectPtr<UInputAction> OrbitCameraAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	TObjectPtr<UInputAction> PanCameraAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	TObjectPtr<UInputAction> AdjustBrushSizeAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	TObjectPtr<UInputAction> MouseDeltaAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	TObjectPtr<UInputAction> PickColorAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	TObjectPtr<UInputAction> CameraZoomAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	TObjectPtr<UInputAction> AdjustBrushSizeWheelAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	TObjectPtr<UInputAction> TogglePaintingModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	int32 PaintingInputPriority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	bool bLoadDefaultInputAssets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Input")
	TObjectPtr<UInputMappingContext> PaintingToggleInputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Widget")
	bool bAutoCreateColorPickerWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Widget", meta = (EditCondition = "bAutoCreateColorPickerWidget"))
	TSubclassOf<UColorPickerPanelWidget> ColorPickerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Widget", meta = (EditCondition = "bAutoCreateColorPickerWidget"))
	int32 ColorPickerWidgetZOrder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Color Picker", meta = (ToolTip = "Controls how 3D eyedropper samples world colors. Mesh Unlit Color reads the mesh base color without lighting. Viewport Lit Color reads the visible lit screen color."))
	ERuntimeMeshPaintColorSampleMode ColorPickerSampleMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Cursor")
	bool bUseBrushCursorOutsideUI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Cursor", meta = (EditCondition = "bUseBrushCursorOutsideUI"))
	TSoftObjectPtr<UTexture2D> BrushCursorTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Cursor", meta = (EditCondition = "bUseBrushCursorOutsideUI"))
	FVector2D BrushCursorHotSpot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintGetter = IsBrushAreaPreviewEnabled, BlueprintSetter = SetBrushAreaPreviewEnabled, Category = "Painting Mode|Brush Area Preview")
	bool bEnableBrushAreaPreview;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Brush Area Preview", meta = (ClampMin = "0.1", EditCondition = "bEnableBrushAreaPreview"))
	float BrushAreaPreviewLineThickness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Brush Area Preview", meta = (ClampMin = "0.0", EditCondition = "bEnableBrushAreaPreview"))
	float BrushAreaPreviewThicknessBrushSizeMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Brush Area Preview", meta = (EditCondition = "bEnableBrushAreaPreview"))
	FLinearColor BrushAreaPreviewColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Brush Area Preview", meta = (ClampMin = "0.0", EditCondition = "bEnableBrushAreaPreview"))
	float BrushAreaPreviewEmissiveIntensity;

	UPROPERTY(BlueprintReadWrite, Category = "Painting Mode|Brush Area Preview", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BrushAreaPreviewOpacity;

	UPROPERTY(BlueprintReadWrite, Category = "Painting Mode|Brush Area Preview", meta = (ClampMin = "0.0"))
	float BrushAreaPreviewBrushSizeScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Paint Target")
	bool bAutoRegister;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Paint Target", meta = (DisplayName = "Paint Target Components", ToolTip = "Leave empty to allow any hit RuntimeMeshPaintTargetComponent. If set, only these target components can be painted."))
	TArray<TObjectPtr<URuntimeMeshPaintTargetComponent>> PaintTargetComponents;

	UPROPERTY(BlueprintReadWrite, Category = "Painting Mode|Paint Target")
	TObjectPtr<URuntimeMeshPaintTargetComponent> PaintTargetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Paint")
	TSoftObjectPtr<UMaterialInterface> PaintBrushMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Paint")
	TEnumAsByte<ECollisionChannel> PaintTraceChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Paint")
	bool bPaintTraceComplex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Movement", meta = (ClampMin = "0.0"))
	float PaintingMovementSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Orbit")
	float OrbitYawSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Orbit")
	float OrbitPitchSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Orbit")
	float MinimumOrbitPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Orbit")
	float MaximumOrbitPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Camera", meta = (ClampMin = "0.0"))
	float CameraZoomSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Camera", meta = (ClampMin = "0.0"))
	float MinimumCameraZoomDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Camera", meta = (ClampMin = "0.0"))
	float MaximumCameraZoomDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Camera", meta = (ClampMin = "0.0"))
	float CameraPanSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Camera", meta = (ClampMin = "0.0"))
	float CameraPanMaxOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Camera", meta = (ClampMin = "0.0"))
	float CameraRestoreSmoothingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Camera", meta = (ToolTip = "Optional CameraComponent name used by First Person and Drone controller modes. Third Person Controller always requires a SpringArmComponent."))
	FName CameraName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Brush", meta = (ClampMin = "0.0"))
	float BrushSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Brush")
	FLinearColor BrushColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Brush", meta = (ClampMin = "0.0"))
	float MinBrushSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Brush", meta = (ClampMin = "0.0"))
	float MaxBrushSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Brush")
	float BrushSizeSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Brush")
	float BrushSizeWheelSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Brush", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BrushMetallic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Brush", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BrushRoughness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Painting Mode|Brush")
	bool bBrushErase;

	UPROPERTY(BlueprintReadOnly, Category = "Painting Mode|State")
	bool bIsPainting;

	UPROPERTY(BlueprintReadOnly, Category = "Painting Mode|State")
	bool bIsOrbiting;

	UPROPERTY(BlueprintReadOnly, Category = "Painting Mode|State")
	bool bIsPanningCamera;

	UPROPERTY(BlueprintReadOnly, Category = "Painting Mode|State")
	bool bIsAdjustingBrushSize;

	UPROPERTY(BlueprintReadOnly, Category = "Painting Mode|State")
	bool bIsPaintingModeActive;

	UPROPERTY(BlueprintAssignable, Category = "Painting Mode|Events")
	FPaintingModeSimpleEvent OnPaintingModeEntered;

	UPROPERTY(BlueprintAssignable, Category = "Painting Mode|Events")
	FPaintingModeSimpleEvent OnPaintingModeExited;

	UPROPERTY(BlueprintAssignable, Category = "Painting Mode|Events")
	FPaintingModeSimpleEvent OnPaintPressed;

	UPROPERTY(BlueprintAssignable, Category = "Painting Mode|Events")
	FPaintingModeSimpleEvent OnPaintTriggered;

	UPROPERTY(BlueprintAssignable, Category = "Painting Mode|Events")
	FPaintingModeSimpleEvent OnPaintReleased;

	UPROPERTY(BlueprintAssignable, Category = "Painting Mode|Events")
	FPaintingModeBrushSizeChangedEvent OnBrushSizeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Painting Mode|Events")
	FPaintingModeSimpleEvent OnOrbitStarted;

	UPROPERTY(BlueprintAssignable, Category = "Painting Mode|Events")
	FPaintingModeSimpleEvent OnOrbitStopped;

	UFUNCTION(BlueprintCallable, Category = "Painting Mode")
	bool EnterPaintingMode();

	UFUNCTION(BlueprintCallable, Category = "Painting Mode")
	bool ExitPaintingMode();

	UFUNCTION(BlueprintCallable, Category = "Painting Mode|Brush")
	void SetBrushSize(float NewBrushSize);

	UFUNCTION(BlueprintPure, BlueprintGetter, Category = "Painting Mode|Brush Area Preview")
	bool IsBrushAreaPreviewEnabled() const { return bEnableBrushAreaPreview; }

	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Painting Mode|Brush Area Preview")
	void SetBrushAreaPreviewEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Painting Mode|Paint Target")
	bool SetPaintTargetComponent(URuntimeMeshPaintTargetComponent* NewPaintTargetComponent);

	UFUNCTION(BlueprintCallable, Category = "Painting Mode|Paint Target")
	void SetPaintTargetComponents(const TArray<URuntimeMeshPaintTargetComponent*>& NewPaintTargetComponents);

	UFUNCTION(BlueprintCallable, Category = "Painting Mode|Paint Target")
	bool AddPaintTargetComponent(URuntimeMeshPaintTargetComponent* NewPaintTargetComponent);

	UFUNCTION(BlueprintCallable, Category = "Painting Mode|Paint Target")
	void ClearPaintTargetComponents();

	UFUNCTION(BlueprintPure, Category = "Painting Mode|Paint Target")
	URuntimeMeshPaintTargetComponent* GetPaintTargetComponent() const;

	UFUNCTION(BlueprintPure, Category = "Painting Mode|Paint Target")
	TArray<URuntimeMeshPaintTargetComponent*> GetPaintTargetComponents() const;

	UFUNCTION(BlueprintPure, Category = "Painting Mode|Brush")
	FMeshPaintBrushMaterialSettings GetBrushMaterialSettings() const;

	UFUNCTION(BlueprintCallable, Category = "Painting Mode")
	virtual void ApplyBrushMaterialSettings_Implementation(const FMeshPaintBrushMaterialSettings& Settings) override;

	UFUNCTION(BlueprintCallable, Category = "Painting Mode")
	virtual bool SamplePaintedSurfaceColor_Implementation(const FHitResult& HitResult, FRuntimeMeshPaintSampleResult& OutSampleResult) override;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Painting Mode|Paint")
	void ApplyPaint();
	virtual void ApplyPaint_Implementation();

	EMouseCursor::Type GetPaintingMouseCursor();
	void RefreshPaintingMouseCursor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool CacheRequiredReferences();
	bool UsesCharacterControls() const;
	bool UsesCameraControls() const;
	bool UsesFullCameraControls() const;
	bool UsesFirstPersonCameraControls() const;
	bool UsesPaintingInputControls() const;
	bool CanApplyOrbitCamera() const;
	bool HasExplicitCameraName() const;
	bool ShouldUseSpringArmCameraControls() const;
	void ReportCharacterControlReferenceError(const FString& Message) const;
	void LoadDefaultInputAssets();
	UInputMappingContext* GetActivePaintingInputMappingContext();
	UInputMappingContext* GetOrCreateSimplePaintingInputMappingContext();
	APlayerController* ResolveLocalPlayerController() const;
	UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem() const;
	bool BindInput();
	void UnbindInput();
	void AddPaintingMappingContext();
	void RemovePaintingMappingContext();
	void AddToggleMappingContext();
	void RemoveToggleMappingContext();
	void SavePrePaintingState();
	void RestorePrePaintingState();
	void StopPainting();
	void StopOrbiting();
	void StopCameraPan();
	void StopBrushSizeAdjustment();
	void CancelEraserMode();
	void MoveWithCamera(const FVector2D& MoveValue);
	void BeginOrbitCamera();
	void OrbitCamera(const FVector2D& MouseDelta);
	void BeginCameraPan();
	void PanCamera(const FVector2D& MouseDelta);
	void AdjustBrushSize(const FVector2D& MouseDelta);
	void AdjustBrushSizeFromWheel(float WheelValue);
	void ZoomCamera(float WheelValue);
	void PickColorUnderCursor();
	bool IsControlKeyDown() const;
	bool IsShiftKeyDown() const;
	void ApplySpringArmOrbitRotation();
	void BeginCameraRestore();
	void ApplyCameraRestoreImmediate();
	void UpdateCameraRestore(float DeltaTime);
	bool ResolveAutoRegisteredPaintTarget();
	UMaterialInterface* ResolvePaintBrushMaterial();
	AController* ResolveOwnerController() const;
	bool ShouldSubmitPaintCommandToServer(const URuntimeMeshPaintTargetComponent* PaintTarget) const;
	void SubmitPaintCommandBatch(const FRuntimeMeshPaintNetCommandBatch& CommandBatch, bool bReliable);
	void FlushCurrentStrokeReliableCommands();
	void NotifyColorPickerPaintApplied();
	bool TryCreateColorPickerWidget();
	void RemoveColorPickerWidget();
	void UpdatePaintingMouseCursor();
	void UpdateBrushAreaPreview(float DeltaTime);
	void InvalidateBrushAreaPreviewCache();
	void ResetBrushAreaPreviewState();
	void ClearLastBrushAreaPreviewMask();
	bool HoldBrushAreaPreviewOnTransientMiss(float DeltaTime);
	bool ShouldTickPaintingMode() const;
	bool RestorePaintingViewportFocus(APlayerController* PlayerController) const;
	bool GetViewportMousePosition(APlayerController* PlayerController, FVector2D& OutMousePosition) const;
	bool IsCursorOverPaintingUI() const;
	EMouseCursor::Type GetBrushMouseCursor();
	bool BuildBrushScreenProjectionData(
		APlayerController* PlayerController, const FVector2D& MousePosition,
		FRuntimeMeshPaintScreenProjectionData& OutProjectionData) const;
	bool BuildBrushMouseProjectionData(
		APlayerController* PlayerController,
		FRuntimeMeshPaintScreenProjectionData& OutProjectionData) const;
	bool BuildPaintTraceFromScreenPosition(
		APlayerController* PlayerController, const FVector2D& MousePosition,
		FVector& OutTraceStart, FVector& OutTraceEnd) const;
	bool TracePaintUnderScreenPosition(
		APlayerController* PlayerController, const FVector2D& MousePosition,
		FHitResult& OutHitResult, FVector& OutTraceStart, FVector& OutTraceEnd) const;
	bool TracePaintUnderCursor(APlayerController* PlayerController, FHitResult& OutHitResult, FVector& OutTraceStart, FVector& OutTraceEnd) const;
	bool HasPaintTargetComponentFilter() const;
	URuntimeMeshPaintTargetComponent* GetFirstAllowedPaintTargetComponent() const;
	URuntimeMeshPaintTargetComponent* GetPrimaryPaintTargetComponent() const;
	URuntimeMeshPaintTargetComponent* ResolvePaintTargetFromHit(const FHitResult& HitResult) const;
	uint32 GetBrushAreaPreviewTargetRevision(const UPrimitiveComponent* Component) const;
	bool CanReuseBrushAreaPreviewCache(
		APlayerController* PlayerController, const FVector2D& MousePosition,
		const FVector& TraceStart, const FVector& TraceEnd, const FHitResult& TraceHit,
		const FLinearColor& PreviewColor, float PreviewLineThickness) const;
	void StoreBrushAreaPreviewCache(
		APlayerController* PlayerController, const FVector2D& MousePosition,
		const FVector& TraceStart, const FVector& TraceEnd, const FHitResult& TraceHit,
		URuntimeMeshPaintTargetComponent* PaintTarget, const FRuntimeMeshPaintSampleResult& PaintHit,
		const FLinearColor& PreviewColor, float PreviewLineThickness);

	UFUNCTION(Server, Unreliable)
	void ServerSubmitPaintCommandBatch(const FRuntimeMeshPaintNetCommandBatch& CommandBatch);

	UFUNCTION(Server, Reliable)
	void ServerSubmitPaintCommandBatchReliable(const FRuntimeMeshPaintNetCommandBatch& CommandBatch);

	UFUNCTION()
	void HandleOwnerControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void HandleMoveInput(const FInputActionValue& Value);
	void HandlePaintStarted(const FInputActionValue& Value);
	void HandlePaintTriggered(const FInputActionValue& Value);
	void HandlePaintCompleted(const FInputActionValue& Value);
	void HandleOrbitStarted(const FInputActionValue& Value);
	void HandleOrbitCompleted(const FInputActionValue& Value);
	void HandlePanCameraStarted(const FInputActionValue& Value);
	void HandlePanCameraCompleted(const FInputActionValue& Value);
	void HandleBrushSizeStarted(const FInputActionValue& Value);
	void HandleBrushSizeCompleted(const FInputActionValue& Value);
	void HandleMouseDelta(const FInputActionValue& Value);
	void HandlePickColor(const FInputActionValue& Value);
	void HandleCameraZoom(const FInputActionValue& Value);
	void HandleBrushSizeWheel(const FInputActionValue& Value);
	void HandleTogglePaintingMode(const FInputActionValue& Value);

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> BoundPlayerController;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> PaintingInputComponent;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> SimplePaintingInputMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> AddedPaintingInputMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UColorPickerPanelWidget> ColorPickerWidget;

	UPROPERTY()
	TObjectPtr<UTexture2D> DefaultBrushCursorTexture;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> DefaultPaintBrushMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> CachedPaintBrushMaterial;

	float PreviousMaxWalkSpeed;
	bool bPreviousShowMouseCursor;
	EMouseCursor::Type PreviousMouseCursor;
	bool bPreviousUseControllerRotationYaw;
	bool bPreviousUseControllerRotationPitch;
	bool bPreviousUseControllerRotationRoll;
	bool bPreviousOrientRotationToMovement;
	bool bPreviousUseControllerDesiredRotation;
	bool bPreviousSpringArmUsePawnControlRotation;
	bool bPreviousCameraUsePawnControlRotation;
	FRotator PreviousSpringArmRelativeRotation;
	float PreviousSpringArmTargetArmLength;
	FVector PreviousSpringArmSocketOffset;
	FVector PreviousCameraRelativeLocation;
	FRotator PreviousCameraRelativeRotation;
	bool bAppliedLookInputIgnore;
	bool bMappingContextAdded;
	bool bToggleMappingContextAdded;
	bool bSimplePaintingInputMappingContextIncludesCameraControls;
	bool bSimplePaintingInputMappingContextIncludesFullCameraControls;
	bool bSpringArmStateChangedForOrbit;
	bool bCameraRotationChangedForOrbit;
	bool bSpringArmTargetArmLengthChangedForZoom;
	bool bSpringArmSocketOffsetChangedForPan;
	bool bCameraRelativeLocationChangedForPan;
	bool bCameraRestoreActive;
	bool bOrbitUsesControllerRotation;
	FRotator PaintingModeActorRotation;
	float OrbitYaw;
	float OrbitPitch;
	uint16 CurrentPaintStrokeId;
	uint32 NextClientPaintPredictionKey;
	bool bNextPaintCommandReliable;
	bool bColorPickerNotifiedForCurrentStroke;
	TArray<FRuntimeMeshPaintNetCommand> CurrentStrokeReliableCommands;

	mutable TWeakObjectPtr<AActor> CachedHitPaintTargetActor;
	mutable TWeakObjectPtr<URuntimeMeshPaintTargetComponent> CachedHitPaintTarget;
	float BrushAreaPreviewMissTime;
	TWeakObjectPtr<UObject> LastBrushAreaPreviewSource;
	bool bBrushAreaPreviewCacheValid;
	FVector2D CachedBrushAreaPreviewMousePosition;
	FVector CachedBrushAreaPreviewTraceStart;
	FVector CachedBrushAreaPreviewTraceEnd;
	TWeakObjectPtr<APlayerController> CachedBrushAreaPreviewPlayerController;
	TWeakObjectPtr<URuntimeMeshPaintTargetComponent> CachedBrushAreaPreviewPaintTarget;
	TWeakObjectPtr<UPrimitiveComponent> CachedBrushAreaPreviewHitComponent;
	FTransform CachedBrushAreaPreviewTargetTransform;
	uint32 CachedBrushAreaPreviewTargetRevision;
	float CachedBrushAreaPreviewBrushSize;
	float CachedBrushAreaPreviewBrushSizeScale;
	FLinearColor CachedBrushAreaPreviewColor;
	float CachedBrushAreaPreviewLineThickness;
	TEnumAsByte<ECollisionChannel> CachedBrushAreaPreviewTraceChannel;
	bool bCachedBrushAreaPreviewTraceComplex;
	FHitResult CachedBrushAreaPreviewTraceHit;
	FRuntimeMeshPaintSampleResult CachedBrushAreaPreviewHit;
};
