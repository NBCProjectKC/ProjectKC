#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "KCPrimaryGameLayout.generated.h"

class UCommonActivatableWidgetStack;
class UCommonActivatableWidget;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCPrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	UCommonActivatableWidget* PushWidgetToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UCommonActivatableWidgetStack* GetLayerStack(FGameplayTag LayerTag) const;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UCommonActivatableWidgetStack> GameLayer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UCommonActivatableWidgetStack> MenuLayer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UCommonActivatableWidgetStack> GameMenuLayer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UCommonActivatableWidgetStack> ModalLayer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UCommonActivatableWidgetStack> SystemLayer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UCommonActivatableWidgetStack> ToastLayer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UCommonActivatableWidgetStack> IndicatorLayer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UCommonActivatableWidgetStack> TransitionLayer;

private:
	void CacheLayerStacks();

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetStack>> LayerStacks;
};
