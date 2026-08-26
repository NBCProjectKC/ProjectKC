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
		UE_LOG(LogTemp, Warning, TEXT("KC UI failed to set HUD: LocalPlayer=%s, WidgetClass=%s."),
			OwnerLocalPlayer ? TEXT("Valid") : TEXT("Null"),
			*GetNameSafe(WidgetClass));
		return nullptr;
	}

	APlayerController* OwnerPlayerController = OwnerLocalPlayer->GetPlayerController(GetWorld());
	if (!OwnerPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC UI failed to set HUD: OwnerPlayerController is null."));
		return nullptr;
	}

	ActiveHUDWidget = CreateWidget<UKCUserWidget>(OwnerPlayerController, WidgetClass);
	if (ActiveHUDWidget)
	{
		const bool bAddedToScreen = ActiveHUDWidget->AddToPlayerScreen();
		UE_LOG(LogTemp, Log, TEXT("KC UI set HUD: Widget=%s, Class=%s, AddedToScreen=%s."),
			*GetNameSafe(ActiveHUDWidget),
			*GetNameSafe(WidgetClass),
			bAddedToScreen ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("KC UI failed to set HUD: CreateWidget failed for %s."),
			*GetNameSafe(WidgetClass));
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
