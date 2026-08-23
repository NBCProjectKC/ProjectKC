#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "KCActivatableWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnKCActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnKCDeactivated();
};
