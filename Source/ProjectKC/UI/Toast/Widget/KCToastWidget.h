#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "KCToastWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCToastWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "KC|UI")
	void SetToastMessage(const FText& Message);
};
