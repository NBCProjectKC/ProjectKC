#include "ProjectKC/UI/MainMenu/Core/KCMainMenuPlayerController.h"

#include "Engine/LocalPlayer.h"
#include "ProjectKC/UI/Common/Core/KCLocalPlayerUISubsystem.h"
#include "ProjectKC/UI/Common/Core/KCUISettings.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"

void AKCMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitializeMainMenuUI();
}

void AKCMainMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearMainMenuUI();

	Super::EndPlay(EndPlayReason);
}

void AKCMainMenuPlayerController::InitializeMainMenuUI()
{
	if (!IsLocalController())
	{
		return;
	}

	bShowMouseCursor = true;

	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC MainMenu UI failed: LocalPlayer is null on %s."), *GetName());
		return;
	}

	UKCLocalPlayerUISubsystem* UISubsystem = LocalPlayer->GetSubsystem<UKCLocalPlayerUISubsystem>();
	if (!UISubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC MainMenu UI failed: KCLocalPlayerUISubsystem is null on %s."), *GetName());
		return;
	}

	const UKCUISettings* UISettings = GetDefault<UKCUISettings>();
	const TSubclassOf<UKCUserWidget> MainMenuScreenClass =
		UISettings ? UISettings->MainMenuScreenClass.LoadSynchronous() : nullptr;
	if (!MainMenuScreenClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC MainMenu UI failed: MainMenuScreenClass is not configured in ProjectKC UI settings."));
		return;
	}

	if (!UISubsystem->SetScreenWidget(MainMenuScreenClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("KC MainMenu UI failed: SetScreenWidget returned null for %s."), *GetNameSafe(MainMenuScreenClass));
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
