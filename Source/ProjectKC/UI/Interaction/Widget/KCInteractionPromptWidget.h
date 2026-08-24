#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "KCInteractionPromptWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCInteractionPromptWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "KC|UI")
	void SetInteractionPrompt(const FText& InputText, const FText& ActionText);
};
