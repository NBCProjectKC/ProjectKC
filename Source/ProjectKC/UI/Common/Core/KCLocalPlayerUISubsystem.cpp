#include "ProjectKC/UI/Common/Core/KCLocalPlayerUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"

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

	APlayerController* OwnerPlayerController = GetOwningPlayerController();
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
	PendingToastMessages.Reset();

	Super::Deinitialize();
}

APlayerController* UKCLocalPlayerUISubsystem::GetOwningPlayerController() const
{
	const ULocalPlayer* OwnerLocalPlayer = GetLocalPlayer();
	return OwnerLocalPlayer ? OwnerLocalPlayer->GetPlayerController(GetWorld()) : nullptr;
}
