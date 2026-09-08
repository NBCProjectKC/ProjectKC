#include "ProjectKC/UI/MainMenu/Core/KCMainMenu3DManager.h"

#include "Camera/CameraActor.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Lobby/KCSessionSubsystem.h"
#include "ProjectKC/UI/Common/Core/KCLocalPlayerUISubsystem.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "ProjectKC/UI/MainMenu/Core/KCMainMenu3DButton.h"
#include "TimerManager.h"

AKCMainMenu3DManager::AKCMainMenu3DManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AKCMainMenu3DManager::BeginPlay()
{
	Super::BeginPlay();

	InitializeMainMenu();
}

void AKCMainMenu3DManager::NotifyButtonHovered(AKCMainMenu3DButton* Button)
{
	CurrentHoveredButton = Button;
}

void AKCMainMenu3DManager::NotifyButtonUnhovered(AKCMainMenu3DButton* Button)
{
	if (CurrentHoveredButton == Button)
	{
		CurrentHoveredButton = nullptr;
	}
}

void AKCMainMenu3DManager::HandleButtonClicked(AKCMainMenu3DButton* Button)
{
	if (!Button)
	{
		return;
	}

	switch (Button->GetAction())
	{
	case EKCMainMenu3DAction::CreateLobby:
		OpenCreateLobbyUI();
		break;
	case EKCMainMenu3DAction::Option:
		OpenOptionUI();
		break;
	case EKCMainMenu3DAction::Exit:
		ExitGame();
		break;
	default:
		break;
	}
}

void AKCMainMenu3DManager::InitializeMainMenu()
{
	InitializeButtons();

	RemainingCameraSetupAttempts = 30;
	RetrySetMainMenuCamera();
}

bool AKCMainMenu3DManager::TrySetMainMenuCamera()
{
	APlayerController* PlayerController = ResolvePlayerController();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return false;
	}

	if (!MainMenuCamera)
	{
		for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
		{
			MainMenuCamera = *It;
			break;
		}
	}

	if (MainMenuCamera)
	{
		PlayerController->SetViewTarget(MainMenuCamera);
		return true;
	}

	return false;
}

void AKCMainMenu3DManager::RetrySetMainMenuCamera()
{
	if (TrySetMainMenuCamera())
	{
		GetWorldTimerManager().ClearTimer(CameraSetupRetryTimerHandle);
		return;
	}

	--RemainingCameraSetupAttempts;
	if (RemainingCameraSetupAttempts <= 0)
	{
		GetWorldTimerManager().ClearTimer(CameraSetupRetryTimerHandle);
		return;
	}

	GetWorldTimerManager().SetTimer(
		CameraSetupRetryTimerHandle,
		this,
		&ThisClass::RetrySetMainMenuCamera,
		0.1f,
		false);
}

void AKCMainMenu3DManager::InitializeButtons()
{
	if (MenuButtons.IsEmpty())
	{
		for (TActorIterator<AKCMainMenu3DButton> It(GetWorld()); It; ++It)
		{
			MenuButtons.Add(*It);
		}
	}

	for (AKCMainMenu3DButton* Button : MenuButtons)
	{
		if (Button)
		{
			Button->InitializeButton(this);
		}
	}
}

void AKCMainMenu3DManager::OpenCreateLobbyUI()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UKCSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UKCSessionSubsystem>())
		{
			SessionSubsystem->CreateSession(DefaultPublicConnections, bCreateLANSession);
		}
	}
}

void AKCMainMenu3DManager::OpenOptionUI()
{
	if (OptionWidgetClass)
	{
		ShowScreenWidget(OptionWidgetClass);
	}
}

void AKCMainMenu3DManager::ExitGame()
{
	UKismetSystemLibrary::QuitGame(this, ResolvePlayerController(), EQuitPreference::Quit, false);
}

bool AKCMainMenu3DManager::ShowScreenWidget(TSubclassOf<UKCUserWidget> WidgetClass)
{
	APlayerController* PlayerController = ResolvePlayerController();
	if (!PlayerController || !WidgetClass)
	{
		return false;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return false;
	}

	UKCLocalPlayerUISubsystem* UISubsystem = LocalPlayer->GetSubsystem<UKCLocalPlayerUISubsystem>();
	if (!UISubsystem)
	{
		return false;
	}

	return UISubsystem->SetScreenWidget(WidgetClass) != nullptr;
}

APlayerController* AKCMainMenu3DManager::ResolvePlayerController() const
{
	UWorld* World = GetWorld();
	return World ? World->GetFirstPlayerController() : nullptr;
}
