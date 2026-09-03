#include "ProjectKC/UI/HUD/ViewModel/KCItemSlotViewModel.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "ProjectKC/Item/Component/KCHeldItemComponent.h"
#include "ProjectKC/Item/Definition/KCItemDefinition.h"
#include "ProjectKC/Item/KCWorldItemActor.h"

void UKCItemSlotViewModel::StartListening(UObject* WorldContextObject)
{
	StopListening();

	if (!WorldContextObject)
	{
		return;
	}

	ListeningWorldContext = WorldContextObject;
	TryBindHeldItemComponent();
}

void UKCItemSlotViewModel::StopListening()
{
	StopHeldItemComponentRetry();
	UnbindHeldItem();
	UnbindHeldItemComponent();
	ListeningWorldContext.Reset();
	RefreshFromHeldItem();
}

void UKCItemSlotViewModel::HandleHeldItemChanged(AKCWorldItemActor* NewHeldItem)
{
	SetHeldItem(NewHeldItem);
}

void UKCItemSlotViewModel::HandleDurabilityChanged(
	float PreviousDurability,
	float CurrentDurability)
{
	(void)PreviousDurability;
	(void)CurrentDurability;
	RefreshFromHeldItem();
}

void UKCItemSlotViewModel::BindHeldItemComponent(UKCHeldItemComponent* HeldItemComponent)
{
	if (BoundHeldItemComponent.Get() == HeldItemComponent)
	{
		return;
	}

	UnbindHeldItemComponent();
	BoundHeldItemComponent = HeldItemComponent;

	if (HeldItemComponent)
	{
		HeldItemComponent->OnHeldItemChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleHeldItemChanged);
		SetHeldItem(HeldItemComponent->GetHeldItem());
	}
	else
	{
		SetHeldItem(nullptr);
		StartHeldItemComponentRetry();
	}
}

void UKCItemSlotViewModel::UnbindHeldItemComponent()
{
	if (UKCHeldItemComponent* HeldItemComponent = BoundHeldItemComponent.Get())
	{
		HeldItemComponent->OnHeldItemChanged.RemoveDynamic(
			this,
			&ThisClass::HandleHeldItemChanged);
	}

	BoundHeldItemComponent.Reset();
}

void UKCItemSlotViewModel::TryBindHeldItemComponent()
{
	UKCHeldItemComponent* HeldItemComponent = ResolveHeldItemComponent();
	BindHeldItemComponent(HeldItemComponent);

	if (HeldItemComponent)
	{
		StopHeldItemComponentRetry();
	}
	else
	{
		StartHeldItemComponentRetry();
	}
}

void UKCItemSlotViewModel::StartHeldItemComponentRetry()
{
	UObject* WorldContextObject = ListeningWorldContext.Get();
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World || World->GetTimerManager().IsTimerActive(HeldItemComponentRetryHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		HeldItemComponentRetryHandle,
		this,
		&ThisClass::TryBindHeldItemComponent,
		0.2f,
		true);
}

void UKCItemSlotViewModel::StopHeldItemComponentRetry()
{
	UObject* WorldContextObject = ListeningWorldContext.Get();
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(HeldItemComponentRetryHandle);
}

void UKCItemSlotViewModel::SetHeldItem(AKCWorldItemActor* NewHeldItem)
{
	if (BoundHeldItem.Get() == NewHeldItem)
	{
		RefreshFromHeldItem();
		return;
	}

	UnbindHeldItem();
	BoundHeldItem = NewHeldItem;

	if (NewHeldItem)
	{
		NewHeldItem->OnDurabilityChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleDurabilityChanged);
	}

	RefreshFromHeldItem();
}

void UKCItemSlotViewModel::UnbindHeldItem()
{
	if (AKCWorldItemActor* HeldItem = BoundHeldItem.Get())
	{
		HeldItem->OnDurabilityChanged.RemoveDynamic(
			this,
			&ThisClass::HandleDurabilityChanged);
	}

	BoundHeldItem.Reset();
}

void UKCItemSlotViewModel::RefreshFromHeldItem()
{
	AKCWorldItemActor* HeldItem = BoundHeldItem.Get();
	const UKCItemDefinition* ItemDefinition = HeldItem ? HeldItem->GetItemDefinition() : nullptr;

	const bool bNewHasItem = HeldItem != nullptr && ItemDefinition != nullptr;
	const UTexture2D* NewItemIcon = bNewHasItem ? ItemDefinition->Icon.Get() : nullptr;
	const bool bNewDurabilityVisible = bNewHasItem && HeldItem->UsesDurability();
	const float NewDurabilityPercent = bNewDurabilityVisible
		? HeldItem->GetDurabilityNormalized()
		: 0.0f;
	const float NewDurabilityBlockCount = bNewDurabilityVisible
		? CalculateDurabilityBlockCount(ItemDefinition)
		: 0.0f;
	const float NewDurabilityGap = bNewDurabilityVisible
		? CalculateDurabilityGap(ItemDefinition)
		: 1.0f;

	bool bChanged = false;

	if (bHasItem != bNewHasItem)
	{
		bHasItem = bNewHasItem;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bHasItem);
		bChanged = true;
	}

	if (ItemIcon != NewItemIcon)
	{
		ItemIcon = const_cast<UTexture2D*>(NewItemIcon);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ItemIcon);
		bChanged = true;
	}

	if (bDurabilityVisible != bNewDurabilityVisible)
	{
		bDurabilityVisible = bNewDurabilityVisible;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bDurabilityVisible);
		bChanged = true;
	}

	if (!FMath::IsNearlyEqual(DurabilityPercent, NewDurabilityPercent))
	{
		DurabilityPercent = NewDurabilityPercent;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DurabilityPercent);
		bChanged = true;
	}

	if (!FMath::IsNearlyEqual(DurabilityBlockCount, NewDurabilityBlockCount))
	{
		DurabilityBlockCount = NewDurabilityBlockCount;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DurabilityBlockCount);
		bChanged = true;
	}

	if (!FMath::IsNearlyEqual(DurabilityGap, NewDurabilityGap))
	{
		DurabilityGap = NewDurabilityGap;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DurabilityGap);
		bChanged = true;
	}

	if (bChanged)
	{
		BroadcastItemSlotChanged();
	}
}

void UKCItemSlotViewModel::BroadcastItemSlotChanged()
{
	OnItemSlotChangedNative.Broadcast();
}

float UKCItemSlotViewModel::CalculateDurabilityBlockCount(
	const UKCItemDefinition* ItemDefinition) const
{
	if (!ItemDefinition || !ItemDefinition->Durability.IsEnabled() ||
		ItemDefinition->Durability.ConsumeAmount <= 0.0f)
	{
		return 0.0f;
	}

	return FKCItemDurabilityStruct::MaximumDurability /
		ItemDefinition->Durability.ConsumeAmount;
}

float UKCItemSlotViewModel::CalculateDurabilityGap(
	const UKCItemDefinition* ItemDefinition) const
{
	if (!ItemDefinition || !ItemDefinition->Durability.IsEnabled())
	{
		return 1.0f;
	}

	return ItemDefinition->Durability.ConsumeMode ==
		EKCItemDurabilityConsumeMode::OnFirstHit ? 0.9f : 1.0f;
}

UKCHeldItemComponent* UKCItemSlotViewModel::ResolveHeldItemComponent() const
{
	UObject* WorldContextObject = ListeningWorldContext.Get();
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	APlayerController* PlayerController = nullptr;

	if (UUserWidget* UserWidget = Cast<UUserWidget>(WorldContextObject))
	{
		PlayerController = UserWidget->GetOwningPlayer();
	}

	if (!PlayerController && World)
	{
		PlayerController = World->GetFirstPlayerController();
	}

	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	return Pawn ? Pawn->FindComponentByClass<UKCHeldItemComponent>() : nullptr;
}
