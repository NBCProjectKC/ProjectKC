// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Painting/PaintingModeControllerComponent.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/EngineTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Materials/MaterialInterface.h"
#include "ImageCore.h"
#include "UnrealClient.h"
#include "Hit/RuntimeMeshPaintHitUtils.h"
#include "Painting/RuntimeMeshPaintTargetComponent.h"
#include "../Core/MeshPaintingCoreStats.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/ColorPickerEyedropperUtils.h"
#include "Widgets/ColorPickerPanelWidget.h"
#include "Widgets/SViewport.h"

static TWeakObjectPtr<UTexture2D> GMeshPaintingCoreBrushCursorTexture;
DEFINE_LOG_CATEGORY_STATIC(LogMeshPaintingModeController, Log, All);

static FVector2D GMeshPaintingCoreBrushCursorHotSpot(-1.0f, -1.0f);
static FColor GMeshPaintingCoreBrushCursorAccentColor = FColor::Transparent;
static bool GMeshPaintingCoreBrushCursorInitialized = false;
static bool GMeshPaintingCoreBrushCursorAvailable = false;
static constexpr float GMeshPaintingCoreBrushTraceDistance = 100000.0f;
static constexpr float GMeshPaintingCoreBrushPreviewMissGraceSeconds = 0.12f;
static constexpr float GMeshPaintingCoreBrushPreviewLineThicknessScale = 0.35f;
static constexpr int32 GMeshPaintingCoreReliableStrokeBatchSize = 64;

static bool CopyBrushCursorCPUCopyPixels(UTexture2D* Texture, TArray<FColor>& OutPixels, int32& OutWidth, int32& OutHeight)
{
	if (!Texture) return false;

	FSharedImageConstRef CPUCopy = Texture->GetCPUCopy();
	if (!CPUCopy || CPUCopy->SizeX <= 0 || CPUCopy->SizeY <= 0 || CPUCopy->NumSlices != 1) return false;

	const int64 PixelCount = static_cast<int64>(CPUCopy->SizeX) * CPUCopy->SizeY;
	if (PixelCount <= 0 || PixelCount > MAX_int32) return false;

	FImage CursorImage;
	CPUCopy->CopyTo(CursorImage, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
	if (CursorImage.SizeX != CPUCopy->SizeX || CursorImage.SizeY != CPUCopy->SizeY || CursorImage.NumSlices != 1)
	{
		return false;
	}

	const TArrayView64<const FColor> SourcePixels = CursorImage.AsBGRA8();
	if (SourcePixels.Num() != PixelCount) return false;

	OutWidth = CursorImage.SizeX;
	OutHeight = CursorImage.SizeY;
	OutPixels.SetNumUninitialized(static_cast<int32>(PixelCount));
	for (int32 Index = 0; Index < OutPixels.Num(); ++Index)
	{
		OutPixels[Index] = SourcePixels[Index];
	}

	return true;
}

#if WITH_EDITOR
static bool CopyBrushCursorSourcePixels(UTexture2D* Texture, TArray<FColor>& OutPixels, int32& OutWidth, int32& OutHeight)
{
	if (!Texture || !Texture->Source.IsValid()) return false;

	const ETextureSourceFormat Format = Texture->Source.GetFormat();
	if (Format != TSF_BGRA8 && Format != TSF_BGRE8 && Format != TSF_G8) return false;

	OutWidth = Texture->Source.GetSizeX();
	OutHeight = Texture->Source.GetSizeY();
	if (OutWidth <= 0 || OutHeight <= 0) return false;

	const uint8* MipData = Texture->Source.LockMipReadOnly(0);
	if (!MipData) return false;

	OutPixels.SetNumUninitialized(OutWidth * OutHeight);
	if (Format == TSF_G8)
	{
		for (int32 Index = 0; Index < OutPixels.Num(); ++Index)
			OutPixels[Index] = FColor(MipData[Index], MipData[Index], MipData[Index], 255);
	}
	else
	{
		for (int32 Index = 0; Index < OutPixels.Num(); ++Index)
		{
			const int32 Offset = Index * 4;
			OutPixels[Index] = FColor(MipData[Offset + 2], MipData[Offset + 1], MipData[Offset], MipData[Offset + 3]);
		}
	}

	Texture->Source.UnlockMip(0);
	return true;
}
#endif

static FColor MakeBrushCursorAccentColor(const FLinearColor& BrushColor)
{
	FLinearColor ClampedColor = BrushColor.GetClamped();
	ClampedColor.A = 1.0f;
	return ClampedColor.ToFColorSRGB();
}

static bool IsBrushCursorAccentPixel(const FColor& Pixel)
{
	if (Pixel.A < 8) return false;

	const FLinearColor LinearPixel = FLinearColor::FromSRGBColor(Pixel);
	const FLinearColor HsvPixel = LinearPixel.LinearRGBToHSV();
	const float Saturation = HsvPixel.G;
	const float Value = HsvPixel.B;

	const float MaxChannel = FMath::Max3(LinearPixel.R, LinearPixel.G, LinearPixel.B);
	const float MinChannel = FMath::Min3(LinearPixel.R, LinearPixel.G, LinearPixel.B);
	return Saturation >= 0.18f && Value >= 0.08f && (MaxChannel - MinChannel) >= 0.05f;
}

static void TintBrushCursorAccentPixels(TArray<FColor>& Pixels, const FColor& AccentColor)
{
	const FLinearColor AccentLinear = FLinearColor::FromSRGBColor(AccentColor);
	FLinearColor AccentHsv = AccentLinear.LinearRGBToHSV();
	AccentHsv.A = 1.0f;

	for (FColor& Pixel : Pixels)
	{
		if (!IsBrushCursorAccentPixel(Pixel)) continue;

		const FLinearColor SourceLinear = FLinearColor::FromSRGBColor(Pixel);
		const FLinearColor SourceHsv = SourceLinear.LinearRGBToHSV();
		FLinearColor TintedHsv(AccentHsv.R, AccentHsv.G, SourceHsv.B * AccentHsv.B, SourceLinear.A);
		FLinearColor TintedLinear = TintedHsv.HSVToLinearRGB();
		TintedLinear.A = SourceLinear.A;
		Pixel = TintedLinear.ToFColorSRGB();
	}
}

static void BuildBrushCursorRawRGBABuffer(const TArray<FColor>& Pixels, TArray<FColor>& OutRawRGBAPixels)
{
	OutRawRGBAPixels.SetNumUninitialized(Pixels.Num());
	for (int32 PixelIndex = 0; PixelIndex < Pixels.Num(); ++PixelIndex)
	{
		const FColor& Pixel = Pixels[PixelIndex];
		OutRawRGBAPixels[PixelIndex] = FColor(Pixel.B, Pixel.G, Pixel.R, Pixel.A);
	}
}

static bool InitializeMeshPaintingCoreBrushCursor(
	UTexture2D* Texture, const FVector2D& HotSpot, const FLinearColor& BrushColor)
{
	const FVector2D ClampedHotSpot(
		FMath::Clamp(HotSpot.X, 0.0f, 1.0f),
		FMath::Clamp(HotSpot.Y, 0.0f, 1.0f));
	const FColor AccentColor = MakeBrushCursorAccentColor(BrushColor);

	if (GMeshPaintingCoreBrushCursorInitialized &&
		GMeshPaintingCoreBrushCursorTexture.Get() == Texture &&
		GMeshPaintingCoreBrushCursorHotSpot.Equals(ClampedHotSpot) &&
		GMeshPaintingCoreBrushCursorAccentColor == AccentColor)
	{
		return GMeshPaintingCoreBrushCursorAvailable;
	}

	GMeshPaintingCoreBrushCursorTexture = Texture;
	GMeshPaintingCoreBrushCursorHotSpot = ClampedHotSpot;
	GMeshPaintingCoreBrushCursorAccentColor = AccentColor;
	GMeshPaintingCoreBrushCursorAvailable = false;
	GMeshPaintingCoreBrushCursorInitialized = true;

	if (!Texture) return false;
	if (!FSlateApplication::IsInitialized()) return false;

	TSharedPtr<ICursor> PlatformCursor = FSlateApplication::Get().GetPlatformCursor();
	if (!PlatformCursor.IsValid() || !PlatformCursor->IsCreateCursorFromRGBABufferSupported()) return false;

	TArray<FColor> Pixels;
	int32 Width = 0;
	int32 Height = 0;
	if (!CopyBrushCursorCPUCopyPixels(Texture, Pixels, Width, Height))
	{
#if WITH_EDITOR
		if (!CopyBrushCursorSourcePixels(Texture, Pixels, Width, Height)) return false;
#else
		return false;
#endif
	}

	TintBrushCursorAccentPixels(Pixels, AccentColor);

	TArray<FColor> CursorRGBAPixels;
	BuildBrushCursorRawRGBABuffer(Pixels, CursorRGBAPixels);

	void* CursorHandle = PlatformCursor->CreateCursorFromRGBABuffer(CursorRGBAPixels.GetData(), Width, Height, ClampedHotSpot);
	if (!CursorHandle) return false;

	PlatformCursor->SetTypeShape(EMouseCursor::Custom, CursorHandle);
	GMeshPaintingCoreBrushCursorAvailable = true;
	return true;
}

static bool BuildMeshPaintingCoreBrushProjectionDataFromRay(
	const FRotator& ViewRotation,
	const FVector& CursorRayOrigin,
	const FVector& CursorRayDirection,
	FRuntimeMeshPaintScreenProjectionData& OutProjectionData)
{
	const FRotationMatrix ViewRotationMatrix(ViewRotation);
	FVector SafeCursorRayDirection = CursorRayDirection.GetSafeNormal(SMALL_NUMBER, ViewRotationMatrix.GetScaledAxis(EAxis::X));
	const FVector CameraRight = ViewRotationMatrix.GetScaledAxis(EAxis::Y);
	const FVector CameraUp = ViewRotationMatrix.GetScaledAxis(EAxis::Z);
	FVector BrushRight = CameraRight - SafeCursorRayDirection * FVector::DotProduct(CameraRight, SafeCursorRayDirection);
	BrushRight = BrushRight.GetSafeNormal();
	if (BrushRight.IsNearlyZero())
	{
		FVector FallbackUp = FVector::UpVector;
		SafeCursorRayDirection.FindBestAxisVectors(BrushRight, FallbackUp);
	}

	FVector BrushUp = FVector::CrossProduct(SafeCursorRayDirection, BrushRight).GetSafeNormal(SMALL_NUMBER, CameraUp);
	if (FVector::DotProduct(BrushUp, CameraUp) < 0.0)
	{
		BrushUp *= -1.0;
		BrushRight *= -1.0;
	}

	OutProjectionData = FRuntimeMeshPaintScreenProjectionData();
	OutProjectionData.bHasScreenProjection = true;
	OutProjectionData.ViewOrigin = CursorRayOrigin;
	OutProjectionData.ViewForward = SafeCursorRayDirection;
	OutProjectionData.ViewRight = BrushRight;
	OutProjectionData.ViewUp = BrushUp;
	return true;
}

UPaintingModeControllerComponent::UPaintingModeControllerComponent()
	: ControlMode(EPaintingModeControllerControlMode::CharacterLock)
	, PaintingInputPriority(100)
	, bLoadDefaultInputAssets(true)
	, PaintingToggleInputMappingContext(nullptr)
	, bAutoCreateColorPickerWidget(true)
	, ColorPickerWidgetClass(UColorPickerPanelWidget::StaticClass())
	, ColorPickerWidgetZOrder(0)
	, ColorPickerSampleMode(ERuntimeMeshPaintColorSampleMode::MeshUnlitColor)
	, bUseBrushCursorOutsideUI(true)
	, BrushCursorTexture(FSoftObjectPath(TEXT("/MeshPaintingCore/Textures/T_Brush.T_Brush")))
	, BrushCursorHotSpot(0.15f, 0.85f)
	, bEnableBrushAreaPreview(true)
	, BrushAreaPreviewLineThickness(1.5f)
	, BrushAreaPreviewThicknessBrushSizeMultiplier(12.0f)
	, BrushAreaPreviewColor(1.f, 1.f, 1.0f, 1.0f)
	, BrushAreaPreviewEmissiveIntensity(6.0f)
	, BrushAreaPreviewOpacity(0.95f)
	, BrushAreaPreviewBrushSizeScale(1.0f)
	, bAutoRegister(true)
	, PaintTargetComponent(nullptr)
	, PaintBrushMaterial(FSoftObjectPath(TEXT("/MeshPaintingCore/Materials/M_RuntimeMeshPaintBrush.M_RuntimeMeshPaintBrush")))
	, PaintTraceChannel(ECC_Visibility)
	, bPaintTraceComplex(true)
	, PaintingMovementSpeed(250.0f)
	, OrbitYawSensitivity(0.25f)
	, OrbitPitchSensitivity(0.25f)
	, MinimumOrbitPitch(-65.0f)
	, MaximumOrbitPitch(35.0f)
	, CameraZoomSensitivity(50.0f)
	, MinimumCameraZoomDistance(100.0f)
	, MaximumCameraZoomDistance(800.0f)
	, CameraPanSensitivity(1.0f)
	, CameraPanMaxOffset(100.0f)
	, CameraRestoreSmoothingSpeed(10.0f)
	, CameraName(NAME_None)
	, BrushSize(0.04f)
	, BrushColor(FLinearColor::White)
	, MinBrushSize(0.005f)
	, MaxBrushSize(0.25f)
	, BrushSizeSensitivity(0.001f)
	, BrushSizeWheelSensitivity(0.01f)
	, BrushMetallic(0.0f)
	, BrushRoughness(0.5f)
	, bBrushErase(false)
	, bIsPainting(false)
	, bIsOrbiting(false)
	, bIsPanningCamera(false)
	, bIsAdjustingBrushSize(false)
	, bIsPaintingModeActive(false)
	, SimplePaintingInputMappingContext(nullptr)
	, AddedPaintingInputMappingContext(nullptr)
	, ColorPickerWidget(nullptr)
	, DefaultBrushCursorTexture(nullptr)
	, DefaultPaintBrushMaterial(nullptr)
	, CachedPaintBrushMaterial(nullptr)
	, PreviousMaxWalkSpeed(0.0f)
	, bPreviousShowMouseCursor(false)
	, PreviousMouseCursor(EMouseCursor::Default)
	, bPreviousUseControllerRotationYaw(false)
	, bPreviousUseControllerRotationPitch(false)
	, bPreviousUseControllerRotationRoll(false)
	, bPreviousOrientRotationToMovement(false)
	, bPreviousUseControllerDesiredRotation(false)
	, bPreviousSpringArmUsePawnControlRotation(false)
	, bPreviousCameraUsePawnControlRotation(false)
	, PreviousSpringArmRelativeRotation(FRotator::ZeroRotator)
	, PreviousSpringArmTargetArmLength(0.0f)
	, PreviousSpringArmSocketOffset(FVector::ZeroVector)
	, PreviousCameraRelativeLocation(FVector::ZeroVector)
	, PreviousCameraRelativeRotation(FRotator::ZeroRotator)
	, bAppliedLookInputIgnore(false)
	, bMappingContextAdded(false)
	, bToggleMappingContextAdded(false)
	, bSimplePaintingInputMappingContextIncludesCameraControls(false)
	, bSimplePaintingInputMappingContextIncludesFullCameraControls(false)
	, bSpringArmStateChangedForOrbit(false)
	, bCameraRotationChangedForOrbit(false)
	, bSpringArmTargetArmLengthChangedForZoom(false)
	, bSpringArmSocketOffsetChangedForPan(false)
	, bCameraRelativeLocationChangedForPan(false)
	, bCameraRestoreActive(false)
	, bOrbitUsesControllerRotation(false)
	, PaintingModeActorRotation(FRotator::ZeroRotator)
	, OrbitYaw(0.0f)
	, OrbitPitch(0.0f)
	, CurrentPaintStrokeId(0)
	, NextClientPaintPredictionKey(0)
	, bNextPaintCommandReliable(false)
	, bColorPickerNotifiedForCurrentStroke(false)
	, BrushAreaPreviewMissTime(0.0f)
	, bBrushAreaPreviewCacheValid(false)
	, CachedBrushAreaPreviewMousePosition(FVector2D::ZeroVector)
	, CachedBrushAreaPreviewTraceStart(FVector::ZeroVector)
	, CachedBrushAreaPreviewTraceEnd(FVector::ZeroVector)
	, CachedBrushAreaPreviewTargetTransform(FTransform::Identity)
	, CachedBrushAreaPreviewTargetRevision(0)
	, CachedBrushAreaPreviewBrushSize(0.0f)
	, CachedBrushAreaPreviewBrushSizeScale(0.0f)
	, CachedBrushAreaPreviewTraceChannel(ECC_Visibility)
	, bCachedBrushAreaPreviewTraceComplex(false)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);

	static ConstructorHelpers::FClassFinder<UColorPickerPanelWidget> DefaultColorPickerWidgetClass(
		TEXT("/MeshPaintingCore/Widgets/WBP_RuntimeColorPicker"));
	if (DefaultColorPickerWidgetClass.Succeeded()) ColorPickerWidgetClass = DefaultColorPickerWidgetClass.Class;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultPaintingInputContext(
		TEXT("/MeshPaintingCore/Input/IMC_PaintingMode.IMC_PaintingMode"));
	if (DefaultPaintingInputContext.Succeeded()) PaintingInputMappingContext = DefaultPaintingInputContext.Object;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultToggleInputContext(
		TEXT("/MeshPaintingCore/Input/IMC_PaintingModeToggle.IMC_PaintingModeToggle"));
	if (DefaultToggleInputContext.Succeeded()) PaintingToggleInputMappingContext = DefaultToggleInputContext.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultMoveAction(
		TEXT("/MeshPaintingCore/Input/IA_PaintingMove.IA_PaintingMove"));
	if (DefaultMoveAction.Succeeded()) PaintingMoveAction = DefaultMoveAction.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultPaintAction(
		TEXT("/MeshPaintingCore/Input/IA_Paint.IA_Paint"));
	if (DefaultPaintAction.Succeeded()) PaintAction = DefaultPaintAction.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultOrbitAction(
		TEXT("/MeshPaintingCore/Input/IA_OrbitCamera.IA_OrbitCamera"));
	if (DefaultOrbitAction.Succeeded()) OrbitCameraAction = DefaultOrbitAction.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultPanCameraAction(
		TEXT("/MeshPaintingCore/Input/IA_PanCamera.IA_PanCamera"));
	if (DefaultPanCameraAction.Succeeded()) PanCameraAction = DefaultPanCameraAction.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultAdjustBrushSizeAction(
		TEXT("/MeshPaintingCore/Input/IA_AdjustBrushSize.IA_AdjustBrushSize"));
	if (DefaultAdjustBrushSizeAction.Succeeded()) AdjustBrushSizeAction = DefaultAdjustBrushSizeAction.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultMouseDeltaAction(
		TEXT("/MeshPaintingCore/Input/IA_MouseDelta.IA_MouseDelta"));
	if (DefaultMouseDeltaAction.Succeeded()) MouseDeltaAction = DefaultMouseDeltaAction.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultPickColorAction(
		TEXT("/MeshPaintingCore/Input/IA_PickColorAction.IA_PickColorAction"));
	if (DefaultPickColorAction.Succeeded()) PickColorAction = DefaultPickColorAction.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultCameraZoomAction(
		TEXT("/MeshPaintingCore/Input/IA_CameraZoom.IA_CameraZoom"));
	if (DefaultCameraZoomAction.Succeeded()) CameraZoomAction = DefaultCameraZoomAction.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultAdjustBrushSizeWheelAction(
		TEXT("/MeshPaintingCore/Input/IA_AdjustBrushSizeWheel.IA_AdjustBrushSizeWheel"));
	if (DefaultAdjustBrushSizeWheelAction.Succeeded()) AdjustBrushSizeWheelAction = DefaultAdjustBrushSizeWheelAction.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultToggleAction(
		TEXT("/MeshPaintingCore/Input/IA_TogglePaintingMode.IA_TogglePaintingMode"));
	if (DefaultToggleAction.Succeeded()) TogglePaintingModeAction = DefaultToggleAction.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultBrushCursorTextureAsset(
		TEXT("/MeshPaintingCore/Textures/T_Brush.T_Brush"));
	if (DefaultBrushCursorTextureAsset.Succeeded()) DefaultBrushCursorTexture = DefaultBrushCursorTextureAsset.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultPaintBrushMaterialAsset(
		TEXT("/MeshPaintingCore/Materials/M_RuntimeMeshPaintBrush.M_RuntimeMeshPaintBrush"));
	if (DefaultPaintBrushMaterialAsset.Succeeded()) DefaultPaintBrushMaterial = DefaultPaintBrushMaterialAsset.Object;
}

void UPaintingModeControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!UsesPaintingInputControls()) return;
	if (!CacheRequiredReferences()) return;

	LoadDefaultInputAssets();
	ResolveAutoRegisteredPaintTarget();
	if (OwnerCharacter)
	{
		OwnerCharacter->ReceiveControllerChangedDelegate.AddUniqueDynamic(
			this, &UPaintingModeControllerComponent::HandleOwnerControllerChanged);
	}
	BindInput();
}

void UPaintingModeControllerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ExitPaintingMode();
	RemoveColorPickerWidget();
	UnbindInput();

	if (OwnerCharacter)
	{
		OwnerCharacter->ReceiveControllerChangedDelegate.RemoveDynamic(
			this, &UPaintingModeControllerComponent::HandleOwnerControllerChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void UPaintingModeControllerComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bCameraRestoreActive) UpdateCameraRestore(DeltaTime);
	if (!bIsPaintingModeActive) return;

	UpdatePaintingMouseCursor();
	UpdateBrushAreaPreview(DeltaTime);
}

bool UPaintingModeControllerComponent::EnterPaintingMode()
{
	if (bIsPaintingModeActive) return true;
	if (!UsesPaintingInputControls()) return false;
	if (!CacheRequiredReferences()) return false;
	bCameraRestoreActive = false;

	APlayerController* PlayerController = ResolveLocalPlayerController();
	if (!PlayerController) return false;

	LoadDefaultInputAssets();
	ResolveAutoRegisteredPaintTarget();
	if (!PaintingInputComponent && !BindInput()) return false;

	BoundPlayerController = PlayerController;
	SavePrePaintingState();
	bSpringArmStateChangedForOrbit = false;
	bCameraRotationChangedForOrbit = false;
	bSpringArmTargetArmLengthChangedForZoom = false;
	bSpringArmSocketOffsetChangedForPan = false;
	bCameraRelativeLocationChangedForPan = false;
	if (OwnerCharacter) PaintingModeActorRotation = OwnerCharacter->GetActorRotation();

	bIsPaintingModeActive = true;
	AddPaintingMappingContext();

	if (UsesCharacterControls() && OwnerCharacter && CharacterMovement)
	{
		CharacterMovement->MaxWalkSpeed = PaintingMovementSpeed;
		CharacterMovement->bOrientRotationToMovement = false;
		CharacterMovement->bUseControllerDesiredRotation = false;

		OwnerCharacter->bUseControllerRotationYaw = false;
		OwnerCharacter->bUseControllerRotationPitch = false;
		OwnerCharacter->bUseControllerRotationRoll = false;
		OwnerCharacter->SetActorRotation(PaintingModeActorRotation);

		PlayerController->SetIgnoreLookInput(true);
		bAppliedLookInputIgnore = true;
	}
	else if (UsesFirstPersonCameraControls() ||
		ControlMode == EPaintingModeControllerControlMode::DroneController)
	{
		PlayerController->SetIgnoreLookInput(true);
		bAppliedLookInputIgnore = true;
		StopOrbiting();
		StopCameraPan();
	}
	else
	{
		StopOrbiting();
		StopCameraPan();
	}
	PlayerController->bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);

	TryCreateColorPickerWidget();
	SetComponentTickEnabled(ShouldTickPaintingMode());
	UpdatePaintingMouseCursor();
	ResetBrushAreaPreviewState();
	OnPaintingModeEntered.Broadcast();
	return true;
}

bool UPaintingModeControllerComponent::ExitPaintingMode()
{
	if (!bIsPaintingModeActive) return true;

	StopBrushSizeAdjustment();
	StopOrbiting();
	StopCameraPan();
	StopPainting();
	RemovePaintingMappingContext();
	RemoveColorPickerWidget();
	ResetBrushAreaPreviewState();
	RestorePrePaintingState();

	bIsPaintingModeActive = false;
	BeginCameraRestore();
	SetComponentTickEnabled(ShouldTickPaintingMode());
	OnPaintingModeExited.Broadcast();
	return true;
}

void UPaintingModeControllerComponent::SetBrushSize(float NewBrushSize)
{
	const float SafeMin = FMath::Min(MinBrushSize, MaxBrushSize);
	const float SafeMax = FMath::Max(MinBrushSize, MaxBrushSize);
	const float ClampedBrushSize = FMath::Clamp(NewBrushSize, SafeMin, SafeMax);
	if (FMath::IsNearlyEqual(BrushSize, ClampedBrushSize)) return;

	BrushSize = ClampedBrushSize;
	InvalidateBrushAreaPreviewCache();
	OnBrushSizeChanged.Broadcast(BrushSize);
}

void UPaintingModeControllerComponent::SetBrushAreaPreviewEnabled(bool bEnabled)
{
	if (bEnableBrushAreaPreview == bEnabled) return;

	bEnableBrushAreaPreview = bEnabled;
	InvalidateBrushAreaPreviewCache();
	if (!bEnableBrushAreaPreview) ResetBrushAreaPreviewState();
	if (bIsPaintingModeActive) SetComponentTickEnabled(ShouldTickPaintingMode());
}

bool UPaintingModeControllerComponent::SetPaintTargetComponent(URuntimeMeshPaintTargetComponent* NewPaintTargetComponent)
{
	PaintTargetComponent = NewPaintTargetComponent;
	PaintTargetComponents.Reset();
	if (NewPaintTargetComponent) PaintTargetComponents.Add(NewPaintTargetComponent);
	CachedHitPaintTargetActor = nullptr;
	CachedHitPaintTarget = nullptr;
	ResetBrushAreaPreviewState();
	return IsValid(PaintTargetComponent.Get());
}

void UPaintingModeControllerComponent::SetPaintTargetComponents(
	const TArray<URuntimeMeshPaintTargetComponent*>& NewPaintTargetComponents)
{
	PaintTargetComponents.Reset(NewPaintTargetComponents.Num());
	for (URuntimeMeshPaintTargetComponent* NewPaintTargetComponent : NewPaintTargetComponents)
	{
		PaintTargetComponents.Add(NewPaintTargetComponent);
	}

	PaintTargetComponent = GetFirstAllowedPaintTargetComponent();
	CachedHitPaintTargetActor = nullptr;
	CachedHitPaintTarget = nullptr;
	ResetBrushAreaPreviewState();
}

bool UPaintingModeControllerComponent::AddPaintTargetComponent(URuntimeMeshPaintTargetComponent* NewPaintTargetComponent)
{
	if (!NewPaintTargetComponent) return false;

	for (const TObjectPtr<URuntimeMeshPaintTargetComponent>& ExistingPaintTargetComponent : PaintTargetComponents)
	{
		if (ExistingPaintTargetComponent.Get() == NewPaintTargetComponent)
		{
			PaintTargetComponent = GetFirstAllowedPaintTargetComponent();
			return true;
		}
	}

	PaintTargetComponents.Add(NewPaintTargetComponent);
	PaintTargetComponent = GetFirstAllowedPaintTargetComponent();
	CachedHitPaintTargetActor = nullptr;
	CachedHitPaintTarget = nullptr;
	ResetBrushAreaPreviewState();
	return true;
}

void UPaintingModeControllerComponent::ClearPaintTargetComponents()
{
	PaintTargetComponents.Reset();
	PaintTargetComponent = nullptr;
	CachedHitPaintTargetActor = nullptr;
	CachedHitPaintTarget = nullptr;
	ResetBrushAreaPreviewState();
}

URuntimeMeshPaintTargetComponent* UPaintingModeControllerComponent::GetPaintTargetComponent() const
{
	return GetPrimaryPaintTargetComponent();
}

TArray<URuntimeMeshPaintTargetComponent*> UPaintingModeControllerComponent::GetPaintTargetComponents() const
{
	TArray<URuntimeMeshPaintTargetComponent*> Result;
	Result.Reserve(PaintTargetComponents.Num());
	for (const TObjectPtr<URuntimeMeshPaintTargetComponent>& ConfiguredPaintTargetComponent : PaintTargetComponents)
	{
		Result.Add(ConfiguredPaintTargetComponent.Get());
	}

	return Result;
}

FMeshPaintBrushMaterialSettings UPaintingModeControllerComponent::GetBrushMaterialSettings() const
{
	FMeshPaintBrushMaterialSettings Settings;
	Settings.Color = BrushColor;
	Settings.BrushSize = BrushSize;
	Settings.Metallic = BrushMetallic;
	Settings.Roughness = BrushRoughness;
	Settings.bErase = bBrushErase;
	Settings.Clamp();
	return Settings;
}

void UPaintingModeControllerComponent::ApplyBrushMaterialSettings_Implementation(const FMeshPaintBrushMaterialSettings& Settings)
{
	BrushColor = Settings.Color;
	BrushMetallic = FMath::Clamp(Settings.Metallic, 0.0f, 1.0f);
	BrushRoughness = FMath::Clamp(Settings.Roughness, 0.0f, 1.0f);
	bBrushErase = Settings.bErase;
	SetBrushSize(Settings.BrushSize);
	RefreshPaintingMouseCursor();
}

bool UPaintingModeControllerComponent::SamplePaintedSurfaceColor_Implementation(
	const FHitResult& HitResult, FRuntimeMeshPaintSampleResult& OutSampleResult)
{
	ResolveAutoRegisteredPaintTarget();
	URuntimeMeshPaintTargetComponent* PaintTarget = ResolvePaintTargetFromHit(HitResult);
	if (!PaintTarget) return false;

	return IRuntimeMeshPaintTargetInterface::Execute_SamplePaintedSurfaceColor(PaintTarget, HitResult, OutSampleResult);
}

void UPaintingModeControllerComponent::ApplyPaint_Implementation()
{
	SCOPE_CYCLE_COUNTER(STAT_MeshPaintingCore_ApplyPaint);

	ResolveAutoRegisteredPaintTarget();

	APlayerController* PlayerController = BoundPlayerController ? BoundPlayerController.Get() : ResolveLocalPlayerController();
	UMaterialInterface* BrushMaterial = ResolvePaintBrushMaterial();
	if (!PlayerController) return;
	if (IsCursorOverPaintingUI() || (ColorPickerWidget && ColorPickerWidget->IsEyedropperActive())) return;

	FVector2D MousePosition = FVector2D::ZeroVector;
	FVector TraceStart = FVector::ZeroVector;
	FVector TraceEnd = FVector::ZeroVector;
	const bool bHasCurrentMousePosition = GetViewportMousePosition(PlayerController, MousePosition);
	FHitResult HitResult;
	if (bHasCurrentMousePosition)
	{
		if (!TracePaintUnderScreenPosition(PlayerController, MousePosition, HitResult, TraceStart, TraceEnd)) return;
	}
	else if (!TracePaintUnderCursor(PlayerController, HitResult, TraceStart, TraceEnd))
	{
		return;
	}
	HitResult.TraceStart = TraceStart;
	HitResult.TraceEnd = TraceEnd;

	URuntimeMeshPaintTargetComponent* PaintTarget = ResolvePaintTargetFromHit(HitResult);
	if (!IsValid(PaintTarget)) return;

	const FMeshPaintBrushMaterialSettings BrushSettings = GetBrushMaterialSettings();
	FRuntimeMeshPaintSampleResult PaintResult;
	FRuntimeMeshPaintSampleResult ProjectedPaintHit;
	if (!PaintTarget->ResolveProjectedPaintHit(HitResult, ProjectedPaintHit)) return;
	if (Cast<USkeletalMeshComponent>(ProjectedPaintHit.HitResult.GetComponent()))
	{
		const bool bBuiltProjectionData = bHasCurrentMousePosition
			? BuildBrushScreenProjectionData(PlayerController, MousePosition, ProjectedPaintHit.ProjectionData)
			: BuildBrushMouseProjectionData(PlayerController, ProjectedPaintHit.ProjectionData);
		if (!bBuiltProjectionData) return;
	}

	if (!ShouldSubmitPaintCommandToServer(PaintTarget))
	{
		PaintTarget->PaintProjectedHitWithSettings(ProjectedPaintHit, BrushMaterial, BrushSettings, PaintResult);
		if (PaintResult.bSuccess)
		{
			NotifyColorPickerPaintApplied();
		}
		return;
	}

	uint32 PredictionKey = ++NextClientPaintPredictionKey;
	if (PredictionKey == 0) PredictionKey = ++NextClientPaintPredictionKey;

	FRuntimeMeshPaintNetCommand PaintCommand;
	if (!PaintTarget->BuildReplicatedPaintCommand(
		ProjectedPaintHit,
		BrushSettings,
		CurrentPaintStrokeId,
		PredictionKey,
		GetOwner(),
		PaintCommand))
	{
		return;
	}

	if (!PaintTarget->ApplyPaintCommandLocal(PaintCommand, &PaintResult)) return;
	if (PaintResult.bSuccess)
	{
		NotifyColorPickerPaintApplied();
	}
	CurrentStrokeReliableCommands.Add(PaintCommand);

	if (AActor* OwnerActor = GetOwner())
	{
		if (!OwnerActor->HasAuthority())
		{
			PaintTarget->RememberPredictedPaintCommand(PaintCommand);
		}
	}

	FRuntimeMeshPaintNetCommandBatch CommandBatch;
	CommandBatch.Commands.Add(PaintCommand);
	const bool bSubmitReliable = bNextPaintCommandReliable;
	bNextPaintCommandReliable = false;
	SubmitPaintCommandBatch(CommandBatch, bSubmitReliable);
}

EMouseCursor::Type UPaintingModeControllerComponent::GetPaintingMouseCursor()
{
	if (!bUseBrushCursorOutsideUI || !bIsPaintingModeActive) return EMouseCursor::Default;

	APlayerController* PlayerController = BoundPlayerController ? BoundPlayerController.Get() : ResolveLocalPlayerController();
	FVector2D MousePosition;
	return GetViewportMousePosition(PlayerController, MousePosition) ? GetBrushMouseCursor() : EMouseCursor::Default;
}

void UPaintingModeControllerComponent::RefreshPaintingMouseCursor()
{
	UpdatePaintingMouseCursor();
}

bool UPaintingModeControllerComponent::CacheRequiredReferences()
{
	AActor* OwnerActor = GetOwner();
	OwnerCharacter = Cast<ACharacter>(OwnerActor);
	if (!OwnerCharacter)
	{
		if (APlayerController* OwnerPlayerController = Cast<APlayerController>(OwnerActor))
		{
			OwnerCharacter = Cast<ACharacter>(OwnerPlayerController->GetPawn());
		}
		else if (AController* OwnerController = Cast<AController>(OwnerActor))
		{
			OwnerCharacter = Cast<ACharacter>(OwnerController->GetPawn());
		}
	}

	SpringArm = OwnerActor ? OwnerActor->FindComponentByClass<USpringArmComponent>() : nullptr;
	if (!SpringArm && OwnerCharacter && OwnerCharacter.Get() != OwnerActor)
	{
		SpringArm = OwnerCharacter->FindComponentByClass<USpringArmComponent>();
	}
	Camera = nullptr;

	const bool bRequiresCameraComponent = UsesFirstPersonCameraControls() ||
		ControlMode == EPaintingModeControllerControlMode::DroneController;
	if (bRequiresCameraComponent)
	{
		auto ResolveCameraFromActor = [this](AActor* CameraOwner)
		{
			if (!CameraOwner || Camera)
			{
				return;
			}

			TArray<UCameraComponent*> OwnerCameras;
			CameraOwner->GetComponents(OwnerCameras);
			if (HasExplicitCameraName())
			{
				for (UCameraComponent* OwnerCamera : OwnerCameras)
				{
					if (OwnerCamera && OwnerCamera->GetFName() == CameraName)
					{
						Camera = OwnerCamera;
						return;
					}
				}
				return;
			}

			for (UCameraComponent* OwnerCamera : OwnerCameras)
			{
				if (OwnerCamera && OwnerCamera->IsActive())
				{
					Camera = OwnerCamera;
					return;
				}
			}

			if (OwnerCameras.Num() > 0)
			{
				Camera = OwnerCameras[0];
			}
		};

		ResolveCameraFromActor(OwnerActor);
		if (!Camera && OwnerCharacter && OwnerCharacter.Get() != OwnerActor)
		{
			ResolveCameraFromActor(OwnerCharacter.Get());
		}
	}

	CharacterMovement = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;

	if (UsesCharacterControls() || UsesCameraControls())
	{
		if (!OwnerActor)
		{
			ReportCharacterControlReferenceError(
				TEXT("Painting Mode controller failed: controller component has no owner actor."));
			return false;
		}

		if (UsesCharacterControls() && !OwnerCharacter)
		{
			ReportCharacterControlReferenceError(FString::Printf(
				TEXT("Painting Mode Third Person Controller failed on '%s': owner must be an ACharacter."),
				*OwnerActor->GetName()));
			return false;
		}

		if (UsesCharacterControls() && !CharacterMovement)
		{
			ReportCharacterControlReferenceError(FString::Printf(
				TEXT("Painting Mode Third Person Controller failed on '%s': CharacterMovementComponent was not found."),
				*OwnerActor->GetName()));
			return false;
		}

		if (UsesCharacterControls())
		{
			if (!SpringArm)
			{
				ReportCharacterControlReferenceError(FString::Printf(
					TEXT("Painting Mode Third Person Controller failed on '%s': SpringArmComponent was not found. Third Person Controller requires a SpringArmComponent."),
					*OwnerActor->GetName()));
				return false;
			}
			return true;
		}

		if (UsesFirstPersonCameraControls() ||
			ControlMode == EPaintingModeControllerControlMode::DroneController)
		{
			const TCHAR* ModeName = UsesFirstPersonCameraControls()
				? TEXT("First Person Controller")
				: TEXT("Drone Controller");

			if (HasExplicitCameraName() && !Camera)
			{
				ReportCharacterControlReferenceError(FString::Printf(
					TEXT("Painting Mode %s failed on '%s': CameraName '%s' was not found."),
					ModeName,
					*OwnerActor->GetName(),
					*CameraName.ToString()));
				return false;
			}

			if (!Camera)
			{
				ReportCharacterControlReferenceError(FString::Printf(
					TEXT("Painting Mode %s failed on '%s': CameraComponent was not found."),
					ModeName,
					*OwnerActor->GetName()));
				return false;
			}
			return true;
		}

		return true;
	}

	return true;
}

bool UPaintingModeControllerComponent::UsesCharacterControls() const
{
	return ControlMode == EPaintingModeControllerControlMode::CharacterLock;
}

bool UPaintingModeControllerComponent::UsesCameraControls() const
{
	return ControlMode == EPaintingModeControllerControlMode::CharacterLock ||
		ControlMode == EPaintingModeControllerControlMode::DroneController ||
		ControlMode == EPaintingModeControllerControlMode::FirstPersonController;
}

bool UPaintingModeControllerComponent::UsesFullCameraControls() const
{
	return ControlMode == EPaintingModeControllerControlMode::CharacterLock ||
		ControlMode == EPaintingModeControllerControlMode::DroneController;
}

bool UPaintingModeControllerComponent::UsesFirstPersonCameraControls() const
{
	return ControlMode == EPaintingModeControllerControlMode::FirstPersonController;
}

bool UPaintingModeControllerComponent::UsesPaintingInputControls() const
{
	return ControlMode != EPaintingModeControllerControlMode::None;
}

bool UPaintingModeControllerComponent::CanApplyOrbitCamera() const
{
	if (UsesFirstPersonCameraControls())
	{
		return Camera && (IsValid(BoundPlayerController.Get()) || ResolveLocalPlayerController());
	}

	if (ControlMode == EPaintingModeControllerControlMode::DroneController)
	{
		return Camera && GetOwner();
	}

	if (UsesCharacterControls())
	{
		return ShouldUseSpringArmCameraControls();
	}

	if (UsesCameraControls())
	{
		return Camera != nullptr;
	}

	return GetOwner() != nullptr;
}
bool UPaintingModeControllerComponent::HasExplicitCameraName() const
{
	return !CameraName.IsNone();
}

bool UPaintingModeControllerComponent::ShouldUseSpringArmCameraControls() const
{
	return ControlMode == EPaintingModeControllerControlMode::CharacterLock && SpringArm;
}
void UPaintingModeControllerComponent::ReportCharacterControlReferenceError(const FString& Message) const
{
	UE_LOG(LogMeshPaintingModeController, Error, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Red, Message);
	}
}

void UPaintingModeControllerComponent::LoadDefaultInputAssets()
{
	if (!bLoadDefaultInputAssets) return;

	if (!PaintingMoveAction)
		PaintingMoveAction = LoadObject<UInputAction>(nullptr, TEXT("/MeshPaintingCore/Input/IA_PaintingMove.IA_PaintingMove"));
	if (!PaintAction) PaintAction = LoadObject<UInputAction>(nullptr, TEXT("/MeshPaintingCore/Input/IA_Paint.IA_Paint"));
	if (!OrbitCameraAction)
		OrbitCameraAction = LoadObject<UInputAction>(nullptr, TEXT("/MeshPaintingCore/Input/IA_OrbitCamera.IA_OrbitCamera"));
	if (!PanCameraAction)
		PanCameraAction = LoadObject<UInputAction>(
			nullptr, TEXT("/MeshPaintingCore/Input/IA_PanCamera.IA_PanCamera"));
	if (!AdjustBrushSizeAction)
		AdjustBrushSizeAction = LoadObject<UInputAction>(nullptr, TEXT("/MeshPaintingCore/Input/IA_AdjustBrushSize.IA_AdjustBrushSize"));
	if (!MouseDeltaAction)
		MouseDeltaAction = LoadObject<UInputAction>(nullptr, TEXT("/MeshPaintingCore/Input/IA_MouseDelta.IA_MouseDelta"));
	if (!PickColorAction)
		PickColorAction = LoadObject<UInputAction>(
			nullptr, TEXT("/MeshPaintingCore/Input/IA_PickColorAction.IA_PickColorAction"));
	if (!CameraZoomAction)
		CameraZoomAction = LoadObject<UInputAction>(
			nullptr, TEXT("/MeshPaintingCore/Input/IA_CameraZoom.IA_CameraZoom"));
	if (!AdjustBrushSizeWheelAction)
		AdjustBrushSizeWheelAction = LoadObject<UInputAction>(
			nullptr, TEXT("/MeshPaintingCore/Input/IA_AdjustBrushSizeWheel.IA_AdjustBrushSizeWheel"));
	if (!TogglePaintingModeAction)
		TogglePaintingModeAction = LoadObject<UInputAction>(nullptr, TEXT("/MeshPaintingCore/Input/IA_TogglePaintingMode.IA_TogglePaintingMode"));
	if (!PaintingInputMappingContext)
		PaintingInputMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/MeshPaintingCore/Input/IMC_PaintingMode.IMC_PaintingMode"));
	if (!PaintingToggleInputMappingContext)
		PaintingToggleInputMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/MeshPaintingCore/Input/IMC_PaintingModeToggle.IMC_PaintingModeToggle"));
}

UInputMappingContext* UPaintingModeControllerComponent::GetActivePaintingInputMappingContext()
{
	if (!UsesPaintingInputControls()) return nullptr;
	if (ControlMode == EPaintingModeControllerControlMode::Simple ||
		ControlMode == EPaintingModeControllerControlMode::DroneController ||
		ControlMode == EPaintingModeControllerControlMode::FirstPersonController)
	{
		return GetOrCreateSimplePaintingInputMappingContext();
	}

	if (PaintingInputMappingContext) return PaintingInputMappingContext;

	if (UInputMappingContext* SimpleContext = GetOrCreateSimplePaintingInputMappingContext())
	{
		return SimpleContext;
	}

	return PaintingInputMappingContext;
}

UInputMappingContext* UPaintingModeControllerComponent::GetOrCreateSimplePaintingInputMappingContext()
{
	const bool bIncludeCameraControls = UsesCameraControls();
	const bool bIncludeFullCameraControls = UsesFullCameraControls();
	if (SimplePaintingInputMappingContext &&
		bSimplePaintingInputMappingContextIncludesCameraControls == bIncludeCameraControls &&
		bSimplePaintingInputMappingContextIncludesFullCameraControls == bIncludeFullCameraControls)
	{
		return SimplePaintingInputMappingContext;
	}

	SimplePaintingInputMappingContext = nullptr;
	bSimplePaintingInputMappingContextIncludesCameraControls = bIncludeCameraControls;
	bSimplePaintingInputMappingContextIncludesFullCameraControls = bIncludeFullCameraControls;
	if (!PaintAction || !AdjustBrushSizeAction || !MouseDeltaAction) return nullptr;

	SimplePaintingInputMappingContext = NewObject<UInputMappingContext>(
		this, TEXT("MeshPaintingCore_SimpleInputMappingContext"), RF_Transient);
	if (!SimplePaintingInputMappingContext) return nullptr;

	SimplePaintingInputMappingContext->MapKey(PaintAction, EKeys::LeftMouseButton);
	if (bIncludeCameraControls && OrbitCameraAction) SimplePaintingInputMappingContext->MapKey(OrbitCameraAction, EKeys::MiddleMouseButton);
	SimplePaintingInputMappingContext->MapKey(MouseDeltaAction, EKeys::Mouse2D);
	if (PickColorAction) SimplePaintingInputMappingContext->MapKey(PickColorAction, EKeys::SpaceBar);
	if (AdjustBrushSizeWheelAction) SimplePaintingInputMappingContext->MapKey(AdjustBrushSizeWheelAction, EKeys::MouseWheelAxis);
	return SimplePaintingInputMappingContext;
}

APlayerController* UPaintingModeControllerComponent::ResolveLocalPlayerController() const
{
	if (APlayerController* PlayerController = BoundPlayerController.Get())
	{
		if (PlayerController->IsLocalPlayerController()) return PlayerController;
	}

	const ACharacter* Character = OwnerCharacter ? OwnerCharacter.Get() : Cast<ACharacter>(GetOwner());
	if (UsesCharacterControls())
	{
		if (!Character || !Character->IsLocallyControlled()) return nullptr;

		APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
		if (!PlayerController || !PlayerController->IsLocalPlayerController()) return nullptr;
		if (PlayerController->GetPawn() != Character) return nullptr;
		return PlayerController;
	}

	AActor* OwnerActor = GetOwner();
	if (APlayerController* OwnerPlayerController = Cast<APlayerController>(OwnerActor))
	{
		return OwnerPlayerController->IsLocalPlayerController() ? OwnerPlayerController : nullptr;
	}

	if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		if (!OwnerPawn->IsLocallyControlled()) return nullptr;

		APlayerController* PawnPlayerController = Cast<APlayerController>(OwnerPawn->GetController());
		if (PawnPlayerController && PawnPlayerController->IsLocalPlayerController()) return PawnPlayerController;
	}

	if (AController* OwnerController = Cast<AController>(OwnerActor))
	{
		APlayerController* OwnerAsPlayerController = Cast<APlayerController>(OwnerController);
		if (OwnerAsPlayerController && OwnerAsPlayerController->IsLocalPlayerController()) return OwnerAsPlayerController;
	}

	UWorld* World = GetWorld();
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (PlayerController && PlayerController->IsLocalPlayerController()) return PlayerController;
	}

	return nullptr;
}

AController* UPaintingModeControllerComponent::ResolveOwnerController() const
{
	AActor* OwnerActor = GetOwner();
	if (AController* OwnerController = Cast<AController>(OwnerActor)) return OwnerController;

	if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		return OwnerPawn->GetController();
	}

	return BoundPlayerController.Get();
}

bool UPaintingModeControllerComponent::ShouldSubmitPaintCommandToServer(
	const URuntimeMeshPaintTargetComponent* PaintTarget) const
{
	if (!PaintTarget || !PaintTarget->bReplicateRuntimePaint) return false;

	if (GetWorld()->GetNetMode() == NM_Standalone) return false;

	return true;
}

void UPaintingModeControllerComponent::SubmitPaintCommandBatch(
	const FRuntimeMeshPaintNetCommandBatch& CommandBatch,
	bool bReliable)
{
	if (CommandBatch.Commands.Num() == 0) return;

	if (AActor* OwnerActor = GetOwner())
	{
		if (OwnerActor->HasAuthority())
		{
			AController* InstigatorController = ResolveOwnerController();
			for (const FRuntimeMeshPaintNetCommand& Command : CommandBatch.Commands)
			{
				URuntimeMeshPaintTargetComponent* PaintTarget =
					Cast<URuntimeMeshPaintTargetComponent>(Command.TargetComponent.Get());
				if (PaintTarget)
				{
					PaintTarget->AcceptReplicatedPaintCommand(Command, InstigatorController, false);
				}
			}
			return;
		}
	}

	if (bReliable)
	{
		ServerSubmitPaintCommandBatchReliable(CommandBatch);
	}
	else
	{
		ServerSubmitPaintCommandBatch(CommandBatch);
	}
}

void UPaintingModeControllerComponent::FlushCurrentStrokeReliableCommands()
{
	if (CurrentStrokeReliableCommands.Num() == 0) return;

	int32 CommandIndex = 0;
	while (CommandIndex < CurrentStrokeReliableCommands.Num())
	{
		const int32 BatchCount = FMath::Min(
			GMeshPaintingCoreReliableStrokeBatchSize,
			CurrentStrokeReliableCommands.Num() - CommandIndex);

		FRuntimeMeshPaintNetCommandBatch CommandBatch;
		CommandBatch.Commands.Reserve(BatchCount);
		for (int32 BatchCommandIndex = 0; BatchCommandIndex < BatchCount; ++BatchCommandIndex)
		{
			CommandBatch.Commands.Add(CurrentStrokeReliableCommands[CommandIndex + BatchCommandIndex]);
		}

		SubmitPaintCommandBatch(CommandBatch, true);
		CommandIndex += BatchCount;
	}

	CurrentStrokeReliableCommands.Reset();
}

void UPaintingModeControllerComponent::NotifyColorPickerPaintApplied()
{
	if (!ColorPickerWidget) return;

	if (bIsPainting)
	{
		if (bColorPickerNotifiedForCurrentStroke) return;
		bColorPickerNotifiedForCurrentStroke = true;
	}

	ColorPickerWidget->NotifyPaintApplied();
}

UEnhancedInputLocalPlayerSubsystem* UPaintingModeControllerComponent::GetEnhancedInputSubsystem() const
{
	APlayerController* PlayerController = BoundPlayerController ? BoundPlayerController.Get() : ResolveLocalPlayerController();
	if (!PlayerController) return nullptr;

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
}

bool UPaintingModeControllerComponent::BindInput()
{
	if (PaintingInputComponent) return true;
	if (!UsesPaintingInputControls()) return true;

	LoadDefaultInputAssets();
	APlayerController* PlayerController = ResolveLocalPlayerController();
	if (!PlayerController) return false;

	PaintingInputComponent = NewObject<UEnhancedInputComponent>(this, TEXT("PaintingModeInputComponent"));
	if (!PaintingInputComponent) return false;

	PaintingInputComponent->RegisterComponentWithWorld(PlayerController->GetWorld());
	PaintingInputComponent->Priority = PaintingInputPriority;
	PaintingInputComponent->bBlockInput = false;

	if (UsesCharacterControls() && PaintingMoveAction)
		PaintingInputComponent->BindAction(PaintingMoveAction, ETriggerEvent::Triggered, this, &UPaintingModeControllerComponent::HandleMoveInput);
	if (PaintAction)
	{
		PaintingInputComponent->BindAction(PaintAction, ETriggerEvent::Started, this, &UPaintingModeControllerComponent::HandlePaintStarted);
		PaintingInputComponent->BindAction(PaintAction, ETriggerEvent::Triggered, this, &UPaintingModeControllerComponent::HandlePaintTriggered);
		PaintingInputComponent->BindAction(PaintAction, ETriggerEvent::Completed, this, &UPaintingModeControllerComponent::HandlePaintCompleted);
		PaintingInputComponent->BindAction(PaintAction, ETriggerEvent::Canceled, this, &UPaintingModeControllerComponent::HandlePaintCompleted);
	}
	if (UsesCameraControls() && OrbitCameraAction)
	{
		PaintingInputComponent->BindAction(OrbitCameraAction, ETriggerEvent::Started, this, &UPaintingModeControllerComponent::HandleOrbitStarted);
		PaintingInputComponent->BindAction(OrbitCameraAction, ETriggerEvent::Completed, this, &UPaintingModeControllerComponent::HandleOrbitCompleted);
		PaintingInputComponent->BindAction(OrbitCameraAction, ETriggerEvent::Canceled, this, &UPaintingModeControllerComponent::HandleOrbitCompleted);
	}
	if (UsesFullCameraControls() && PanCameraAction)
	{
		PaintingInputComponent->BindAction(PanCameraAction, ETriggerEvent::Started, this, &UPaintingModeControllerComponent::HandlePanCameraStarted);
		PaintingInputComponent->BindAction(PanCameraAction, ETriggerEvent::Completed, this, &UPaintingModeControllerComponent::HandlePanCameraCompleted);
		PaintingInputComponent->BindAction(PanCameraAction, ETriggerEvent::Canceled, this, &UPaintingModeControllerComponent::HandlePanCameraCompleted);
	}
	if (AdjustBrushSizeAction)
	{
		PaintingInputComponent->BindAction(AdjustBrushSizeAction, ETriggerEvent::Started, this, &UPaintingModeControllerComponent::HandleBrushSizeStarted);
		PaintingInputComponent->BindAction(AdjustBrushSizeAction, ETriggerEvent::Completed, this, &UPaintingModeControllerComponent::HandleBrushSizeCompleted);
		PaintingInputComponent->BindAction(AdjustBrushSizeAction, ETriggerEvent::Canceled, this, &UPaintingModeControllerComponent::HandleBrushSizeCompleted);
	}
	if (MouseDeltaAction)
		PaintingInputComponent->BindAction(MouseDeltaAction, ETriggerEvent::Triggered, this, &UPaintingModeControllerComponent::HandleMouseDelta);
	if (PickColorAction)
		PaintingInputComponent->BindAction(PickColorAction, ETriggerEvent::Started, this, &UPaintingModeControllerComponent::HandlePickColor);
	if (UsesFullCameraControls() && CameraZoomAction)
		PaintingInputComponent->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &UPaintingModeControllerComponent::HandleCameraZoom);
	if (AdjustBrushSizeWheelAction)
		PaintingInputComponent->BindAction(
			AdjustBrushSizeWheelAction, ETriggerEvent::Triggered, this, &UPaintingModeControllerComponent::HandleBrushSizeWheel);
	if (TogglePaintingModeAction)
		PaintingInputComponent->BindAction(TogglePaintingModeAction, ETriggerEvent::Started, this, &UPaintingModeControllerComponent::HandleTogglePaintingMode);

	PlayerController->PushInputComponent(PaintingInputComponent);
	BoundPlayerController = PlayerController;
	AddToggleMappingContext();
	return true;
}

void UPaintingModeControllerComponent::UnbindInput()
{
	RemovePaintingMappingContext();
	RemoveToggleMappingContext();

	if (APlayerController* PlayerController = BoundPlayerController.Get())
	{
		if (PaintingInputComponent) PlayerController->PopInputComponent(PaintingInputComponent);
	}

	if (PaintingInputComponent)
	{
		PaintingInputComponent->DestroyComponent();
		PaintingInputComponent = nullptr;
	}

	BoundPlayerController = nullptr;
}

void UPaintingModeControllerComponent::AddPaintingMappingContext()
{
	if (bMappingContextAdded) return;

	UInputMappingContext* ActiveMappingContext = GetActivePaintingInputMappingContext();
	if (!ActiveMappingContext) return;

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetEnhancedInputSubsystem())
	{
		InputSubsystem->AddMappingContext(ActiveMappingContext, PaintingInputPriority);
		AddedPaintingInputMappingContext = ActiveMappingContext;
		bMappingContextAdded = true;
	}
}

void UPaintingModeControllerComponent::RemovePaintingMappingContext()
{
	if (!bMappingContextAdded) return;

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetEnhancedInputSubsystem())
	{
		if (AddedPaintingInputMappingContext) InputSubsystem->RemoveMappingContext(AddedPaintingInputMappingContext);
	}

	AddedPaintingInputMappingContext = nullptr;
	bMappingContextAdded = false;
}

void UPaintingModeControllerComponent::AddToggleMappingContext()
{
	if (bToggleMappingContextAdded || !PaintingToggleInputMappingContext) return;

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetEnhancedInputSubsystem())
	{
		InputSubsystem->AddMappingContext(PaintingToggleInputMappingContext, PaintingInputPriority);
		bToggleMappingContextAdded = true;
	}
}

void UPaintingModeControllerComponent::RemoveToggleMappingContext()
{
	if (!bToggleMappingContextAdded || !PaintingToggleInputMappingContext) return;

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetEnhancedInputSubsystem())
		InputSubsystem->RemoveMappingContext(PaintingToggleInputMappingContext);

	bToggleMappingContextAdded = false;
}

void UPaintingModeControllerComponent::SavePrePaintingState()
{
	PreviousMaxWalkSpeed = CharacterMovement ? CharacterMovement->MaxWalkSpeed : 0.0f;
	bPreviousShowMouseCursor = BoundPlayerController ? BoundPlayerController->bShowMouseCursor : false;
	PreviousMouseCursor = BoundPlayerController ? BoundPlayerController->CurrentMouseCursor.GetValue() : EMouseCursor::Default;
	bPreviousUseControllerRotationYaw = OwnerCharacter ? OwnerCharacter->bUseControllerRotationYaw : false;
	bPreviousUseControllerRotationPitch = OwnerCharacter ? OwnerCharacter->bUseControllerRotationPitch : false;
	bPreviousUseControllerRotationRoll = OwnerCharacter ? OwnerCharacter->bUseControllerRotationRoll : false;
	bPreviousOrientRotationToMovement = CharacterMovement ? CharacterMovement->bOrientRotationToMovement : false;
	bPreviousUseControllerDesiredRotation = CharacterMovement ? CharacterMovement->bUseControllerDesiredRotation : false;
	bPreviousSpringArmUsePawnControlRotation = SpringArm ? SpringArm->bUsePawnControlRotation : false;
	bPreviousCameraUsePawnControlRotation = Camera ? Camera->bUsePawnControlRotation : false;
	PreviousSpringArmRelativeRotation = SpringArm ? SpringArm->GetRelativeRotation() : FRotator::ZeroRotator;
	PreviousSpringArmTargetArmLength = SpringArm ? SpringArm->TargetArmLength : 0.0f;
	PreviousSpringArmSocketOffset = SpringArm ? SpringArm->SocketOffset : FVector::ZeroVector;
	PreviousCameraRelativeLocation = Camera ? Camera->GetRelativeLocation() : FVector::ZeroVector;
	PreviousCameraRelativeRotation = Camera ? Camera->GetRelativeRotation() : FRotator::ZeroRotator;
}

void UPaintingModeControllerComponent::RestorePrePaintingState()
{
	if (CharacterMovement)
	{
		CharacterMovement->MaxWalkSpeed = PreviousMaxWalkSpeed;
		CharacterMovement->bOrientRotationToMovement = bPreviousOrientRotationToMovement;
		CharacterMovement->bUseControllerDesiredRotation = bPreviousUseControllerDesiredRotation;
	}

	if (OwnerCharacter)
	{
		OwnerCharacter->bUseControllerRotationYaw = bPreviousUseControllerRotationYaw;
		OwnerCharacter->bUseControllerRotationPitch = bPreviousUseControllerRotationPitch;
		OwnerCharacter->bUseControllerRotationRoll = bPreviousUseControllerRotationRoll;
	}

	if (SpringArm && bSpringArmStateChangedForOrbit)
	{
		SpringArm->bUsePawnControlRotation = bPreviousSpringArmUsePawnControlRotation;
		SpringArm->SetRelativeRotation(PreviousSpringArmRelativeRotation);
		bSpringArmStateChangedForOrbit = false;
	}

	if (Camera && bCameraRotationChangedForOrbit)
	{
		Camera->bUsePawnControlRotation = bPreviousCameraUsePawnControlRotation;
		Camera->SetRelativeRotation(PreviousCameraRelativeRotation);
		bCameraRotationChangedForOrbit = false;
	}

	if (APlayerController* PlayerController = BoundPlayerController.Get())
	{
		if (bAppliedLookInputIgnore)
		{
			PlayerController->SetIgnoreLookInput(false);
			bAppliedLookInputIgnore = false;
		}

		PlayerController->bShowMouseCursor = bPreviousShowMouseCursor;
		PlayerController->CurrentMouseCursor = PreviousMouseCursor;
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false);
		PlayerController->SetInputMode(InputMode);
	}
}

void UPaintingModeControllerComponent::StopPainting()
{
	if (!bIsPainting) return;

	FlushCurrentStrokeReliableCommands();
	bIsPainting = false;
	OnPaintReleased.Broadcast();
}

void UPaintingModeControllerComponent::StopOrbiting()
{
	if (!bIsOrbiting) return;

	bIsOrbiting = false;
	bOrbitUsesControllerRotation = false;
	OnOrbitStopped.Broadcast();
}

void UPaintingModeControllerComponent::StopCameraPan()
{
	bIsPanningCamera = false;
}

void UPaintingModeControllerComponent::StopBrushSizeAdjustment()
{
	bIsAdjustingBrushSize = false;
}

void UPaintingModeControllerComponent::CancelEraserMode()
{
	if (!bBrushErase) return;

	if (ColorPickerWidget)
	{
		ColorPickerWidget->SetEraserActive(false, true);
	}
	else
	{
		bBrushErase = false;
		RefreshPaintingMouseCursor();
	}
}

void UPaintingModeControllerComponent::MoveWithCamera(const FVector2D& MoveValue)
{
	if (!OwnerCharacter || MoveValue.IsNearlyZero()) return;

	const FRotator CameraRotation = ShouldUseSpringArmCameraControls() ? SpringArm->GetComponentRotation() :
		(Camera ? Camera->GetComponentRotation() : OwnerCharacter->GetActorRotation());
	const FRotator YawRotation(0.0f, CameraRotation.Yaw, 0.0f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	OwnerCharacter->AddMovementInput(Forward, MoveValue.Y);
	OwnerCharacter->AddMovementInput(Right, MoveValue.X);
	if (bIsPaintingModeActive) OwnerCharacter->SetActorRotation(PaintingModeActorRotation);
}

void UPaintingModeControllerComponent::BeginOrbitCamera()
{
	if (ControlMode == EPaintingModeControllerControlMode::DroneController)
	{
		const AActor* OwnerActor = GetOwner();
		const FRotator SourceRotation = OwnerActor ? OwnerActor->GetActorRotation() :
			(Camera ? Camera->GetComponentRotation() : FRotator::ZeroRotator);
		OrbitYaw = SourceRotation.Yaw;
		OrbitPitch = FRotator::NormalizeAxis(SourceRotation.Pitch);
		bOrbitUsesControllerRotation = false;
		return;
	}

	if (UsesFirstPersonCameraControls())
	{
		APlayerController* PlayerController = BoundPlayerController ? BoundPlayerController.Get() : ResolveLocalPlayerController();
		const FRotator ControlRotation = PlayerController ? PlayerController->GetControlRotation() :
			(Camera ? Camera->GetComponentRotation() : FRotator::ZeroRotator);
		OrbitYaw = ControlRotation.Yaw;
		OrbitPitch = FRotator::NormalizeAxis(ControlRotation.Pitch);
		bOrbitUsesControllerRotation = true;
		return;
	}

	if (ShouldUseSpringArmCameraControls())
	{
		bOrbitUsesControllerRotation = SpringArm->bUsePawnControlRotation && BoundPlayerController;
		if (bOrbitUsesControllerRotation)
		{
			const FRotator ControlRotation = BoundPlayerController->GetControlRotation();
			OrbitYaw = ControlRotation.Yaw;
			OrbitPitch = FRotator::NormalizeAxis(ControlRotation.Pitch);
			return;
		}

		const FRotator SpringArmRotation = SpringArm->GetRelativeRotation();
		OrbitYaw = SpringArmRotation.Yaw;
		OrbitPitch = FRotator::NormalizeAxis(SpringArmRotation.Pitch);
		bSpringArmStateChangedForOrbit = true;
		return;
	}

	const AActor* OwnerActor = GetOwner();
	FRotator OrbitSourceRotation = FRotator::ZeroRotator;
	if (Camera && Camera->bUsePawnControlRotation)
	{
		OrbitSourceRotation = BoundPlayerController ? BoundPlayerController->GetControlRotation() : Camera->GetComponentRotation();
	}
	else if (Camera)
	{
		OrbitSourceRotation = Camera->GetRelativeRotation();
	}
	else if (OwnerActor)
	{
		OrbitSourceRotation = OwnerActor->GetActorRotation();
	}
	OrbitYaw = OrbitSourceRotation.Yaw;
	OrbitPitch = FRotator::NormalizeAxis(OrbitSourceRotation.Pitch);
	bOrbitUsesControllerRotation = false;
}

void UPaintingModeControllerComponent::OrbitCamera(const FVector2D& MouseDelta)
{
	if (MouseDelta.IsNearlyZero()) return;

	OrbitYaw += MouseDelta.X * OrbitYawSensitivity;
	const float NewOrbitPitch = OrbitPitch + MouseDelta.Y * OrbitPitchSensitivity;

	if (ControlMode == EPaintingModeControllerControlMode::DroneController)
	{
		float DronePitchMin = -89.0f;
		float DronePitchMax = 89.0f;
		if (APlayerController* PlayerController = BoundPlayerController ? BoundPlayerController.Get() : ResolveLocalPlayerController())
		{
			if (PlayerController->PlayerCameraManager)
			{
				DronePitchMin = PlayerController->PlayerCameraManager->ViewPitchMin;
				DronePitchMax = PlayerController->PlayerCameraManager->ViewPitchMax;
			}
		}

		const float SafePitchMin = FMath::Min(DronePitchMin, DronePitchMax);
		const float SafePitchMax = FMath::Max(DronePitchMin, DronePitchMax);
		OrbitPitch = FMath::Clamp(NewOrbitPitch, SafePitchMin, SafePitchMax);
		const FRotator NewRotation(OrbitPitch, OrbitYaw, 0.0f);
		if (AActor* OwnerActor = GetOwner())
		{
			OwnerActor->SetActorRotation(NewRotation);
		}
		else if (Camera)
		{
			Camera->SetRelativeRotation(NewRotation);
			bCameraRotationChangedForOrbit =
				!Camera->GetRelativeRotation().Equals(PreviousCameraRelativeRotation, 0.1f) ||
				Camera->bUsePawnControlRotation != bPreviousCameraUsePawnControlRotation;
		}
		InvalidateBrushAreaPreviewCache();
		return;
	}

	if (UsesFirstPersonCameraControls())
	{
		APlayerController* PlayerController = BoundPlayerController ? BoundPlayerController.Get() : ResolveLocalPlayerController();
		if (!PlayerController) return;

		float ControlRotationPitchMin = -89.0f;
		float ControlRotationPitchMax = 89.0f;
		if (PlayerController->PlayerCameraManager)
		{
			ControlRotationPitchMin = PlayerController->PlayerCameraManager->ViewPitchMin;
			ControlRotationPitchMax = PlayerController->PlayerCameraManager->ViewPitchMax;
		}

		const float SafePitchMin = FMath::Min(ControlRotationPitchMin, ControlRotationPitchMax);
		const float SafePitchMax = FMath::Max(ControlRotationPitchMin, ControlRotationPitchMax);
		OrbitPitch = FMath::Clamp(NewOrbitPitch, SafePitchMin, SafePitchMax);
		const FRotator NewControlRotation(OrbitPitch, OrbitYaw, 0.0f);
		PlayerController->SetControlRotation(NewControlRotation);
		InvalidateBrushAreaPreviewCache();
		return;
	}

	OrbitPitch = FMath::Clamp(NewOrbitPitch, MinimumOrbitPitch, MaximumOrbitPitch);

	if (ShouldUseSpringArmCameraControls())
	{
		if (bOrbitUsesControllerRotation)
		{
			if (APlayerController* PlayerController = BoundPlayerController.Get())
				PlayerController->SetControlRotation(FRotator(OrbitPitch, OrbitYaw, 0.0f));
			return;
		}

		ApplySpringArmOrbitRotation();
		return;
	}

	if (Camera)
	{
		if (Camera->bUsePawnControlRotation)
		{
			Camera->bUsePawnControlRotation = false;
		}

		Camera->SetRelativeRotation(FRotator(OrbitPitch, OrbitYaw, 0.0f));
		bCameraRotationChangedForOrbit =
			!Camera->GetRelativeRotation().Equals(PreviousCameraRelativeRotation, 0.1f) ||
			Camera->bUsePawnControlRotation != bPreviousCameraUsePawnControlRotation;
		InvalidateBrushAreaPreviewCache();
		return;
	}

	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->SetActorRotation(FRotator(OrbitPitch, OrbitYaw, 0.0f));
		InvalidateBrushAreaPreviewCache();
	}
}

void UPaintingModeControllerComponent::BeginCameraPan()
{
	if (!ShouldUseSpringArmCameraControls() && !Camera) return;

	StopOrbiting();
	StopPainting();
	bIsPanningCamera = true;
}

void UPaintingModeControllerComponent::PanCamera(const FVector2D& MouseDelta)
{
	if (MouseDelta.IsNearlyZero()) return;
	const bool bUseSpringArmControls = ShouldUseSpringArmCameraControls();
	const float DistanceScale = bUseSpringArmControls ? FMath::Max(SpringArm->TargetArmLength, 1.0f) / 300.0f : 1.0f;
	const FVector LocalPanOffset(0.0f, -MouseDelta.X, -MouseDelta.Y);

	if (bUseSpringArmControls)
	{
		FVector NewSocketOffset = SpringArm->SocketOffset + (LocalPanOffset * CameraPanSensitivity * DistanceScale);
		if (CameraPanMaxOffset > 0.0f)
		{
			const FVector OffsetFromDefault = NewSocketOffset - PreviousSpringArmSocketOffset;
			NewSocketOffset = PreviousSpringArmSocketOffset +
				OffsetFromDefault.GetClampedToMaxSize(CameraPanMaxOffset);
		}

		SpringArm->SocketOffset = NewSocketOffset;
		bSpringArmSocketOffsetChangedForPan = !SpringArm->SocketOffset.Equals(PreviousSpringArmSocketOffset, 0.1f);
	}
	else if (Camera)
	{
		FVector NewCameraLocation = Camera->GetRelativeLocation() + (LocalPanOffset * CameraPanSensitivity);
		if (CameraPanMaxOffset > 0.0f)
		{
			const FVector OffsetFromDefault = NewCameraLocation - PreviousCameraRelativeLocation;
			NewCameraLocation = PreviousCameraRelativeLocation +
				OffsetFromDefault.GetClampedToMaxSize(CameraPanMaxOffset);
		}

		Camera->SetRelativeLocation(NewCameraLocation);
		bCameraRelativeLocationChangedForPan = !Camera->GetRelativeLocation().Equals(PreviousCameraRelativeLocation, 0.1f);
	}

	InvalidateBrushAreaPreviewCache();
}

void UPaintingModeControllerComponent::AdjustBrushSize(const FVector2D& MouseDelta)
{
	if (MouseDelta.IsNearlyZero()) return;

	SetBrushSize(BrushSize + ((MouseDelta.X + MouseDelta.Y) * BrushSizeSensitivity));
}

void UPaintingModeControllerComponent::AdjustBrushSizeFromWheel(float WheelValue)
{
	if (FMath::IsNearlyZero(WheelValue)) return;

	SetBrushSize(BrushSize + (WheelValue * BrushSizeWheelSensitivity));
}

void UPaintingModeControllerComponent::ZoomCamera(float WheelValue)
{
	const bool bUseSpringArmControls = ShouldUseSpringArmCameraControls();
	if ((!bUseSpringArmControls && !Camera) || FMath::IsNearlyZero(WheelValue)) return;

	const float SafeMin = FMath::Min(MinimumCameraZoomDistance, MaximumCameraZoomDistance);
	const float SafeMax = FMath::Max(MinimumCameraZoomDistance, MaximumCameraZoomDistance);
	if (bUseSpringArmControls)
	{
		const float DefaultZoomOutLimit = PreviousSpringArmTargetArmLength > 0.0f ? PreviousSpringArmTargetArmLength : SafeMax;
		const float MaxAllowedArmLength = FMath::Max(SafeMin, DefaultZoomOutLimit);
		const float NewTargetArmLength = FMath::Clamp(
			SpringArm->TargetArmLength - (WheelValue * CameraZoomSensitivity),
			SafeMin,
			MaxAllowedArmLength);
		if (FMath::IsNearlyEqual(SpringArm->TargetArmLength, NewTargetArmLength)) return;

		SpringArm->TargetArmLength = NewTargetArmLength;
		bSpringArmTargetArmLengthChangedForZoom =
			!FMath::IsNearlyEqual(SpringArm->TargetArmLength, PreviousSpringArmTargetArmLength, 0.1f);
		InvalidateBrushAreaPreviewCache();
		return;
	}

	const FVector CameraForward = PreviousCameraRelativeRotation.Vector();
	const float DefaultCameraDistance = PreviousCameraRelativeLocation.Size();
	if (DefaultCameraDistance <= KINDA_SMALL_NUMBER || CameraForward.IsNearlyZero()) return;

	const float CurrentZoomOffset = FVector::DotProduct(Camera->GetRelativeLocation() - PreviousCameraRelativeLocation, CameraForward);
	const float CurrentCameraDistance = FMath::Max(0.0f, DefaultCameraDistance - CurrentZoomOffset);
	const float MinAllowedCameraDistance = FMath::Min(SafeMin, DefaultCameraDistance);
	const float MaxAllowedCameraDistance = FMath::Max(MinAllowedCameraDistance, DefaultCameraDistance);
	const float NewCameraDistance = FMath::Clamp(
		CurrentCameraDistance - (WheelValue * CameraZoomSensitivity),
		MinAllowedCameraDistance,
		MaxAllowedCameraDistance);
	if (FMath::IsNearlyEqual(CurrentCameraDistance, NewCameraDistance)) return;

	const float NewZoomOffset = DefaultCameraDistance - NewCameraDistance;
	Camera->SetRelativeLocation(PreviousCameraRelativeLocation + (CameraForward * NewZoomOffset));
	bCameraRelativeLocationChangedForPan = !Camera->GetRelativeLocation().Equals(PreviousCameraRelativeLocation, 0.1f);
	InvalidateBrushAreaPreviewCache();
}

void UPaintingModeControllerComponent::PickColorUnderCursor()
{
	if (IsCursorOverPaintingUI() || (ColorPickerWidget && ColorPickerWidget->IsEyedropperActive())) return;

	ResolveAutoRegisteredPaintTarget();

	APlayerController* PlayerController = BoundPlayerController ? BoundPlayerController.Get() : ResolveLocalPlayerController();
	if (!PlayerController) return;

	FRuntimeMeshPaintSampleResult SampleResult;
	if (!ColorPickerEyedropper::SampleUnderCursor(
			PlayerController, PaintTraceChannel, bPaintTraceComplex, this, SampleResult, ColorPickerSampleMode) ||
		!SampleResult.bSuccess)
	{
		return;
	}

	if (ColorPickerWidget)
	{
		ColorPickerWidget->SetCurrentColor(SampleResult.Color, true);
		ColorPickerWidget->CommitCurrentColor();
		ColorPickerWidget->OnColorSampled.Broadcast(SampleResult);
		return;
	}

	FMeshPaintBrushMaterialSettings Settings = GetBrushMaterialSettings();
	Settings.Color = SampleResult.Color;
	ApplyBrushMaterialSettings_Implementation(Settings);
}

bool UPaintingModeControllerComponent::IsControlKeyDown() const
{
	APlayerController* PlayerController = BoundPlayerController ? BoundPlayerController.Get() : ResolveLocalPlayerController();
	return PlayerController &&
		(PlayerController->IsInputKeyDown(EKeys::LeftControl) || PlayerController->IsInputKeyDown(EKeys::RightControl));
}

bool UPaintingModeControllerComponent::IsShiftKeyDown() const
{
	APlayerController* PlayerController = BoundPlayerController ? BoundPlayerController.Get() : ResolveLocalPlayerController();
	return PlayerController &&
		(PlayerController->IsInputKeyDown(EKeys::LeftShift) || PlayerController->IsInputKeyDown(EKeys::RightShift));
}

void UPaintingModeControllerComponent::ApplySpringArmOrbitRotation()
{
	if (SpringArm) SpringArm->SetRelativeRotation(FRotator(OrbitPitch, OrbitYaw, 0.0f));
}

void UPaintingModeControllerComponent::BeginCameraRestore()
{
	const bool bNeedsRestore =
		(SpringArm && bSpringArmTargetArmLengthChangedForZoom) ||
		(SpringArm && bSpringArmSocketOffsetChangedForPan) ||
		(Camera && bCameraRelativeLocationChangedForPan);
	if (!bNeedsRestore)
	{
		bCameraRestoreActive = false;
		return;
	}

	if (CameraRestoreSmoothingSpeed <= 0.0f)
	{
		ApplyCameraRestoreImmediate();
		return;
	}

	bCameraRestoreActive = true;
}

void UPaintingModeControllerComponent::ApplyCameraRestoreImmediate()
{
	if (SpringArm && bSpringArmTargetArmLengthChangedForZoom)
	{
		SpringArm->TargetArmLength = PreviousSpringArmTargetArmLength;
		bSpringArmTargetArmLengthChangedForZoom = false;
	}

	if (SpringArm && bSpringArmSocketOffsetChangedForPan)
	{
		SpringArm->SocketOffset = PreviousSpringArmSocketOffset;
		bSpringArmSocketOffsetChangedForPan = false;
	}

	if (Camera && bCameraRelativeLocationChangedForPan)
	{
		Camera->SetRelativeLocation(PreviousCameraRelativeLocation);
		bCameraRelativeLocationChangedForPan = false;
	}

	bCameraRestoreActive = false;
}

void UPaintingModeControllerComponent::UpdateCameraRestore(float DeltaTime)
{
	const float RestoreSpeed = FMath::Max(0.0f, CameraRestoreSmoothingSpeed);
	if (RestoreSpeed <= 0.0f)
	{
		ApplyCameraRestoreImmediate();
		return;
	}

	bool bRestoreComplete = true;

	if (!SpringArm)
	{
		bSpringArmTargetArmLengthChangedForZoom = false;
		bSpringArmSocketOffsetChangedForPan = false;
	}

	if (!Camera)
	{
		bCameraRelativeLocationChangedForPan = false;
	}

	if (SpringArm && bSpringArmTargetArmLengthChangedForZoom)
	{
		SpringArm->TargetArmLength = FMath::FInterpTo(
			SpringArm->TargetArmLength, PreviousSpringArmTargetArmLength, DeltaTime, RestoreSpeed);
		if (FMath::IsNearlyEqual(SpringArm->TargetArmLength, PreviousSpringArmTargetArmLength, 0.1f))
		{
			SpringArm->TargetArmLength = PreviousSpringArmTargetArmLength;
			bSpringArmTargetArmLengthChangedForZoom = false;
		}
		else
		{
			bRestoreComplete = false;
		}
	}

	if (SpringArm && bSpringArmSocketOffsetChangedForPan)
	{
		SpringArm->SocketOffset = FMath::VInterpTo(
			SpringArm->SocketOffset, PreviousSpringArmSocketOffset, DeltaTime, RestoreSpeed);
		if (SpringArm->SocketOffset.Equals(PreviousSpringArmSocketOffset, 0.1f))
		{
			SpringArm->SocketOffset = PreviousSpringArmSocketOffset;
			bSpringArmSocketOffsetChangedForPan = false;
		}
		else
		{
			bRestoreComplete = false;
		}
	}

	if (Camera && bCameraRelativeLocationChangedForPan)
	{
		Camera->SetRelativeLocation(FMath::VInterpTo(
			Camera->GetRelativeLocation(), PreviousCameraRelativeLocation, DeltaTime, RestoreSpeed));
		if (Camera->GetRelativeLocation().Equals(PreviousCameraRelativeLocation, 0.1f))
		{
			Camera->SetRelativeLocation(PreviousCameraRelativeLocation);
			bCameraRelativeLocationChangedForPan = false;
		}
		else
		{
			bRestoreComplete = false;
		}
	}

	if (bRestoreComplete)
	{
		bCameraRestoreActive = false;
		SetComponentTickEnabled(ShouldTickPaintingMode());
	}
}

bool UPaintingModeControllerComponent::ResolveAutoRegisteredPaintTarget()
{
	if (HasPaintTargetComponentFilter())
	{
		PaintTargetComponent = GetFirstAllowedPaintTargetComponent();
		return PaintTargetComponent != nullptr;
	}

	if (PaintTargetComponent) return true;
	if (!bAutoRegister) return false;

	TArray<URuntimeMeshPaintTargetComponent*> PaintTargets;
	if (AActor* OwnerActor = GetOwner()) OwnerActor->GetComponents(PaintTargets);
	if (PaintTargets.Num() != 1) return false;

	PaintTargetComponent = PaintTargets[0];
	return IsValid(PaintTargetComponent.Get());
}

UMaterialInterface* UPaintingModeControllerComponent::ResolvePaintBrushMaterial()
{
	if (CachedPaintBrushMaterial) return CachedPaintBrushMaterial;

	if (!PaintBrushMaterial.IsNull()) CachedPaintBrushMaterial = PaintBrushMaterial.LoadSynchronous();
	if (!CachedPaintBrushMaterial) CachedPaintBrushMaterial = DefaultPaintBrushMaterial;
	return CachedPaintBrushMaterial;
}

bool UPaintingModeControllerComponent::TryCreateColorPickerWidget()
{
	if (!bAutoCreateColorPickerWidget) return false;
	if (ColorPickerWidget) return true;

	APlayerController* PlayerController = BoundPlayerController ? BoundPlayerController.Get() : ResolveLocalPlayerController();
	if (!PlayerController || !ColorPickerWidgetClass) return false;

	UColorPickerPanelWidget* CreatedWidget = CreateWidget<UColorPickerPanelWidget>(PlayerController, ColorPickerWidgetClass);
	if (!CreatedWidget) return false;

	CreatedWidget->EyedropperSampleMode = ColorPickerSampleMode;
	CreatedWidget->SetPaintTarget(this);
	CreatedWidget->AddToViewport(ColorPickerWidgetZOrder);
	ColorPickerWidget = CreatedWidget;
	return true;
}

void UPaintingModeControllerComponent::RemoveColorPickerWidget()
{
	if (ColorPickerWidget) ColorPickerWidget->RemoveFromParent();
	ColorPickerWidget = nullptr;
}

void UPaintingModeControllerComponent::UpdatePaintingMouseCursor()
{
	if (!bUseBrushCursorOutsideUI || !bIsPaintingModeActive) return;

	APlayerController* PlayerController = BoundPlayerController ? BoundPlayerController.Get() : ResolveLocalPlayerController();
	if (!PlayerController) return;
	if (ColorPickerWidget && ColorPickerWidget->IsEyedropperActive()) return;

	const bool bCursorOverPaintingUI = IsCursorOverPaintingUI();
	const EMouseCursor::Type DesiredCursor = !bCursorOverPaintingUI
		? GetPaintingMouseCursor()
		: EMouseCursor::Default;

	PlayerController->CurrentMouseCursor = DesiredCursor;
	if (!bCursorOverPaintingUI) RestorePaintingViewportFocus(PlayerController);
}

void UPaintingModeControllerComponent::UpdateBrushAreaPreview(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_MeshPaintingCore_UpdateBrushPreview);

	if (!bEnableBrushAreaPreview || !bIsPaintingModeActive)
	{
		ResetBrushAreaPreviewState();
		return;
	}

	APlayerController* PlayerController = BoundPlayerController ? BoundPlayerController.Get() : ResolveLocalPlayerController();
	if (!PlayerController || IsCursorOverPaintingUI() || (ColorPickerWidget && ColorPickerWidget->IsEyedropperActive()))
	{
		ResetBrushAreaPreviewState();
		return;
	}

	ResolveAutoRegisteredPaintTarget();

	FVector2D MousePosition;
	if (!GetViewportMousePosition(PlayerController, MousePosition))
	{
		ResetBrushAreaPreviewState();
		return;
	}

	FVector TraceStart = FVector::ZeroVector;
	FVector TraceEnd = FVector::ZeroVector;
	FRuntimeMeshPaintSampleResult PaintHit;
	FLinearColor PreviewColor = BrushColor.GetClamped();
	const FLinearColor PreviewTint = BrushAreaPreviewColor.GetClamped();
	PreviewColor.R *= PreviewTint.R;
	PreviewColor.G *= PreviewTint.G;
	PreviewColor.B *= PreviewTint.B;
	const float PreviewBrightness = FMath::Max(0.0f, BrushAreaPreviewEmissiveIntensity);
	PreviewColor.R *= PreviewBrightness;
	PreviewColor.G *= PreviewBrightness;
	PreviewColor.B *= PreviewBrightness;
	PreviewColor.A = FMath::Clamp(PreviewTint.A * BrushAreaPreviewOpacity, 0.0f, 1.0f);
	const float PreviewLineThickness = FMath::Max(
		0.1f,
		(BrushAreaPreviewLineThickness +
		 BrushSize * FMath::Max(0.0f, BrushAreaPreviewThicknessBrushSizeMultiplier)) *
		GMeshPaintingCoreBrushPreviewLineThicknessScale);
	UObject* PreviewSource = nullptr;

	URuntimeMeshPaintTargetComponent* HitPaintTarget = nullptr;
	FHitResult HitResult;
	if (!TracePaintUnderScreenPosition(PlayerController, MousePosition, HitResult, TraceStart, TraceEnd))
	{
		HoldBrushAreaPreviewOnTransientMiss(DeltaTime);
		return;
	}
	HitResult.TraceStart = TraceStart;
	HitResult.TraceEnd = TraceEnd;

	if (CanReuseBrushAreaPreviewCache(
		PlayerController,
		MousePosition,
		TraceStart,
		TraceEnd,
		HitResult,
		PreviewColor,
		PreviewLineThickness))
	{
		HitPaintTarget = CachedBrushAreaPreviewPaintTarget.Get();
		PaintHit = CachedBrushAreaPreviewHit;
	}
	else
	{
		HitPaintTarget = ResolvePaintTargetFromHit(HitResult);
		const bool bPaintableHit = IsValid(HitPaintTarget) && HitPaintTarget->IsPaintableHit(HitResult);
		if (!bPaintableHit)
		{
			HoldBrushAreaPreviewOnTransientMiss(DeltaTime);
			return;
		}

		if (!HitPaintTarget->ResolveProjectedPaintHit(HitResult, PaintHit))
		{
			HoldBrushAreaPreviewOnTransientMiss(DeltaTime);
			return;
		}
		if (Cast<USkeletalMeshComponent>(PaintHit.HitResult.GetComponent()))
		{
			BuildBrushScreenProjectionData(PlayerController, MousePosition, PaintHit.ProjectionData);
		}

		PreviewSource = HitPaintTarget;
		if (LastBrushAreaPreviewSource.Get() != PreviewSource)
		{
			ClearLastBrushAreaPreviewMask();
			LastBrushAreaPreviewSource = PreviewSource;
		}

		if (!HitPaintTarget->UpdateProjectedBrushPreviewMask(
			PaintHit,
			BrushSize,
			BrushAreaPreviewBrushSizeScale,
			PreviewColor,
			PreviewLineThickness))
		{
			HoldBrushAreaPreviewOnTransientMiss(DeltaTime);
			return;
		}

		StoreBrushAreaPreviewCache(
			PlayerController,
			MousePosition,
			TraceStart,
			TraceEnd,
			HitResult,
			HitPaintTarget,
			PaintHit,
			PreviewColor,
			PreviewLineThickness);
	}

	PreviewSource = HitPaintTarget;

	if (LastBrushAreaPreviewSource.Get() != PreviewSource)
	{
		ClearLastBrushAreaPreviewMask();
		LastBrushAreaPreviewSource = PreviewSource;
	}

	BrushAreaPreviewMissTime = 0.0f;
}

void UPaintingModeControllerComponent::InvalidateBrushAreaPreviewCache()
{
	bBrushAreaPreviewCacheValid = false;
	CachedBrushAreaPreviewPlayerController.Reset();
	CachedBrushAreaPreviewPaintTarget.Reset();
	CachedBrushAreaPreviewHitComponent.Reset();
	CachedBrushAreaPreviewTraceHit = FHitResult();
	CachedBrushAreaPreviewHit = FRuntimeMeshPaintSampleResult();
}

void UPaintingModeControllerComponent::ResetBrushAreaPreviewState()
{
	ClearLastBrushAreaPreviewMask();
	InvalidateBrushAreaPreviewCache();
	BrushAreaPreviewMissTime = 0.0f;
	LastBrushAreaPreviewSource.Reset();
}

void UPaintingModeControllerComponent::ClearLastBrushAreaPreviewMask()
{
	if (URuntimeMeshPaintTargetComponent* PreviousPaintTarget =
		Cast<URuntimeMeshPaintTargetComponent>(LastBrushAreaPreviewSource.Get()))
	{
		PreviousPaintTarget->ClearBrushPreviewMask();
	}
}

bool UPaintingModeControllerComponent::HoldBrushAreaPreviewOnTransientMiss(float DeltaTime)
{
	if (!bBrushAreaPreviewCacheValid ||
		!CachedBrushAreaPreviewPaintTarget.IsValid())
	{
		ResetBrushAreaPreviewState();
		return false;
	}

	BrushAreaPreviewMissTime += FMath::Max(0.0f, DeltaTime);
	if (BrushAreaPreviewMissTime <= GMeshPaintingCoreBrushPreviewMissGraceSeconds)
	{
		return true;
	}

	ResetBrushAreaPreviewState();
	return false;
}

bool UPaintingModeControllerComponent::ShouldTickPaintingMode() const
{
	return bCameraRestoreActive ||
		(bIsPaintingModeActive && (bUseBrushCursorOutsideUI || bEnableBrushAreaPreview));
}

bool UPaintingModeControllerComponent::RestorePaintingViewportFocus(APlayerController* PlayerController) const
{
	if (!PlayerController || !FSlateApplication::IsInitialized() || !FSlateApplication::Get().IsActive()) return false;

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer || !LocalPlayer->ViewportClient) return false;

	TSharedPtr<SViewport> ViewportWidget = LocalPlayer->ViewportClient->GetGameViewportWidget();
	if (!ViewportWidget.IsValid()) return false;
	if (ViewportWidget->HasKeyboardFocus()) return true;

	FSlateApplication& SlateApplication = FSlateApplication::Get();
	if (SlateApplication.HasAnyMouseCaptor()) return false;

	const int32 UserIndex = FMath::Max(0, LocalPlayer->GetPlatformUserIndex());
	return SlateApplication.SetUserFocus(UserIndex, ViewportWidget, EFocusCause::SetDirectly);
}

bool UPaintingModeControllerComponent::GetViewportMousePosition(
	APlayerController* PlayerController, FVector2D& OutMousePosition) const
{
	if (!PlayerController) return false;

	const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (FSlateApplication::IsInitialized() && LocalPlayer && LocalPlayer->ViewportClient)
	{
		TSharedPtr<SViewport> ViewportWidget = LocalPlayer->ViewportClient->GetGameViewportWidget();
		if (ViewportWidget.IsValid())
		{
			const FGeometry& Geometry = ViewportWidget->GetCachedGeometry();
			const FVector2D LocalMousePosition = Geometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
			const FVector2D ViewportLocalSize = Geometry.GetLocalSize();
			if (LocalMousePosition.X >= 0.0f && LocalMousePosition.Y >= 0.0f &&
				LocalMousePosition.X <= ViewportLocalSize.X && LocalMousePosition.Y <= ViewportLocalSize.Y)
			{
				OutMousePosition = LocalMousePosition * Geometry.Scale;
				return true;
			}
		}
	}

	if (LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->Viewport)
	{
		FIntPoint MousePosition;
		LocalPlayer->ViewportClient->Viewport->GetMousePos(MousePosition, true);
		const FIntPoint ViewportSize = LocalPlayer->ViewportClient->Viewport->GetSizeXY();
		if (MousePosition.X >= 0 && MousePosition.Y >= 0 &&
			MousePosition.X <= ViewportSize.X && MousePosition.Y <= ViewportSize.Y)
		{
			OutMousePosition = FVector2D(MousePosition);
			return true;
		}
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (PlayerController->GetMousePosition(MouseX, MouseY))
	{
		OutMousePosition = FVector2D(MouseX, MouseY);
		return true;
	}

	if (!FSlateApplication::IsInitialized()) return false;
	if (!LocalPlayer || !LocalPlayer->ViewportClient) return false;

	TSharedPtr<SViewport> ViewportWidget = LocalPlayer->ViewportClient->GetGameViewportWidget();
	if (!ViewportWidget.IsValid()) return false;

	const FGeometry& Geometry = ViewportWidget->GetCachedGeometry();
	const FVector2D LocalMousePosition = Geometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
	const FVector2D ViewportSize = Geometry.GetLocalSize();
	if (LocalMousePosition.X < 0.0f || LocalMousePosition.Y < 0.0f ||
		LocalMousePosition.X > ViewportSize.X || LocalMousePosition.Y > ViewportSize.Y)
	{
		return false;
	}

	OutMousePosition = LocalMousePosition;
	return true;
}

bool UPaintingModeControllerComponent::IsCursorOverPaintingUI() const
{
	return ColorPickerWidget && ColorPickerWidget->IsCursorOverPanel();
}

EMouseCursor::Type UPaintingModeControllerComponent::GetBrushMouseCursor()
{
	UTexture2D* CursorTexture = BrushCursorTexture.Get();
	if (!CursorTexture && !BrushCursorTexture.IsNull()) CursorTexture = BrushCursorTexture.LoadSynchronous();
	if (!CursorTexture) CursorTexture = DefaultBrushCursorTexture;

	return InitializeMeshPaintingCoreBrushCursor(CursorTexture, BrushCursorHotSpot, BrushColor)
		? EMouseCursor::Custom
		: EMouseCursor::Crosshairs;
}

bool UPaintingModeControllerComponent::BuildBrushScreenProjectionData(
	APlayerController* PlayerController, const FVector2D& MousePosition,
	FRuntimeMeshPaintScreenProjectionData& OutProjectionData) const
{
	OutProjectionData = FRuntimeMeshPaintScreenProjectionData();
	if (!PlayerController) return false;

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	FVector CursorRayOrigin = FVector::ZeroVector;
	FVector CursorRayDirection = FVector::ForwardVector;
	if (!PlayerController->DeprojectScreenPositionToWorld(MousePosition.X, MousePosition.Y, CursorRayOrigin, CursorRayDirection))
	{
		return false;
	}

	return BuildMeshPaintingCoreBrushProjectionDataFromRay(
		ViewRotation,
		CursorRayOrigin,
		CursorRayDirection,
		OutProjectionData);
}

bool UPaintingModeControllerComponent::BuildBrushMouseProjectionData(
	APlayerController* PlayerController,
	FRuntimeMeshPaintScreenProjectionData& OutProjectionData) const
{
	OutProjectionData = FRuntimeMeshPaintScreenProjectionData();
	if (!PlayerController) return false;

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FVector CursorRayOrigin = FVector::ZeroVector;
	FVector CursorRayDirection = FVector::ForwardVector;
	if (!PlayerController->DeprojectMousePositionToWorld(CursorRayOrigin, CursorRayDirection))
	{
		return false;
	}

	return BuildMeshPaintingCoreBrushProjectionDataFromRay(
		ViewRotation,
		CursorRayOrigin,
		CursorRayDirection,
		OutProjectionData);
}

bool UPaintingModeControllerComponent::BuildPaintTraceFromScreenPosition(
	APlayerController* PlayerController, const FVector2D& MousePosition,
	FVector& OutTraceStart, FVector& OutTraceEnd) const
{
	OutTraceStart = FVector::ZeroVector;
	OutTraceEnd = FVector::ZeroVector;
	if (!PlayerController) return false;

	FVector TraceDirection = FVector::ForwardVector;
	if (!PlayerController->DeprojectScreenPositionToWorld(MousePosition.X, MousePosition.Y, OutTraceStart, TraceDirection)) return false;

	OutTraceEnd = OutTraceStart + (TraceDirection * GMeshPaintingCoreBrushTraceDistance);
	return true;
}

bool UPaintingModeControllerComponent::TracePaintUnderScreenPosition(
	APlayerController* PlayerController, const FVector2D& MousePosition,
	FHitResult& OutHitResult, FVector& OutTraceStart, FVector& OutTraceEnd) const
{
	SCOPE_CYCLE_COUNTER(STAT_MeshPaintingCore_TracePaintUnderCursor);

	OutHitResult = FHitResult();
	if (!BuildPaintTraceFromScreenPosition(PlayerController, MousePosition, OutTraceStart, OutTraceEnd)) return false;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeshPaintingCoreBrushTrace), bPaintTraceComplex);
	QueryParams.bReturnFaceIndex = true;
	QueryParams.bReturnPhysicalMaterial = true;

	return PlayerController->GetWorld()->LineTraceSingleByChannel(OutHitResult, OutTraceStart, OutTraceEnd, PaintTraceChannel, QueryParams);
}

bool UPaintingModeControllerComponent::TracePaintUnderCursor(
	APlayerController* PlayerController, FHitResult& OutHitResult, FVector& OutTraceStart, FVector& OutTraceEnd) const
{
	if (!PlayerController) return false;

	FVector2D MousePosition;
	if (!GetViewportMousePosition(PlayerController, MousePosition)) return false;

	return TracePaintUnderScreenPosition(PlayerController, MousePosition, OutHitResult, OutTraceStart, OutTraceEnd);
}

uint32 UPaintingModeControllerComponent::GetBrushAreaPreviewTargetRevision(const UPrimitiveComponent* Component) const
{
	const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Component);
	return SkeletalMeshComponent ? SkeletalMeshComponent->GetBoneTransformRevisionNumber() : 0;
}

bool UPaintingModeControllerComponent::CanReuseBrushAreaPreviewCache(
	APlayerController* PlayerController, const FVector2D& MousePosition,
	const FVector& TraceStart, const FVector& TraceEnd, const FHitResult& TraceHit,
	const FLinearColor& PreviewColor, float PreviewLineThickness) const
{
	if (!bBrushAreaPreviewCacheValid || !CachedBrushAreaPreviewHit.bSuccess) return false;
	if (CachedBrushAreaPreviewPlayerController.Get() != PlayerController) return false;
	if (CachedBrushAreaPreviewTraceChannel.GetValue() != PaintTraceChannel.GetValue() ||
		bCachedBrushAreaPreviewTraceComplex != bPaintTraceComplex)
	{
		return false;
	}

	const URuntimeMeshPaintTargetComponent* PaintTarget = CachedBrushAreaPreviewPaintTarget.Get();
	const UPrimitiveComponent* HitComponent = CachedBrushAreaPreviewHitComponent.Get();
	if (!PaintTarget || !HitComponent || !PaintTarget->IsPaintableHit(CachedBrushAreaPreviewHit.HitResult)) return false;
	if (TraceHit.GetComponent() != HitComponent ||
		TraceHit.FaceIndex != CachedBrushAreaPreviewTraceHit.FaceIndex ||
		TraceHit.Item != CachedBrushAreaPreviewTraceHit.Item ||
		TraceHit.ElementIndex != CachedBrushAreaPreviewTraceHit.ElementIndex ||
		TraceHit.BoneName != CachedBrushAreaPreviewTraceHit.BoneName)
	{
		return false;
	}

	if (!TraceHit.ImpactPoint.Equals(CachedBrushAreaPreviewTraceHit.ImpactPoint, 0.1f) ||
		!TraceHit.ImpactNormal.Equals(CachedBrushAreaPreviewTraceHit.ImpactNormal, 0.01f))
	{
		return false;
	}

	if (!MousePosition.Equals(CachedBrushAreaPreviewMousePosition, 0.1f)) return false;
	if (!TraceStart.Equals(CachedBrushAreaPreviewTraceStart, 0.1f) ||
		!TraceEnd.Equals(CachedBrushAreaPreviewTraceEnd, 1.0f))
	{
		return false;
	}

	if (!HitComponent->GetComponentTransform().Equals(CachedBrushAreaPreviewTargetTransform, 0.01f)) return false;
	if (GetBrushAreaPreviewTargetRevision(HitComponent) != CachedBrushAreaPreviewTargetRevision) return false;

	if (!FMath::IsNearlyEqual(BrushSize, CachedBrushAreaPreviewBrushSize) ||
		!FMath::IsNearlyEqual(BrushAreaPreviewBrushSizeScale, CachedBrushAreaPreviewBrushSizeScale) ||
		!FMath::IsNearlyEqual(PreviewLineThickness, CachedBrushAreaPreviewLineThickness) ||
		!PreviewColor.Equals(CachedBrushAreaPreviewColor))
	{
		return false;
	}

	return true;
}

void UPaintingModeControllerComponent::StoreBrushAreaPreviewCache(
	APlayerController* PlayerController, const FVector2D& MousePosition,
	const FVector& TraceStart, const FVector& TraceEnd, const FHitResult& TraceHit,
	URuntimeMeshPaintTargetComponent* PaintTarget, const FRuntimeMeshPaintSampleResult& PaintHit,
	const FLinearColor& PreviewColor, float PreviewLineThickness)
{
	UPrimitiveComponent* HitComponent = PaintHit.HitResult.GetComponent();
	if (!PlayerController || !PaintTarget || !HitComponent || !PaintHit.bSuccess)
	{
		InvalidateBrushAreaPreviewCache();
		return;
	}

	bBrushAreaPreviewCacheValid = true;
	CachedBrushAreaPreviewMousePosition = MousePosition;
	CachedBrushAreaPreviewTraceStart = TraceStart;
	CachedBrushAreaPreviewTraceEnd = TraceEnd;
	CachedBrushAreaPreviewPlayerController = PlayerController;
	CachedBrushAreaPreviewPaintTarget = PaintTarget;
	CachedBrushAreaPreviewHitComponent = HitComponent;
	CachedBrushAreaPreviewTargetTransform = HitComponent->GetComponentTransform();
	CachedBrushAreaPreviewTargetRevision = GetBrushAreaPreviewTargetRevision(HitComponent);
	CachedBrushAreaPreviewBrushSize = BrushSize;
	CachedBrushAreaPreviewBrushSizeScale = BrushAreaPreviewBrushSizeScale;
	CachedBrushAreaPreviewColor = PreviewColor;
	CachedBrushAreaPreviewLineThickness = PreviewLineThickness;
	CachedBrushAreaPreviewTraceChannel = PaintTraceChannel;
	bCachedBrushAreaPreviewTraceComplex = bPaintTraceComplex;
	CachedBrushAreaPreviewTraceHit = TraceHit;
	CachedBrushAreaPreviewHit = PaintHit;
}

void UPaintingModeControllerComponent::ServerSubmitPaintCommandBatch_Implementation(
	const FRuntimeMeshPaintNetCommandBatch& CommandBatch)
{
	AController* InstigatorController = ResolveOwnerController();
	for (const FRuntimeMeshPaintNetCommand& Command : CommandBatch.Commands)
	{
		URuntimeMeshPaintTargetComponent* PaintTarget =
			Cast<URuntimeMeshPaintTargetComponent>(Command.TargetComponent.Get());
		if (PaintTarget)
		{
			PaintTarget->AcceptReplicatedPaintCommand(Command, InstigatorController, true);
		}
	}
}

void UPaintingModeControllerComponent::ServerSubmitPaintCommandBatchReliable_Implementation(
	const FRuntimeMeshPaintNetCommandBatch& CommandBatch)
{
	AController* InstigatorController = ResolveOwnerController();
	for (const FRuntimeMeshPaintNetCommand& Command : CommandBatch.Commands)
	{
		URuntimeMeshPaintTargetComponent* PaintTarget =
			Cast<URuntimeMeshPaintTargetComponent>(Command.TargetComponent.Get());
		if (PaintTarget)
		{
			PaintTarget->AcceptReplicatedPaintCommand(Command, InstigatorController, true);
		}
	}
}

bool UPaintingModeControllerComponent::HasPaintTargetComponentFilter() const
{
	return PaintTargetComponents.Num() > 0;
}

URuntimeMeshPaintTargetComponent* UPaintingModeControllerComponent::GetFirstAllowedPaintTargetComponent() const
{
	for (const TObjectPtr<URuntimeMeshPaintTargetComponent>& ConfiguredPaintTargetComponent : PaintTargetComponents)
	{
		if (IsValid(ConfiguredPaintTargetComponent.Get())) return ConfiguredPaintTargetComponent.Get();
	}

	return nullptr;
}

URuntimeMeshPaintTargetComponent* UPaintingModeControllerComponent::GetPrimaryPaintTargetComponent() const
{
	if (URuntimeMeshPaintTargetComponent* ConfiguredPaintTargetComponent = GetFirstAllowedPaintTargetComponent())
	{
		return ConfiguredPaintTargetComponent;
	}

	return IsValid(PaintTargetComponent.Get()) ? PaintTargetComponent.Get() : nullptr;
}

URuntimeMeshPaintTargetComponent* UPaintingModeControllerComponent::ResolvePaintTargetFromHit(const FHitResult& HitResult) const
{
	if (!HitResult.bBlockingHit) return nullptr;

	if (HasPaintTargetComponentFilter())
	{
		for (const TObjectPtr<URuntimeMeshPaintTargetComponent>& ConfiguredPaintTargetComponent : PaintTargetComponents)
		{
			URuntimeMeshPaintTargetComponent* PaintTarget = ConfiguredPaintTargetComponent.Get();
			if (IsValid(PaintTarget) && PaintTarget->IsPaintableHit(HitResult)) return PaintTarget;
		}

		return nullptr;
	}

	if (IsValid(PaintTargetComponent.Get()) && PaintTargetComponent->IsPaintableHit(HitResult))
	{
		return PaintTargetComponent.Get();
	}

	AActor* HitActor = HitResult.GetActor();
	if (!HitActor) return nullptr;

	if (CachedHitPaintTargetActor.Get() == HitActor)
	{
		URuntimeMeshPaintTargetComponent* CachedPaintTarget = CachedHitPaintTarget.Get();
		if (IsValid(CachedPaintTarget) && CachedPaintTarget->IsPaintableHit(HitResult))
		{
			return CachedPaintTarget;
		}
	}

	TArray<URuntimeMeshPaintTargetComponent*> ActorPaintTargets;
	HitActor->GetComponents(ActorPaintTargets);
	for (URuntimeMeshPaintTargetComponent* ActorPaintTarget : ActorPaintTargets)
	{
		if (IsValid(ActorPaintTarget) && ActorPaintTarget->IsPaintableHit(HitResult))
		{
			CachedHitPaintTargetActor = HitActor;
			CachedHitPaintTarget = ActorPaintTarget;
			return ActorPaintTarget;
		}
	}

	CachedHitPaintTargetActor = HitActor;
	CachedHitPaintTarget = nullptr;
	return nullptr;
}

void UPaintingModeControllerComponent::HandleOwnerControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	const bool bWasPaintingModeActive = bIsPaintingModeActive;
	ExitPaintingMode();
	UnbindInput();

	if (BindInput() && bWasPaintingModeActive) EnterPaintingMode();
}

void UPaintingModeControllerComponent::HandleMoveInput(const FInputActionValue& Value)
{
	if (!bIsPaintingModeActive || !UsesCharacterControls()) return;

	MoveWithCamera(Value.Get<FVector2D>());
}

void UPaintingModeControllerComponent::HandlePaintStarted(const FInputActionValue& Value)
{
	const bool bCameraBlocksPainting = bIsPanningCamera || (UsesCameraControls() && bIsOrbiting);
	if (!bIsPaintingModeActive || bIsAdjustingBrushSize || bCameraBlocksPainting || bIsPainting) return;
	if (IsCursorOverPaintingUI() || (ColorPickerWidget && ColorPickerWidget->IsEyedropperActive())) return;

	++CurrentPaintStrokeId;
	if (CurrentPaintStrokeId == 0) ++CurrentPaintStrokeId;
	bNextPaintCommandReliable = true;
	bColorPickerNotifiedForCurrentStroke = false;
	CurrentStrokeReliableCommands.Reset();
	bIsPainting = true;
	OnPaintPressed.Broadcast();
	OnPaintTriggered.Broadcast();
	ApplyPaint();
}

void UPaintingModeControllerComponent::HandlePaintTriggered(const FInputActionValue& Value)
{
	const bool bCameraBlocksPainting = bIsPanningCamera || (UsesCameraControls() && bIsOrbiting);
	if (!bIsPaintingModeActive || !bIsPainting || bIsAdjustingBrushSize || bCameraBlocksPainting) return;

	OnPaintTriggered.Broadcast();
	ApplyPaint();
}

void UPaintingModeControllerComponent::HandlePaintCompleted(const FInputActionValue& Value)
{
	StopPainting();
}

void UPaintingModeControllerComponent::HandleOrbitStarted(const FInputActionValue& Value)
{
	if (!bIsPaintingModeActive || !UsesCameraControls() || bIsAdjustingBrushSize || bIsOrbiting || bIsPanningCamera) return;

	if (UsesFullCameraControls() && IsShiftKeyDown())
	{
		BeginCameraPan();
		return;
	}

	StopPainting();
	if (!CanApplyOrbitCamera()) return;

	BeginOrbitCamera();
	bIsOrbiting = true;
	OnOrbitStarted.Broadcast();
}

void UPaintingModeControllerComponent::HandleOrbitCompleted(const FInputActionValue& Value)
{
	StopOrbiting();
	StopCameraPan();
}

void UPaintingModeControllerComponent::HandlePanCameraStarted(const FInputActionValue& Value)
{
	if (!bIsPaintingModeActive || !UsesFullCameraControls() || bIsAdjustingBrushSize || bIsPanningCamera || !IsShiftKeyDown()) return;

	BeginCameraPan();
}

void UPaintingModeControllerComponent::HandlePanCameraCompleted(const FInputActionValue& Value)
{
	StopCameraPan();
}

void UPaintingModeControllerComponent::HandleBrushSizeStarted(const FInputActionValue& Value)
{
	if (!bIsPaintingModeActive) return;

	if (bBrushErase)
	{
		StopOrbiting();
		StopCameraPan();
		StopPainting();
		StopBrushSizeAdjustment();
		CancelEraserMode();
		return;
	}

	if (bIsAdjustingBrushSize) return;

	StopOrbiting();
	StopCameraPan();
	StopPainting();
	bIsAdjustingBrushSize = true;
}

void UPaintingModeControllerComponent::HandleBrushSizeCompleted(const FInputActionValue& Value)
{
	StopBrushSizeAdjustment();
}

void UPaintingModeControllerComponent::HandleMouseDelta(const FInputActionValue& Value)
{
	if (!bIsPaintingModeActive) return;

	const FVector2D MouseDelta = Value.Get<FVector2D>();
	if (bIsPanningCamera)
	{
		if (!IsShiftKeyDown())
		{
			StopCameraPan();
			return;
		}

		PanCamera(MouseDelta);
		return;
	}

	if (bIsAdjustingBrushSize)
	{
		AdjustBrushSize(MouseDelta);
		return;
	}

	if (bIsOrbiting && UsesCameraControls())
	{
		if (!CanApplyOrbitCamera())
		{
			StopOrbiting();
			return;
		}

		OrbitCamera(MouseDelta);
	}
}

void UPaintingModeControllerComponent::HandlePickColor(const FInputActionValue& Value)
{
	if (!bIsPaintingModeActive || !UsesPaintingInputControls()) return;

	if ((ColorPickerWidget || TryCreateColorPickerWidget()) && ColorPickerWidget)
	{
		ColorPickerWidget->EyedropperSampleMode = ColorPickerSampleMode;
		ColorPickerWidget->SampleColorUnderCursor();
		return;
	}

	PickColorUnderCursor();
}

void UPaintingModeControllerComponent::HandleCameraZoom(const FInputActionValue& Value)
{
	if (!bIsPaintingModeActive || !UsesFullCameraControls() || IsControlKeyDown()) return;
	if (IsCursorOverPaintingUI() || (ColorPickerWidget && ColorPickerWidget->IsEyedropperActive())) return;

	ZoomCamera(Value.Get<float>());
}

void UPaintingModeControllerComponent::HandleBrushSizeWheel(const FInputActionValue& Value)
{
	if (!bIsPaintingModeActive || !UsesPaintingInputControls() || !IsControlKeyDown()) return;
	if (IsCursorOverPaintingUI() || (ColorPickerWidget && ColorPickerWidget->IsEyedropperActive())) return;

	AdjustBrushSizeFromWheel(Value.Get<float>());
}

void UPaintingModeControllerComponent::HandleTogglePaintingMode(const FInputActionValue& Value)
{
	if (!UsesPaintingInputControls()) return;

	if (bIsPaintingModeActive) ExitPaintingMode();
	else EnterPaintingMode();
}
