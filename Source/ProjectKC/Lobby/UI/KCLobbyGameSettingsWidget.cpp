#include "ProjectKC/Lobby/UI/KCLobbyGameSettingsWidget.h"
#include "ProjectKC/Lobby/UI/KCLobbyWidget.h"
#include "ProjectKC/Lobby/KCLobbyPlayerController.h"
#include "ProjectKC/ProjectKC.h"
#include "Components/Button.h"

void UKCLobbyGameSettingsWidget::InitializeGameSettings(
	UKCLobbyWidget* InLobbyWidget,
	int32 InCount,
	EKCLevelType InMap,
	float InDuration)
{
	OwnerLobbyWidget = InLobbyWidget;
	SelectedPlayerCount = InCount;
	SelectedMapType = InMap;
	SelectedMatchDuration = InDuration;

	UE_LOG(LogKCLobby, Log, TEXT("[KCLobbyGameSettingsWidget] Initialized: Players=%d, Map=%d, Duration=%.0fs"),
		SelectedPlayerCount, static_cast<int32>(SelectedMapType), SelectedMatchDuration);

	OnSettingsInitialized(SelectedPlayerCount, SelectedMapType, SelectedMatchDuration);
}

void UKCLobbyGameSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Apply)
	{
		Button_Apply->OnClicked.AddDynamic(this, &UKCLobbyGameSettingsWidget::OnApplyButtonClicked);
	}

	if (Button_Close)
	{
		Button_Close->OnClicked.AddDynamic(this, &UKCLobbyGameSettingsWidget::OnCloseButtonClicked);
	}
}

void UKCLobbyGameSettingsWidget::NativeDestruct()
{
	if (Button_Apply)
	{
		Button_Apply->OnClicked.RemoveDynamic(this, &UKCLobbyGameSettingsWidget::OnApplyButtonClicked);
	}

	if (Button_Close)
	{
		Button_Close->OnClicked.RemoveDynamic(this, &UKCLobbyGameSettingsWidget::OnCloseButtonClicked);
	}

	Super::NativeDestruct();
}

void UKCLobbyGameSettingsWidget::SetSelectedPlayerCount(int32 InCount)
{
	SelectedPlayerCount = FMath::Clamp((InCount / 2) * 2, 2, 6);
	OnSettingValuesChanged(SelectedPlayerCount, SelectedMapType, SelectedMatchDuration);
}

void UKCLobbyGameSettingsWidget::SetSelectedMapType(EKCLevelType InMapType)
{
	SelectedMapType = InMapType;
	OnSettingValuesChanged(SelectedPlayerCount, SelectedMapType, SelectedMatchDuration);
}

void UKCLobbyGameSettingsWidget::SetSelectedMatchDuration(float InSeconds)
{
	SelectedMatchDuration = FMath::Max(60.0f, InSeconds);
	OnSettingValuesChanged(SelectedPlayerCount, SelectedMapType, SelectedMatchDuration);
}

void UKCLobbyGameSettingsWidget::ConfirmAndApplySettings()
{
	if (AKCLobbyPlayerController* PC = Cast<AKCLobbyPlayerController>(GetOwningPlayer()))
	{
		PC->ROS_ApplyGameSettings(SelectedPlayerCount, SelectedMapType, SelectedMatchDuration);
	}

	CloseSettings();
}

void UKCLobbyGameSettingsWidget::CloseSettings()
{
	if (UKCLobbyWidget* LobbyUI = OwnerLobbyWidget.Get())
	{
		LobbyUI->NotifyGameSettingsWidgetClosed(this);
	}

	RemoveFromParent();
}

void UKCLobbyGameSettingsWidget::OnApplyButtonClicked()
{
	ConfirmAndApplySettings();
}

void UKCLobbyGameSettingsWidget::OnCloseButtonClicked()
{
	CloseSettings();
}
