#include "ProjectKC/UI/Common/Core/KCLocalPlayerUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectKC/UI/Common/Core/KCPrimaryGameLayout.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"

void UKCLocalPlayerUISubsystem::SetPrimaryGameLayout(UKCPrimaryGameLayout* InPrimaryGameLayout)
{
	PrimaryGameLayout = InPrimaryGameLayout;
}

UCommonActivatableWidget* UKCLocalPlayerUISubsystem::PushWidgetToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	return PrimaryGameLayout ? PrimaryGameLayout->PushWidgetToLayer(LayerTag, WidgetClass) : nullptr;
}

UKCUserWidget* UKCLocalPlayerUISubsystem::SetHUDWidget(TSubclassOf<UKCUserWidget> WidgetClass)
{
	ClearHUDWidget();

	ULocalPlayer* OwnerLocalPlayer = GetLocalPlayer();
	if (!OwnerLocalPlayer || !WidgetClass)
	{
		return nullptr;
	}

	APlayerController* OwnerPlayerController = OwnerLocalPlayer->GetPlayerController(GetWorld());
	if (!OwnerPlayerController)
	{
		return nullptr;
	}

	ActiveHUDWidget = CreateWidget<UKCUserWidget>(OwnerPlayerController, WidgetClass);
	if (ActiveHUDWidget)
	{
		ActiveHUDWidget->AddToPlayerScreen();
	}

	return ActiveHUDWidget;
}

void UKCLocalPlayerUISubsystem::ClearHUDWidget()
{
	if (ActiveHUDWidget)
	{
		ActiveHUDWidget->RemoveFromParent();
		ActiveHUDWidget = nullptr;
	}
}

void UKCLocalPlayerUISubsystem::QueueToast(const FText& Message)
{
	PendingToastMessages.Add(Message);
}

void UKCLocalPlayerUISubsystem::Deinitialize()
{
	ClearHUDWidget();
	PrimaryGameLayout = nullptr;
	PendingToastMessages.Reset();

	Super::Deinitialize();
}
