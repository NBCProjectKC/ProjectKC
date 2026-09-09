/**
 * @file KCLobbyToastWidget.cpp
 * @brief 로비 전용 토스트 위젯 구현부
 */

#include "ProjectKC/Lobby/UI/KCLobbyToastWidget.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "TimerManager.h"

UKCLobbyToastWidget::UKCLobbyToastWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UKCLobbyToastWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!CachedMessage.IsEmpty())
	{
		SetToastMessage(CachedMessage);
	}

	if (Anim)
	{
		PlayAnimation(Anim);
	}
}

void UKCLobbyToastWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DismissTimerHandle);
	}

	Super::NativeDestruct();
}

void UKCLobbyToastWidget::SetToastMessage_Implementation(const FText& Message)
{
	CachedMessage = Message;

	bool bApplied = false;
	if (Text_Message)
	{
		Text_Message->SetText(Message);
		bApplied = true;
	}
	
	if (!bApplied && WidgetTree)
	{
		TArray<UWidget*> AllWidgets;
		WidgetTree->GetAllWidgets(AllWidgets);
		for (UWidget* ChildWidget : AllWidgets)
		{
			if (UTextBlock* FallbackText = Cast<UTextBlock>(ChildWidget))
			{
				FallbackText->SetText(Message);
				bApplied = true;
				break;
			}
		}
	}
}

void UKCLobbyToastWidget::ShowToast(const FText& Message, float DisplayDuration)
{
	SetToastMessage(Message);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DismissTimerHandle);

		const float Duration = (DisplayDuration > 0.0f) ? DisplayDuration : DefaultToastDuration;
		World->GetTimerManager().SetTimer(
			DismissTimerHandle,
			this,
			&UKCLobbyToastWidget::OnDismissTimerFired,
			Duration,
			false
		);
	}
}

void UKCLobbyToastWidget::DismissToast()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DismissTimerHandle);
	}

	RemoveFromParent();
}

void UKCLobbyToastWidget::OnDismissTimerFired()
{
	DismissToast();
}
