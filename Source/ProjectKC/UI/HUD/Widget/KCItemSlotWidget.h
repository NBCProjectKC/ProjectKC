#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "KCItemSlotWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UKCItemSlotViewModel;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCItemSlotWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UKCItemSlotViewModel* GetItemSlotViewModel() const { return ItemSlotViewModel; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Material")
	FName DurabilityBlockParameterName = TEXT("Number_ob_Block");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Material")
	FName DurabilityProgressParameterName = TEXT("Radial_wipe");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Material")
	FName DurabilityGapParameterName = TEXT("Gap");

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UImage> ItemImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UImage> DurabilityImage;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "KC|UI")
	TObjectPtr<UKCItemSlotViewModel> ItemSlotViewModel;

private:
	void HandleItemSlotChanged();
	void RefreshItemSlot();
	void ApplyItemIcon();
	void ApplyDurability();
	void EnsureDurabilityDynamicMaterial();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DurabilityDynamicMaterial;
};
