#include "ProjectKC/UI/Common/Core/KCLocalPlayerUISubsystem.h"

#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"

UKCUserWidget* UKCLocalPlayerUISubsystem::SetScreenWidget(TSubclassOf<UKCUserWidget> WidgetClass, bool bPersistAcrossLevelTravel)
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
		bool bAddedToScreen = false;

		// 트래블 동안 위젯을 유지시키는 분기
		if (bPersistAcrossLevelTravel)
		{
			// AddToPlayerScreen()은 ZOrder만 받고 Slot 세부값(bAutoRemoveOnWorldRemoved)을 노출 안 함
			// 플래그를 끄려면 서브시스템 직접 호출 필요
			if (UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get())
			{
				FGameViewportWidgetSlot Slot;
				Slot.bAutoRemoveOnWorldRemoved = false;
				ViewportSubsystem->AddWidgetForPlayer(ActiveScreenWidget, OwnerLocalPlayer, Slot);
				bAddedToScreen = true;
			}
		}
		else
		{
			bAddedToScreen = ActiveScreenWidget->AddToPlayerScreen();
		}

		UE_LOG(LogTemp, Log, TEXT("KC UI set screen: Widget=%s, Class=%s, AddedToScreen=%s, Persistent=%s."),
			*GetNameSafe(ActiveScreenWidget),
			*GetNameSafe(WidgetClass),
			bAddedToScreen ? TEXT("true") : TEXT("false"),
			bPersistAcrossLevelTravel ? TEXT("true") : TEXT("false"));
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

UKCUserWidget* UKCLocalPlayerUISubsystem::SetHUDWidget(TSubclassOf<UKCUserWidget> WidgetClass, bool bPersistAcrossLevelTravel)
{
	return SetScreenWidget(WidgetClass, bPersistAcrossLevelTravel);
}

void UKCLocalPlayerUISubsystem::ClearHUDWidget()
{
	ClearScreenWidget();
}

void UKCLocalPlayerUISubsystem::QueueToast(const FText& Message)
{
	PendingToastMessages.Add(Message);
}

void UKCLocalPlayerUISubsystem::Deinitialize()
{
	ClearScreenWidget();
	PendingToastMessages.Reset();

	Super::Deinitialize();
}

APlayerController* UKCLocalPlayerUISubsystem::GetOwningPlayerController() const
{
	const ULocalPlayer* OwnerLocalPlayer = GetLocalPlayer();
	return OwnerLocalPlayer ? OwnerLocalPlayer->GetPlayerController(GetWorld()) : nullptr;
}
