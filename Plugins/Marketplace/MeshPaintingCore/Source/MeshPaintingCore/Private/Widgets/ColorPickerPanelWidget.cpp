// Copyright Shared Orbit 2026. All Rights Reserved.
#include "Widgets/ColorPickerPanelWidget.h"

#include "ColorPickerPanelWidgetPrivate.h"
#include "ColorPicker/ColorPickerPalette.h"
#include "ColorPicker/ColorPickerState.h"
#include "Components/Border.h"
#include "Framework/Application/SlateApplication.h"
#include "Painting/PaintingModeControllerComponent.h"
#include "Painting/RuntimeMeshPaintTargetComponent.h"
#include "Widgets/ColorHistoryBarWidget.h"
#include "Widgets/ColorPickerToolStripWidget.h"
#include "Widgets/ColorPreviewWidget.h"

UColorPickerPanelWidget::UColorPickerPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CurrentColor(FLinearColor::White)
	, PreviousColor(FLinearColor::Black)
	, BrushSize(DefaultNormalizedBrushSize)
	, Metallic(0.0f)
	, Roughness(0.5f)
	, bPushSettingsToPaintTarget(true)
	, PaletteSaveSlotName(TEXT("MeshPaintingCore_ColorPalette"))
	, EyedropperTraceChannel(ECC_Visibility)
	, bEyedropperTraceComplex(true)
	, EyedropperSampleMode(ERuntimeMeshPaintColorSampleMode::MeshUnlitColor)
	, bBuildNativeLayoutIfMissing(true)
	, bRecentColorsExpanded(true)
	, PanelPadding(10.0f)
	, BoundRuntimePaintTarget(nullptr)
	, BoundPaintingController(nullptr)
	, PreviousMouseCursor(EMouseCursor::Default)
	, bIsSynchronizing(false)
	, bNativeTreeBuilt(false)
	, bUsingNativeFallbackLayout(false)
	, bIsEyedropperActive(false)
	, bIsContinuousInteraction(false)
{
}

bool UColorPickerPanelWidget::IsCursorOverPanel() const
{
	if (!RootBorder) return false;
	if (!FSlateApplication::IsInitialized()) return RootBorder->IsHovered();

	const FGeometry& Geometry = RootBorder->GetCachedGeometry();
	const FVector2D LocalSize = Geometry.GetLocalSize();
	if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f) return RootBorder->IsHovered();

	FSlateApplication& SlateApplication = FSlateApplication::Get();
	const FVector2D CursorPosition = SlateApplication.GetCursorPos();
	if (Geometry.IsUnderLocation(CursorPosition)) return true;

	if (!SlateApplication.AnyMenusVisible()) return false;

	const FWidgetPath WidgetsUnderCursor = SlateApplication.LocateWindowUnderMouse(
		CursorPosition,
		SlateApplication.GetInteractiveTopLevelWindows(),
		true);
	return WidgetsUnderCursor.IsValid() && SlateApplication.FindMenuInWidgetPath(WidgetsUnderCursor).IsValid();
}

void UColorPickerPanelWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	NotifyPaintingUIPointerState(true);
}

void UColorPickerPanelWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	NotifyPaintingUIPointerState(IsCursorOverPanel());
}

void UColorPickerPanelWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
	NotifyPaintingUIPointerState(IsCursorOverPanel());
}

FCursorReply UColorPickerPanelWidget::NativeOnCursorQuery(
	const FGeometry& InGeometry, const FPointerEvent& InCursorEvent)
{
	if (bIsEyedropperActive) return FCursorReply::Cursor(EMouseCursor::EyeDropper);
	if (IsCursorOverPanel()) return Super::NativeOnCursorQuery(InGeometry, InCursorEvent);

	if (UPaintingModeControllerComponent* PaintingController = Cast<UPaintingModeControllerComponent>(PaintTarget.GetObject()))
		return FCursorReply::Cursor(PaintingController->GetPaintingMouseCursor());

	return FCursorReply::Unhandled();
}

void UColorPickerPanelWidget::NotifyPaintingUIPointerState(bool)
{
	if (UPaintingModeControllerComponent* PaintingController = Cast<UPaintingModeControllerComponent>(PaintTarget.GetObject()))
	{
		PaintingController->RefreshPaintingMouseCursor();
	}
}

void UColorPickerPanelWidget::SetCurrentColor(FLinearColor NewColor, bool bBroadcast)
{
	NewColor = ClampColor01(NewColor);
	if (CurrentColor.Equals(NewColor, KINDA_SMALL_NUMBER))
	{
		if (bBroadcast && IsEraserActive()) SetEraserActive(false, true);
		SynchronizeColorWidgets();
		return;
	}

	const bool bCapturePrevious = !bIsSynchronizing && !bIsContinuousInteraction;
	if (bCapturePrevious) PreviousColor = CurrentColor;

	CurrentColor = NewColor;
	EnsureRuntimeObjects();
	const bool bWasEraserActive = ColorState && ColorState->IsEraserActive();
	if (ColorState) ColorState->SetCurrentColor(CurrentColor, bCapturePrevious, false);
	if (bWasEraserActive && ColorState && !ColorState->IsEraserActive())
	{
		SetEraserActiveVisual(false);
		if (bBroadcast) OnToolToggled.Broadcast(ToolEraser, false);
	}

	SynchronizeColorWidgets();

	if (bBroadcast)
	{
		OnColorChanged.Broadcast(CurrentColor);
		BroadcastBrushSettings();
	}
}

void UColorPickerPanelWidget::CommitCurrentColor()
{
	OnColorCommitted.Broadcast(CurrentColor);
}

void UColorPickerPanelWidget::SetPreviousColor(FLinearColor NewColor)
{
	PreviousColor = ClampColor01(NewColor);
	if (ColorState) ColorState->PreviousLinearColor = PreviousColor;
	if (PreviousPreview) PreviousPreview->SetColor(PreviousColor);
}

void UColorPickerPanelWidget::SetBrushSize(float NewBrushSize, bool bBroadcast)
{
	SetScalarValue(ChannelBrushSize, NewBrushSize, MinNormalizedBrushSize, MaxNormalizedBrushSize, BrushSize, BrushSizeRow, bBroadcast);
}

void UColorPickerPanelWidget::SetMetallic(float NewMetallic, bool bBroadcast)
{
	SetScalarValue(ChannelMetallic, NewMetallic, 0.0f, 1.0f, Metallic, MetallicRow, bBroadcast);
}

void UColorPickerPanelWidget::SetRoughness(float NewRoughness, bool bBroadcast)
{
	SetScalarValue(ChannelRoughness, NewRoughness, 0.0f, 1.0f, Roughness, RoughnessRow, bBroadcast);
}

void UColorPickerPanelWidget::CommitScalar(FName ScalarId)
{
	float Value = 0.0f;
	if (GetScalarChannelValue(ScalarId, Value)) OnScalarCommitted.Broadcast(ScalarId, Value);
}

void UColorPickerPanelWidget::SetPaintTarget(UObject* NewPaintTarget)
{
	UnbindRuntimePaintTarget();
	UnbindPaintingController();

	if (ImplementsPaintTarget(NewPaintTarget))
	{
		PaintTarget.SetObject(NewPaintTarget);
		PaintTarget.SetInterface(Cast<IRuntimeMeshPaintTargetInterface>(NewPaintTarget));
		BindRuntimePaintTarget(Cast<URuntimeMeshPaintTargetComponent>(NewPaintTarget));
		BindPaintingController(Cast<UPaintingModeControllerComponent>(NewPaintTarget));

		if (UPaintingModeControllerComponent* PaintingController = Cast<UPaintingModeControllerComponent>(NewPaintTarget))
		{
			const FMeshPaintBrushMaterialSettings Settings = PaintingController->GetBrushMaterialSettings();
			EyedropperSampleMode = PaintingController->ColorPickerSampleMode;
			SetCurrentColor(Settings.Color, false);
			SetBrushSize(GetNormalizedBrushSizeFromRadius(Settings.BrushSize), false);
			SetMetallic(Settings.Metallic, false);
			SetRoughness(Settings.Roughness, false);
			SetEraserActive(Settings.bErase, false);
		}
		else
		{
			ApplyBrushSettingsToPaintTarget();
		}
	}
	else
	{
		PaintTarget.SetObject(nullptr);
		PaintTarget.SetInterface(nullptr);
	}
}

void UColorPickerPanelWidget::ApplyBrushSettingsToPaintTarget()
{
	UObject* PaintTargetObject = PaintTarget.GetObject();
	if (!ImplementsPaintTarget(PaintTargetObject)) return;

	IRuntimeMeshPaintTargetInterface::Execute_ApplyBrushMaterialSettings(
		PaintTargetObject,
		ColorState ? ColorState->GetBrushMaterialSettings() : FMeshPaintBrushMaterialSettings());
}

void UColorPickerPanelWidget::AddCurrentColorToCustomPalette()
{
	EnsureRuntimeObjects();
	if (PaletteStorage) PaletteStorage->AddCustomColor(CurrentColor);
}

void UColorPickerPanelWidget::RemoveCustomPaletteColorAt(int32 CustomColorIndex)
{
	EnsureRuntimeObjects();
	if (PaletteStorage) PaletteStorage->RemoveCustomColorAt(CustomColorIndex);
}

void UColorPickerPanelWidget::RestoreDefaultPalette()
{
	EnsureRuntimeObjects();
	if (PaletteStorage) PaletteStorage->RestoreDefaultPalette();
}

void UColorPickerPanelWidget::RecordCurrentColorAsRecent()
{
	EnsureRuntimeObjects();
	if (PaletteStorage) PaletteStorage->AddRecentColor(CurrentColor);
}

void UColorPickerPanelWidget::NotifyPaintApplied()
{
	if (IsEraserActive()) return;

	RecordCurrentColorAsRecent();
}

void UColorPickerPanelWidget::SetRecentColorsExpanded(bool bExpanded)
{
	bRecentColorsExpanded = bExpanded;
	if (HistoryBar) HistoryBar->SetVisibility(bRecentColorsExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (ToolStrip) ToolStrip->SetThemePanelVisible(bRecentColorsExpanded);
}

void UColorPickerPanelWidget::SetEraserActive(bool bActive, bool bBroadcast)
{
	EnsureRuntimeObjects();
	if (!ColorState) return;

	const bool bWasActive = ColorState->IsEraserActive();
	ColorState->SetEraserActive(bActive, false);
	const bool bIsActive = ColorState->IsEraserActive();
	SetEraserActiveVisual(bIsActive);

	if (bBroadcast && bWasActive != bIsActive)
	{
		OnToolToggled.Broadcast(ToolEraser, bIsActive);
		BroadcastBrushSettings();
	}
}

bool UColorPickerPanelWidget::IsEraserActive() const
{
	return ColorState && ColorState->IsEraserActive();
}

TSharedRef<SWidget> UColorPickerPanelWidget::RebuildWidget()
{
	EnsureRuntimeObjects();
	BuildWidgetTree();
	ConfigureBoundWidgets();
	BindChildWidgetEvents();
	SynchronizeAllWidgets();
	return Super::RebuildWidget();
}

void UColorPickerPanelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureRuntimeObjects();
	ConfigureBoundWidgets();
	BindChildWidgetEvents();
	SynchronizeAllWidgets();
}

void UColorPickerPanelWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureRuntimeObjects();
	ConfigureBoundWidgets();
	BindChildWidgetEvents();
	SynchronizeAllWidgets();
}

void UColorPickerPanelWidget::NativeDestruct()
{
	CancelEyedropperMode();
	UnbindRuntimePaintTarget();
	UnbindPaintingController();
	Super::NativeDestruct();
}

void UColorPickerPanelWidget::EnsureRuntimeObjects()
{
	if (!ColorState)
	{
		ColorState = NewObject<UColorPickerState>(this);
		ColorState->CurrentLinearColor = CurrentColor;
		ColorState->PreviousLinearColor = PreviousColor;
		ColorState->BrushSize = GetBrushRadiusFromNormalizedSize(BrushSize);
		ColorState->Metallic = Metallic;
		ColorState->Roughness = Roughness;
	}

	if (!PaletteStorage)
	{
		PaletteStorage = NewObject<UColorPickerPaletteStorage>(this);
		PaletteStorage->MaxColors = 46;
		PaletteStorage->bUseFallbackColorsWhenEmpty = false;
		PaletteStorage->OnPaletteChanged.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandlePaletteChanged);
		PaletteStorage->Initialize(PaletteDataAsset, PaletteSaveSlotName);
	}

	BindRuntimePaintTarget(Cast<URuntimeMeshPaintTargetComponent>(PaintTarget.GetObject()));
	BindPaintingController(Cast<UPaintingModeControllerComponent>(PaintTarget.GetObject()));
}

void UColorPickerPanelWidget::BroadcastBrushSettings()
{
	EnsureRuntimeObjects();
	if (!ColorState) return;

	const FMeshPaintBrushMaterialSettings Settings = ColorState->GetBrushMaterialSettings();
	OnBrushSettingsChanged.Broadcast(Settings);

	if (bPushSettingsToPaintTarget) ApplyBrushSettingsToPaintTarget();
}

void UColorPickerPanelWidget::SetScalarValue(
	FName ScalarId, float NewValue, float MinValue, float MaxValue,
	float& StoredValue, UColorChannelRowWidget* Row, bool bBroadcast)
{
	const float ClampedValue = FMath::Clamp(NewValue, MinValue, MaxValue);
	const bool bChanged = !FMath::IsNearlyEqual(StoredValue, ClampedValue);
	StoredValue = ClampedValue;

	EnsureRuntimeObjects();
	if (ColorState)
	{
		if (ScalarId == ChannelBrushSize) ColorState->SetBrushSize(GetBrushRadiusFromNormalizedSize(BrushSize), false);
		else if (ScalarId == ChannelMetallic) ColorState->SetMetallic(Metallic, false);
		else if (ScalarId == ChannelRoughness) ColorState->SetRoughness(Roughness, false);
	}
	if (Row) Row->SetValue(StoredValue, false);

	if (bChanged && bBroadcast)
	{
		OnScalarChanged.Broadcast(ScalarId, StoredValue);
		BroadcastBrushSettings();
	}
}

float UColorPickerPanelWidget::GetMaxBrushSize() const
{
	if (const UPaintingModeControllerComponent* PaintingController = Cast<UPaintingModeControllerComponent>(PaintTarget.GetObject()))
		return FMath::Max(KINDA_SMALL_NUMBER, PaintingController->MaxBrushSize);

	const UPaintingModeControllerComponent* DefaultPaintingController = GetDefault<UPaintingModeControllerComponent>();
	return FMath::Max(KINDA_SMALL_NUMBER, DefaultPaintingController->MaxBrushSize);
}

float UColorPickerPanelWidget::GetBrushRadiusFromNormalizedSize(float NormalizedBrushSize) const
{
	const float SafeNormalizedBrushSize = FMath::Clamp(NormalizedBrushSize, MinNormalizedBrushSize, MaxNormalizedBrushSize);
	return SafeNormalizedBrushSize * GetMaxBrushSize();
}

float UColorPickerPanelWidget::GetNormalizedBrushSizeFromRadius(float BrushRadius) const
{
	return FMath::Clamp(BrushRadius / GetMaxBrushSize(), MinNormalizedBrushSize, MaxNormalizedBrushSize);
}

void UColorPickerPanelWidget::HandleRuntimePaintApplied(FRuntimeMeshPaintSampleResult PaintResult)
{
	if (!PaintResult.bSuccess) return;
	if (IsEraserActive()) return;

	EnsureRuntimeObjects();
	if (PaletteStorage) PaletteStorage->AddRecentColor(PaintResult.Color);
}

void UColorPickerPanelWidget::HandlePaintingControllerBrushSizeChanged(float NewSize)
{
	SetBrushSize(GetNormalizedBrushSizeFromRadius(NewSize), false);
}

void UColorPickerPanelWidget::BindRuntimePaintTarget(URuntimeMeshPaintTargetComponent* RuntimePaintTarget)
{
	if (BoundRuntimePaintTarget == RuntimePaintTarget) return;

	UnbindRuntimePaintTarget();
	if (!RuntimePaintTarget) return;

	BoundRuntimePaintTarget = RuntimePaintTarget;
	BoundRuntimePaintTarget->OnPaintApplied.AddUniqueDynamic(this, &UColorPickerPanelWidget::HandleRuntimePaintApplied);

	ConfigureChannelRow(
		BrushSizeRow, ChannelBrushSize, TEXT("Brush Size"),
		MinNormalizedBrushSize, MaxNormalizedBrushSize, 3,
		EColorBarGradientMode::TwoColor, bUsingNativeFallbackLayout);
	SetBrushSize(BrushSize, false);
}

void UColorPickerPanelWidget::UnbindRuntimePaintTarget()
{
	if (!BoundRuntimePaintTarget) return;

	BoundRuntimePaintTarget->OnPaintApplied.RemoveDynamic(this, &UColorPickerPanelWidget::HandleRuntimePaintApplied);
	BoundRuntimePaintTarget = nullptr;
}

void UColorPickerPanelWidget::BindPaintingController(UPaintingModeControllerComponent* PaintingController)
{
	if (BoundPaintingController == PaintingController) return;

	UnbindPaintingController();
	if (!PaintingController) return;

	BoundPaintingController = PaintingController;
	EyedropperSampleMode = BoundPaintingController->ColorPickerSampleMode;
	BoundPaintingController->OnBrushSizeChanged.AddUniqueDynamic(
		this, &UColorPickerPanelWidget::HandlePaintingControllerBrushSizeChanged);
	HandlePaintingControllerBrushSizeChanged(BoundPaintingController->BrushSize);
}

void UColorPickerPanelWidget::UnbindPaintingController()
{
	if (!BoundPaintingController) return;

	BoundPaintingController->OnBrushSizeChanged.RemoveDynamic(
		this, &UColorPickerPanelWidget::HandlePaintingControllerBrushSizeChanged);
	BoundPaintingController = nullptr;
}
