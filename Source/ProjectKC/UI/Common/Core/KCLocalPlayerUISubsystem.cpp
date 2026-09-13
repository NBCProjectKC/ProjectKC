#include "ProjectKC/UI/Common/Core/KCLocalPlayerUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/Common/Core/KCUISettings.h"
#include "ProjectKC/UI/Menu/Widget/KCEscMenuWidget.h"

UKCUserWidget* UKCLocalPlayerUISubsystem::SetScreenWidget(TSubclassOf<UKCUserWidget> WidgetClass)
{
	ClearScreenWidget();

	ULocalPlayer* OwnerLocalPlayer = GetLocalPlayer();
	if (!OwnerLocalPlayer || !WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC UI failed to set screen: LocalPlayer=%s, WidgetClass=%s."),
			OwnerLocalPlayer ? TEXT("Valid") : TEXT("Null"),
			*GetNameSafe(WidgetClass));
		return nullptr;
	}

	APlayerController* OwnerPlayerController = GetOwningPlayerController();
	if (!OwnerPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC UI failed to set screen: OwnerPlayerController is null."));
		return nullptr;
	}

	ActiveScreenWidget = CreateWidget<UKCUserWidget>(OwnerPlayerController, WidgetClass);
	if (ActiveScreenWidget)
	{
		const bool bAddedToScreen = ActiveScreenWidget->AddToPlayerScreen();

		UE_LOG(LogTemp, Log, TEXT("KC UI set screen: Widget=%s, Class=%s, AddedToScreen=%s."),
			*GetNameSafe(ActiveScreenWidget),
			*GetNameSafe(WidgetClass),
			bAddedToScreen ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("KC UI failed to set screen: CreateWidget failed for %s."),
			*GetNameSafe(WidgetClass));
	}

	return ActiveScreenWidget;
}

void UKCLocalPlayerUISubsystem::ClearScreenWidget()
{
	if (ActiveScreenWidget)
	{
		ActiveScreenWidget->RemoveFromParent();
		ActiveScreenWidget = nullptr;
	}
}

UKCUserWidget* UKCLocalPlayerUISubsystem::SetHUDWidget(TSubclassOf<UKCUserWidget> WidgetClass)
{
	return SetScreenWidget(WidgetClass);
}

void UKCLocalPlayerUISubsystem::ClearHUDWidget()
{
	ClearScreenWidget();
}

void UKCLocalPlayerUISubsystem::QueueToast(const FText& Message)
{
	PendingToastMessages.Add(Message);
}

UKCEscMenuWidget* UKCLocalPlayerUISubsystem::ShowEscMenu(const bool bRestoreGameOnlyWhenHidden)
{
	bRestoreGameOnlyOnEscMenuHidden = bRestoreGameOnlyWhenHidden;

	if (ActiveEscMenuWidget)
	{
		return ActiveEscMenuWidget;
	}

	APlayerController* OwnerPlayerController = GetOwningPlayerController();
	const UKCUISettings* UISettings = GetDefault<UKCUISettings>();
	const TSubclassOf<UKCEscMenuWidget> EscMenuClass = UISettings
		? UISettings->EscMenuWidgetClass.LoadSynchronous()
		: nullptr;
	if (!OwnerPlayerController || !EscMenuClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC UI failed to show ESC menu: PlayerController=%s, WidgetClass=%s."),
			OwnerPlayerController ? TEXT("Valid") : TEXT("Null"),
			*GetNameSafe(EscMenuClass));
		return nullptr;
	}

	ActiveEscMenuWidget = CreateWidget<UKCEscMenuWidget>(OwnerPlayerController, EscMenuClass);
	if (!ActiveEscMenuWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC UI failed to show ESC menu: CreateWidget failed for %s."),
			*GetNameSafe(EscMenuClass));
		return nullptr;
	}

	ActiveEscMenuWidget->AddToViewport(1000);

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	OwnerPlayerController->SetInputMode(InputMode);
	OwnerPlayerController->bShowMouseCursor = true;
	return ActiveEscMenuWidget;
}

void UKCLocalPlayerUISubsystem::HideEscMenu()
{
	if (ActiveEscMenuWidget)
	{
		ActiveEscMenuWidget->RemoveFromParent();
		ActiveEscMenuWidget = nullptr;
	}

	if (APlayerController* OwnerPlayerController = GetOwningPlayerController())
	{
		if (bRestoreGameOnlyOnEscMenuHidden)
		{
			FInputModeGameOnly InputMode;
			InputMode.SetConsumeCaptureMouseDown(false);
			OwnerPlayerController->SetInputMode(InputMode);
		}
		else
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			OwnerPlayerController->SetInputMode(InputMode);
			OwnerPlayerController->bShowMouseCursor = true;
		}
	}
}

void UKCLocalPlayerUISubsystem::ToggleEscMenu(const bool bRestoreGameOnlyWhenHidden)
{
	if (ActiveEscMenuWidget)
	{
		HideEscMenu();
		return;
	}

	ShowEscMenu(bRestoreGameOnlyWhenHidden);
}
void UKCLocalPlayerUISubsystem::Deinitialize()
{
	HideEscMenu();
	ClearScreenWidget();
	PendingToastMessages.Reset();

	Super::Deinitialize();
}

APlayerController* UKCLocalPlayerUISubsystem::GetOwningPlayerController() const
{
	const ULocalPlayer* OwnerLocalPlayer = GetLocalPlayer();
	return OwnerLocalPlayer ? OwnerLocalPlayer->GetPlayerController(GetWorld()) : nullptr;
}