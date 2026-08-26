#include "ProjectKC/UI/Common/Core/KCLocalPlayerUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectKC/UI/Common/Core/KCPrimaryGameLayout.h"
#include "ProjectKC/UI/Common/Core/KCUISettings.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"

void UKCLocalPlayerUISubsystem::SetPrimaryGameLayout(UKCPrimaryGameLayout* InPrimaryGameLayout)
{
	PrimaryGameLayout = InPrimaryGameLayout;
}

UKCPrimaryGameLayout* UKCLocalPlayerUISubsystem::EnsurePrimaryGameLayout()
{
	if (PrimaryGameLayout)
	{
		return PrimaryGameLayout;
	}

	APlayerController* OwnerPlayerController = GetOwningPlayerController();
	const UKCUISettings* UISettings = GetDefault<UKCUISettings>();
	const TSubclassOf<UKCPrimaryGameLayout> LayoutClass =
		UISettings ? UISettings->PrimaryGameLayoutClass.LoadSynchronous() : nullptr;
	if (!OwnerPlayerController || !LayoutClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC UI failed to create PrimaryGameLayout: PlayerController=%s, LayoutClass=%s."),
			*GetNameSafe(OwnerPlayerController),
			*GetNameSafe(LayoutClass));
		return nullptr;
	}

	PrimaryGameLayout = CreateWidget<UKCPrimaryGameLayout>(OwnerPlayerController, LayoutClass);
	if (PrimaryGameLayout)
	{
		const bool bAddedToScreen = PrimaryGameLayout->AddToPlayerScreen();
		UE_LOG(LogTemp, Log, TEXT("KC UI created PrimaryGameLayout: Widget=%s, AddedToScreen=%s."),
			*GetNameSafe(PrimaryGameLayout),
			bAddedToScreen ? TEXT("true") : TEXT("false"));
	}

	return PrimaryGameLayout;
}

UCommonActivatableWidget* UKCLocalPlayerUISubsystem::PushWidgetToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	UKCPrimaryGameLayout* Layout = EnsurePrimaryGameLayout();
	return Layout ? Layout->PushWidgetToLayer(LayerTag, WidgetClass) : nullptr;
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

	APlayerController* OwnerPlayerController = GetOwningPlayerController();
	if (!OwnerPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("KC UI failed to set HUD: OwnerPlayerController is null."));
		return nullptr;
	}

	ActiveHUDWidget = CreateWidget<UKCUserWidget>(OwnerPlayerController, WidgetClass);
	if (ActiveHUDWidget)
	{
		if (UKCPrimaryGameLayout* Layout = EnsurePrimaryGameLayout())
		{
			Layout->SetHUDWidget(ActiveHUDWidget);
		}

		UE_LOG(LogTemp, Log, TEXT("KC UI set HUD: Widget=%s, Class=%s."),
			*GetNameSafe(ActiveHUDWidget),
			*GetNameSafe(WidgetClass));
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
	if (PrimaryGameLayout)
	{
		PrimaryGameLayout->ClearHUDWidget();
	}

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

APlayerController* UKCLocalPlayerUISubsystem::GetOwningPlayerController() const
{
	const ULocalPlayer* OwnerLocalPlayer = GetLocalPlayer();
	return OwnerLocalPlayer ? OwnerLocalPlayer->GetPlayerController(GetWorld()) : nullptr;
}
