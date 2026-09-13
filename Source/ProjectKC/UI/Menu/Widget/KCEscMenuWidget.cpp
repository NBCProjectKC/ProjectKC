#include "ProjectKC/UI/Menu/Widget/KCEscMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Lobby/KCSessionSubsystem.h"
#include "ProjectKC/UI/Common/Core/KCUISettings.h"
#include "ProjectKC/UI/Common/Core/KCLocalPlayerUISubsystem.h"
#include "Engine/LocalPlayer.h"
#include "ProjectKC/UI/Menu/Widget/KCToastButtonWidget.h"

namespace
{
	constexpr int32 ConfirmToastZOrder = 1200;
	const FName LocalizationTableId(TEXT("ST_LocalizationTable"));
	const FString MainMenuToastKey(TEXT("ESC.Mainmenu.Toast"));
	const FString ExitToastKey(TEXT("ESC.Exit.Toast"));
}

void UKCEscMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CloseButton = ResolveButton({TEXT("ReturnButton")});
	MainMenuButton = ResolveButton({TEXT("MainMenuButton"), TEXT("Button_49")});
	OptionButton = ResolveButton({TEXT("OptionButton"), TEXT("Button")});
	ExitButton = ResolveButton({TEXT("ExitButton"), TEXT("Button_1")});

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleMainMenuClicked);
	}
	if (OptionButton)
	{
		OptionButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleOptionClicked);
	}
	if (ExitButton)
	{
		ExitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleExitClicked);
	}
}

void UKCEscMenuWidget::NativeDestruct()
{
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveAll(this);
	}
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.RemoveAll(this);
	}
	if (OptionButton)
	{
		OptionButton->OnClicked.RemoveAll(this);
	}
	if (ExitButton)
	{
		ExitButton->OnClicked.RemoveAll(this);
	}
	if (ActiveConfirmToast)
	{
		ActiveConfirmToast->RemoveFromParent();
		ActiveConfirmToast = nullptr;
	}

	Super::NativeDestruct();
}

UButton* UKCEscMenuWidget::ResolveButton(const TArray<FName>& CandidateNames) const
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	for (const FName& CandidateName : CandidateNames)
	{
		if (UWidget* Widget = WidgetTree->FindWidget(CandidateName))
		{
			if (UButton* Button = Cast<UButton>(Widget))
			{
				return Button;
			}
		}
	}

	return nullptr;
}

void UKCEscMenuWidget::ShowConfirmToast(const FText& Message, FSimpleDelegate PositiveAction)
{
	if (ActiveConfirmToast)
	{
		ActiveConfirmToast->RemoveFromParent();
		ActiveConfirmToast = nullptr;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	const UKCUISettings* UISettings = GetDefault<UKCUISettings>();
	const TSubclassOf<UKCToastButtonWidget> ToastClass = UISettings
		? UISettings->ToastButtonWidgetClass.LoadSynchronous()
		: nullptr;
	if (!PlayerController || !ToastClass)
	{
		return;
	}

	ActiveConfirmToast = CreateWidget<UKCToastButtonWidget>(PlayerController, ToastClass);
	if (!ActiveConfirmToast)
	{
		return;
	}

	ActiveConfirmToast->Configure(Message, MoveTemp(PositiveAction));
	ActiveConfirmToast->AddToViewport(ConfirmToastZOrder);
}

void UKCEscMenuWidget::ReturnToMainMenu()
{
	RemoveFromParent();

	UGameInstance* GameInstance = GetGameInstance();
	UKCSessionSubsystem* SessionSubsystem = GameInstance
		? GameInstance->GetSubsystem<UKCSessionSubsystem>()
		: nullptr;
	if (SessionSubsystem)
	{
		SessionSubsystem->ReturnToMainMenu();
	}
}

void UKCEscMenuWidget::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UKCEscMenuWidget::HandleCloseClicked()
{
	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UKCLocalPlayerUISubsystem* UISubsystem = LocalPlayer->GetSubsystem<UKCLocalPlayerUISubsystem>())
			{
				UISubsystem->HideEscMenu();
				return;
			}
		}
	}

	RemoveFromParent();
}

void UKCEscMenuWidget::HandleMainMenuClicked()
{
	ShowConfirmToast(
		FText::FromStringTable(LocalizationTableId, MainMenuToastKey),
		FSimpleDelegate::CreateUObject(this, &ThisClass::ReturnToMainMenu));
}

void UKCEscMenuWidget::HandleOptionClicked()
{
	// Option은 아직 미동작으로 유지한다.
}

void UKCEscMenuWidget::HandleExitClicked()
{
	ShowConfirmToast(
		FText::FromStringTable(LocalizationTableId, ExitToastKey),
		FSimpleDelegate::CreateUObject(this, &ThisClass::QuitGame));
}