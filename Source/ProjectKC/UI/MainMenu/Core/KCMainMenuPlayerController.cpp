#include "ProjectKC/UI/MainMenu/Core/KCMainMenuPlayerController.h"

#include "Camera/CameraActor.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "ProjectKC/UI/Common/Core/KCLocalPlayerUISubsystem.h"

void AKCMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitializeMainMenuInput();
	ApplyMainMenuCamera();
}

void AKCMainMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearMainMenuUI();

	Super::EndPlay(EndPlayReason);
}

void AKCMainMenuPlayerController::InitializeMainMenuInput()
{
	if (!IsLocalController())
	{
		return;
	}

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void AKCMainMenuPlayerController::ApplyMainMenuCamera()
{
	if (!IsLocalController())
	{
		return;
	}

	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		SetViewTarget(*It);
		return;
	}
}

void AKCMainMenuPlayerController::ClearMainMenuUI()
{
	if (!IsLocalController())
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UKCLocalPlayerUISubsystem* UISubsystem = LocalPlayer->GetSubsystem<UKCLocalPlayerUISubsystem>())
		{
			UISubsystem->ClearScreenWidget();
		}
	}
}
