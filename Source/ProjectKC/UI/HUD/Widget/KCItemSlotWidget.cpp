#include "ProjectKC/UI/HUD/Widget/KCItemSlotWidget.h"

#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProjectKC/UI/HUD/ViewModel/KCItemSlotViewModel.h"

void UKCItemSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	RefreshItemSlot();
}

void UKCItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ItemSlotViewModel)
	{
		ItemSlotViewModel = NewObject<UKCItemSlotViewModel>(this);
	}

	EnsureDurabilityDynamicMaterial();
	ItemSlotViewModel->OnItemSlotChangedNative.AddUObject(
		this,
		&ThisClass::HandleItemSlotChanged);
	ItemSlotViewModel->StartListening(this);
	RefreshItemSlot();
}

void UKCItemSlotWidget::NativeDestruct()
{
	if (ItemSlotViewModel)
	{
		ItemSlotViewModel->OnItemSlotChangedNative.RemoveAll(this);
		ItemSlotViewModel->StopListening();
	}

	Super::NativeDestruct();
}

void UKCItemSlotWidget::HandleItemSlotChanged()
{
	RefreshItemSlot();
}

void UKCItemSlotWidget::RefreshItemSlot()
{
	if (!ItemSlotViewModel || !ItemSlotViewModel->HasItem())
	{
		SetVisibility(ESlateVisibility::Hidden);
		ApplyItemIcon();
		ApplyDurability();
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ApplyItemIcon();
	ApplyDurability();
}

void UKCItemSlotWidget::ApplyItemIcon()
{
	if (!ItemImage)
	{
		return;
	}

	if (!ItemSlotViewModel || !ItemSlotViewModel->HasItem() ||
		!ItemSlotViewModel->GetItemIcon())
	{
		ItemImage->SetBrushFromTexture(nullptr);
		ItemImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	ItemImage->SetBrushFromTexture(ItemSlotViewModel->GetItemIcon(), false);
	ItemImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UKCItemSlotWidget::ApplyDurability()
{
	if (!DurabilityImage)
	{
		return;
	}

	if (!ItemSlotViewModel || !ItemSlotViewModel->IsDurabilityVisible())
	{
		DurabilityImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	EnsureDurabilityDynamicMaterial();
	if (DurabilityDynamicMaterial)
	{
		DurabilityDynamicMaterial->SetScalarParameterValue(
			DurabilityBlockParameterName,
			ItemSlotViewModel->GetDurabilityBlockCount());
		DurabilityDynamicMaterial->SetScalarParameterValue(
			DurabilityProgressParameterName,
			ItemSlotViewModel->GetDurabilityPercent());
		DurabilityDynamicMaterial->SetScalarParameterValue(
			DurabilityGapParameterName,
			ItemSlotViewModel->GetDurabilityGap());
	}

	DurabilityImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UKCItemSlotWidget::EnsureDurabilityDynamicMaterial()
{
	if (DurabilityDynamicMaterial || !DurabilityImage)
	{
		return;
	}

	DurabilityDynamicMaterial = DurabilityImage->GetDynamicMaterial();
}
