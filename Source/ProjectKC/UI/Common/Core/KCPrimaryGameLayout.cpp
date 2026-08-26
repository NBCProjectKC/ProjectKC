#include "ProjectKC/UI/Common/Core/KCPrimaryGameLayout.h"

#include "CommonActivatableWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "ProjectKC/UI/Common/Core/KCUITags.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"

void UKCPrimaryGameLayout::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureRuntimeLayers();
	CacheLayerStacks();
}

UCommonActivatableWidget* UKCPrimaryGameLayout::PushWidgetToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!WidgetClass)
	{
		return nullptr;
	}

	UCommonActivatableWidgetStack* Stack = GetLayerStack(LayerTag);
	return Stack ? Stack->AddWidget(WidgetClass) : nullptr;
}

UCommonActivatableWidgetStack* UKCPrimaryGameLayout::GetLayerStack(FGameplayTag LayerTag) const
{
	const TObjectPtr<UCommonActivatableWidgetStack>* Stack = LayerStacks.Find(LayerTag);
	return Stack ? Stack->Get() : nullptr;
}

void UKCPrimaryGameLayout::EnsureRuntimeLayers()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		return;
	}

	if (!HUDLayer)
	{
		HUDLayer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HUDLayer"));
		AddRuntimeLayerToRoot(HUDLayer, 0);
	}

	if (!GameLayer)
	{
		GameLayer = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(
			UCommonActivatableWidgetStack::StaticClass(),
			TEXT("GameLayer"));
		AddRuntimeLayerToRoot(GameLayer, 10);
	}

	if (!MenuLayer)
	{
		MenuLayer = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(
			UCommonActivatableWidgetStack::StaticClass(),
			TEXT("MenuLayer"));
		AddRuntimeLayerToRoot(MenuLayer, 20);
	}

	if (!GameMenuLayer)
	{
		GameMenuLayer = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(
			UCommonActivatableWidgetStack::StaticClass(),
			TEXT("GameMenuLayer"));
		AddRuntimeLayerToRoot(GameMenuLayer, 30);
	}

	if (!ModalLayer)
	{
		ModalLayer = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(
			UCommonActivatableWidgetStack::StaticClass(),
			TEXT("ModalLayer"));
		AddRuntimeLayerToRoot(ModalLayer, 40);
	}
}

void UKCPrimaryGameLayout::AddRuntimeLayerToRoot(UWidget* LayerWidget, int32 ZOrder)
{
	if (!LayerWidget)
	{
		return;
	}

	UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree ? WidgetTree->RootWidget : nullptr);
	if (!RootPanel)
	{
		return;
	}

	UPanelSlot* PanelSlot = RootPanel->AddChild(LayerWidget);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(PanelSlot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
		CanvasSlot->SetZOrder(ZOrder);
	}
}

void UKCPrimaryGameLayout::SetHUDWidget(UKCUserWidget* InHUDWidget)
{
	ClearHUDWidget();

	if (!HUDLayer || !InHUDWidget)
	{
		return;
	}

	UOverlaySlot* HUDSlot = HUDLayer->AddChildToOverlay(InHUDWidget);
	if (HUDSlot)
	{
		HUDSlot->SetHorizontalAlignment(HAlign_Fill);
		HUDSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UKCPrimaryGameLayout::ClearHUDWidget()
{
	if (HUDLayer)
	{
		HUDLayer->ClearChildren();
	}
}

void UKCPrimaryGameLayout::CacheLayerStacks()
{
	LayerStacks.Reset();

	LayerStacks.Add(KCUITags::UI_Layer_Game, GameLayer);
	LayerStacks.Add(KCUITags::UI_Layer_Menu, MenuLayer);
	LayerStacks.Add(KCUITags::UI_Layer_GameMenu, GameMenuLayer);
	LayerStacks.Add(KCUITags::UI_Layer_Modal, ModalLayer);
}
