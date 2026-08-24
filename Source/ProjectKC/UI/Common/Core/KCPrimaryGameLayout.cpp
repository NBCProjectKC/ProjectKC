#include "ProjectKC/UI/Common/Core/KCPrimaryGameLayout.h"

#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "ProjectKC/UI/Common/Core/KCUITags.h"

void UKCPrimaryGameLayout::NativeConstruct()
{
	Super::NativeConstruct();

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

void UKCPrimaryGameLayout::CacheLayerStacks()
{
	LayerStacks.Reset();

	LayerStacks.Add(KCUITags::UI_Layer_Game, GameLayer);
	LayerStacks.Add(KCUITags::UI_Layer_Menu, MenuLayer);
	LayerStacks.Add(KCUITags::UI_Layer_GameMenu, GameMenuLayer);
	LayerStacks.Add(KCUITags::UI_Layer_Modal, ModalLayer);
	LayerStacks.Add(KCUITags::UI_Layer_System, SystemLayer);
	LayerStacks.Add(KCUITags::UI_Layer_Toast, ToastLayer);
	LayerStacks.Add(KCUITags::UI_Layer_Indicator, IndicatorLayer);
	LayerStacks.Add(KCUITags::UI_Layer_Transition, TransitionLayer);
}
