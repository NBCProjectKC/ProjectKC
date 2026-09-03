#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "TimerManager.h"
#include "KCItemSlotViewModel.generated.h"

class AKCWorldItemActor;
class UTexture2D;
class UKCHeldItemComponent;
class UKCItemDefinition;

DECLARE_MULTICAST_DELEGATE(FKCItemSlotChangedNativeDelegate);

UCLASS(BlueprintType, Blueprintable, meta = (MVVMAllowedContextCreationType = "Manual|CreateInstance"))
class PROJECTKC_API UKCItemSlotViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void StartListening(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void StopListening();

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	bool HasItem() const { return bHasItem; }

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UTexture2D* GetItemIcon() const { return ItemIcon; }

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	bool IsDurabilityVisible() const { return bDurabilityVisible; }

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	float GetDurabilityPercent() const { return DurabilityPercent; }

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	float GetDurabilityBlockCount() const { return DurabilityBlockCount; }

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	float GetDurabilityGap() const { return DurabilityGap; }

	FKCItemSlotChangedNativeDelegate OnItemSlotChangedNative;

private:
	UFUNCTION()
	void HandleHeldItemChanged(AKCWorldItemActor* NewHeldItem);

	UFUNCTION()
	void HandleDurabilityChanged(float PreviousDurability, float CurrentDurability);

	void BindHeldItemComponent(UKCHeldItemComponent* HeldItemComponent);
	void UnbindHeldItemComponent();
	void TryBindHeldItemComponent();
	void StartHeldItemComponentRetry();
	void StopHeldItemComponentRetry();
	void SetHeldItem(AKCWorldItemActor* NewHeldItem);
	void UnbindHeldItem();
	void RefreshFromHeldItem();
	void BroadcastItemSlotChanged();
	float CalculateDurabilityBlockCount(const UKCItemDefinition* ItemDefinition) const;
	float CalculateDurabilityGap(const UKCItemDefinition* ItemDefinition) const;
	UKCHeldItemComponent* ResolveHeldItemComponent() const;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter = "HasItem", Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	bool bHasItem = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> ItemIcon;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter = "IsDurabilityVisible", Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	bool bDurabilityVisible = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	float DurabilityPercent = 0.0f;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	float DurabilityBlockCount = 0.0f;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	float DurabilityGap = 1.0f;

	TWeakObjectPtr<UObject> ListeningWorldContext;
	TWeakObjectPtr<UKCHeldItemComponent> BoundHeldItemComponent;
	TWeakObjectPtr<AKCWorldItemActor> BoundHeldItem;
	FTimerHandle HeldItemComponentRetryHandle;
};
