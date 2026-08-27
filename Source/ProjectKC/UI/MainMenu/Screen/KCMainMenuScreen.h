#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCActivatableWidget.h"
#include "KCMainMenuScreen.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCMainMenuScreen : public UKCActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnStartRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnSettingsRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnQuitRequested();
};
