#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "KCPrimaryGameLayout.generated.h"

class UCommonActivatableWidgetStack;
class UCommonActivatableWidget;
class UOverlay;
class UPanelWidget;
class UWidget;
class UKCUserWidget;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCPrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	UCommonActivatableWidget* PushWidgetToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UCommonActivatableWidgetStack* GetLayerStack(FGameplayTag LayerTag) const;

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetHUDWidget(UKCUserWidget* InHUDWidget);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void ClearHUDWidget();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UCommonActivatableWidgetStack> GameLayer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UOverlay> HUDLayer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UCommonActivatableWidgetStack> MenuLayer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UCommonActivatableWidgetStack> GameMenuLayer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UCommonActivatableWidgetStack> ModalLayer;

private:
	void EnsureRuntimeLayers();
	void AddRuntimeLayerToRoot(UWidget* LayerWidget, int32 ZOrder);
	void CacheLayerStacks();

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetStack>> LayerStacks;
};
