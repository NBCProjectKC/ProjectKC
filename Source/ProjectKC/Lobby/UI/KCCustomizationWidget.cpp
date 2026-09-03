#include "Lobby/UI/KCCustomizationWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Lobby/KCLobbyPlayerController.h"
#include "Lobby/UI/KCLobbyWidget.h"
#include "Painting/PaintingModeControllerComponent.h"

#define LOCTEXT_NAMESPACE "KCCustomizationWidget"

namespace
{
	FText GetFailureText(const EKCCustomizationSaveResult Result)
	{
		switch (Result)
		{
		case EKCCustomizationSaveResult::NoSaveFound:
			return LOCTEXT("NoSaveFound", "저장된 커스터마이징이 없습니다.");
		case EKCCustomizationSaveResult::InvalidPaintTarget:
			return LOCTEXT("InvalidPaintTarget", "편집할 캐릭터를 찾지 못했습니다.");
		case EKCCustomizationSaveResult::NoPaintData:
			return LOCTEXT("NoPaintData", "저장할 페인트 데이터가 없습니다.");
		case EKCCustomizationSaveResult::SaveFailed:
			return LOCTEXT("SaveFailed", "커스터마이징 저장에 실패했습니다.");
		case EKCCustomizationSaveResult::LoadFailed:
			return LOCTEXT("LoadFailed", "커스터마이징 불러오기에 실패했습니다.");
		case EKCCustomizationSaveResult::IncompatibleVersion:
			return LOCTEXT("IncompatibleVersion", "저장 데이터 버전이 호환되지 않습니다.");
		case EKCCustomizationSaveResult::ApplyFailed:
			return LOCTEXT("ApplyFailed", "캐릭터 외형 적용에 실패했습니다.");
		default:
			return LOCTEXT("UnknownFailure", "커스터마이징 작업에 실패했습니다.");
		}
	}
}

void UKCCustomizationWidget::InitializeLobbyWidget(
	UKCLobbyWidget* InLobbyWidget)
{
	OwningLobbyWidget = InLobbyWidget;
}

void UKCCustomizationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bClosing = false;
	bOperationInProgress = false;
	LobbyPlayerController =
		Cast<AKCLobbyPlayerController>(GetOwningPlayer());
	PaintingController = LobbyPlayerController
		? LobbyPlayerController->GetCustomizationPaintingController()
		: nullptr;

	BindControls();
	SetOperationControlsEnabled(true);
	if (Text_Result)
	{
		Text_Result->SetText(FText::GetEmpty());
	}

	if (Slider_BrushSize && PaintingController)
	{
		const float MinimumBrushSize = FMath::Min(
			PaintingController->MinBrushSize,
			PaintingController->MaxBrushSize);
		const float MaximumBrushSize = FMath::Max(
			PaintingController->MinBrushSize,
			PaintingController->MaxBrushSize);
		const float NormalizedBrushSize = MaximumBrushSize > MinimumBrushSize
			? FMath::GetMappedRangeValueClamped(
				FVector2D(MinimumBrushSize, MaximumBrushSize),
				FVector2D(0.0f, 1.0f),
				PaintingController->BrushSize)
			: 0.0f;
		Slider_BrushSize->SetMinValue(0.0f);
		Slider_BrushSize->SetMaxValue(1.0f);
		Slider_BrushSize->SetValue(NormalizedBrushSize);
	}

	if (CheckBox_Eraser && PaintingController)
	{
		CheckBox_Eraser->SetIsChecked(PaintingController->bBrushErase);
	}
}

void UKCCustomizationWidget::NativeDestruct()
{
	UnbindControls();
	if (!bClosing && LobbyPlayerController &&
		LobbyPlayerController->IsCustomizationEditing())
	{
		LobbyPlayerController->CancelCustomizationEditing();
	}
	if (OwningLobbyWidget)
	{
		OwningLobbyWidget->NotifyCustomizationWidgetClosed(this);
	}

	PaintingController = nullptr;
	LobbyPlayerController = nullptr;
	OwningLobbyWidget = nullptr;
	Super::NativeDestruct();
}

void UKCCustomizationWidget::SetBrushColor(const FLinearColor NewColor)
{
	if (!PaintingController)
	{
		return;
	}

	PaintingController->BrushColor = NewColor;
	SetEraserEnabled(false);
}

void UKCCustomizationWidget::SetEraserEnabled(const bool bEnabled)
{
	if (PaintingController)
	{
		PaintingController->bBrushErase = bEnabled;
	}
	if (CheckBox_Eraser && CheckBox_Eraser->IsChecked() != bEnabled)
	{
		CheckBox_Eraser->SetIsChecked(bEnabled);
	}
}

void UKCCustomizationWidget::RequestCancelAndClose()
{
	if (bClosing)
	{
		return;
	}

	if (LobbyPlayerController &&
		LobbyPlayerController->IsCustomizationEditing())
	{
		LobbyPlayerController->CancelCustomizationEditing();
	}
	CloseWidget();
}

void UKCCustomizationWidget::HandleSaveClicked()
{
	if (bOperationInProgress || !LobbyPlayerController)
	{
		return;
	}

	bOperationInProgress = true;
	SetOperationControlsEnabled(false);
	if (LobbyPlayerController->SaveCustomizationEditing())
	{
		CloseWidget();
		return;
	}

	ReportOperationFailure(
		LobbyPlayerController->LastCustomizationEditingResult);
	bOperationInProgress = false;
	if (LobbyPlayerController->IsCustomizationEditing())
	{
		SetOperationControlsEnabled(true);
	}
	else
	{
		CloseWidget();
	}
}

void UKCCustomizationWidget::HandleCancelClicked()
{
	if (bOperationInProgress || !LobbyPlayerController)
	{
		return;
	}

	bOperationInProgress = true;
	SetOperationControlsEnabled(false);
	if (!LobbyPlayerController->CancelCustomizationEditing())
	{
		ReportOperationFailure(
			LobbyPlayerController->LastCustomizationEditingResult);
	}
	CloseWidget();
}

void UKCCustomizationWidget::HandleResetClicked()
{
	if (bOperationInProgress || !LobbyPlayerController)
	{
		return;
	}

	bOperationInProgress = true;
	SetOperationControlsEnabled(false);
	if (!LobbyPlayerController->ResetCustomizationEditing())
	{
		ReportOperationFailure(
			LobbyPlayerController->LastCustomizationEditingResult);
	}
	bOperationInProgress = false;
	if (LobbyPlayerController->IsCustomizationEditing())
	{
		SetOperationControlsEnabled(true);
	}
	else
	{
		CloseWidget();
	}
}

void UKCCustomizationWidget::HandleResetCameraClicked()
{
	if (LobbyPlayerController)
	{
		LobbyPlayerController->ResetCustomizationCamera();
	}
}

void UKCCustomizationWidget::HandleBrushSizeChanged(
	const float NormalizedValue)
{
	if (!PaintingController)
	{
		return;
	}

	const float MinimumBrushSize = FMath::Min(
		PaintingController->MinBrushSize,
		PaintingController->MaxBrushSize);
	const float MaximumBrushSize = FMath::Max(
		PaintingController->MinBrushSize,
		PaintingController->MaxBrushSize);
	PaintingController->SetBrushSize(FMath::Lerp(
		MinimumBrushSize,
		MaximumBrushSize,
		FMath::Clamp(NormalizedValue, 0.0f, 1.0f)));
}

void UKCCustomizationWidget::HandleEraserChanged(const bool bIsChecked)
{
	if (PaintingController)
	{
		PaintingController->bBrushErase = bIsChecked;
	}
}

void UKCCustomizationWidget::HandleBlackClicked()
{
	SetBrushColor(FLinearColor::Black);
}

void UKCCustomizationWidget::HandleWhiteClicked()
{
	SetBrushColor(FLinearColor::White);
}

void UKCCustomizationWidget::HandleRedClicked()
{
	SetBrushColor(FLinearColor::Red);
}

void UKCCustomizationWidget::HandleBlueClicked()
{
	SetBrushColor(FLinearColor::Blue);
}

void UKCCustomizationWidget::HandleGreenClicked()
{
	SetBrushColor(FLinearColor::Green);
}

void UKCCustomizationWidget::HandleYellowClicked()
{
	SetBrushColor(FLinearColor::Yellow);
}

void UKCCustomizationWidget::BindControls()
{
	if (Button_Save)
	{
		Button_Save->OnClicked.AddUniqueDynamic(
			this, &UKCCustomizationWidget::HandleSaveClicked);
	}
	if (Button_Cancel)
	{
		Button_Cancel->OnClicked.AddUniqueDynamic(
			this, &UKCCustomizationWidget::HandleCancelClicked);
	}
	if (Button_Reset)
	{
		Button_Reset->OnClicked.AddUniqueDynamic(
			this, &UKCCustomizationWidget::HandleResetClicked);
	}
	if (Button_ResetCamera)
	{
		Button_ResetCamera->OnClicked.AddUniqueDynamic(
			this, &UKCCustomizationWidget::HandleResetCameraClicked);
	}
	if (Slider_BrushSize)
	{
		Slider_BrushSize->OnValueChanged.AddUniqueDynamic(
			this, &UKCCustomizationWidget::HandleBrushSizeChanged);
	}
	if (CheckBox_Eraser)
	{
		CheckBox_Eraser->OnCheckStateChanged.AddUniqueDynamic(
			this, &UKCCustomizationWidget::HandleEraserChanged);
	}
	if (Button_ColorBlack)
	{
		Button_ColorBlack->OnClicked.AddUniqueDynamic(
			this, &UKCCustomizationWidget::HandleBlackClicked);
	}
	if (Button_ColorWhite)
	{
		Button_ColorWhite->OnClicked.AddUniqueDynamic(
			this, &UKCCustomizationWidget::HandleWhiteClicked);
	}
	if (Button_ColorRed)
	{
		Button_ColorRed->OnClicked.AddUniqueDynamic(
			this, &UKCCustomizationWidget::HandleRedClicked);
	}
	if (Button_ColorBlue)
	{
		Button_ColorBlue->OnClicked.AddUniqueDynamic(
			this, &UKCCustomizationWidget::HandleBlueClicked);
	}
	if (Button_ColorGreen)
	{
		Button_ColorGreen->OnClicked.AddUniqueDynamic(
			this, &UKCCustomizationWidget::HandleGreenClicked);
	}
	if (Button_ColorYellow)
	{
		Button_ColorYellow->OnClicked.AddUniqueDynamic(
			this, &UKCCustomizationWidget::HandleYellowClicked);
	}
}

void UKCCustomizationWidget::UnbindControls()
{
	if (Button_Save)
	{
		Button_Save->OnClicked.RemoveDynamic(
			this, &UKCCustomizationWidget::HandleSaveClicked);
	}
	if (Button_Cancel)
	{
		Button_Cancel->OnClicked.RemoveDynamic(
			this, &UKCCustomizationWidget::HandleCancelClicked);
	}
	if (Button_Reset)
	{
		Button_Reset->OnClicked.RemoveDynamic(
			this, &UKCCustomizationWidget::HandleResetClicked);
	}
	if (Button_ResetCamera)
	{
		Button_ResetCamera->OnClicked.RemoveDynamic(
			this, &UKCCustomizationWidget::HandleResetCameraClicked);
	}
	if (Slider_BrushSize)
	{
		Slider_BrushSize->OnValueChanged.RemoveDynamic(
			this, &UKCCustomizationWidget::HandleBrushSizeChanged);
	}
	if (CheckBox_Eraser)
	{
		CheckBox_Eraser->OnCheckStateChanged.RemoveDynamic(
			this, &UKCCustomizationWidget::HandleEraserChanged);
	}
	if (Button_ColorBlack)
	{
		Button_ColorBlack->OnClicked.RemoveDynamic(
			this, &UKCCustomizationWidget::HandleBlackClicked);
	}
	if (Button_ColorWhite)
	{
		Button_ColorWhite->OnClicked.RemoveDynamic(
			this, &UKCCustomizationWidget::HandleWhiteClicked);
	}
	if (Button_ColorRed)
	{
		Button_ColorRed->OnClicked.RemoveDynamic(
			this, &UKCCustomizationWidget::HandleRedClicked);
	}
	if (Button_ColorBlue)
	{
		Button_ColorBlue->OnClicked.RemoveDynamic(
			this, &UKCCustomizationWidget::HandleBlueClicked);
	}
	if (Button_ColorGreen)
	{
		Button_ColorGreen->OnClicked.RemoveDynamic(
			this, &UKCCustomizationWidget::HandleGreenClicked);
	}
	if (Button_ColorYellow)
	{
		Button_ColorYellow->OnClicked.RemoveDynamic(
			this, &UKCCustomizationWidget::HandleYellowClicked);
	}
}

void UKCCustomizationWidget::SetOperationControlsEnabled(
	const bool bEnabled)
{
	if (Button_Save)
	{
		Button_Save->SetIsEnabled(bEnabled);
	}
	if (Button_Cancel)
	{
		Button_Cancel->SetIsEnabled(bEnabled);
	}
	if (Button_Reset)
	{
		Button_Reset->SetIsEnabled(bEnabled);
	}
}

void UKCCustomizationWidget::ReportOperationFailure(
	const EKCCustomizationSaveResult Result)
{
	if (Text_Result)
	{
		Text_Result->SetText(GetFailureText(Result));
	}
	OnCustomizationOperationFailed(Result);
}

void UKCCustomizationWidget::CloseWidget()
{
	if (bClosing)
	{
		return;
	}

	bClosing = true;
	if (OwningLobbyWidget)
	{
		OwningLobbyWidget->NotifyCustomizationWidgetClosed(this);
	}
	RemoveFromParent();
}

#undef LOCTEXT_NAMESPACE
