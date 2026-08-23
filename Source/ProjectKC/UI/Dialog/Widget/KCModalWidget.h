#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCActivatableWidget.h"
#include "KCModalWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCModalWidget : public UKCActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "KC|UI")
	void SetDialogMessage(const FText& Message);
};
