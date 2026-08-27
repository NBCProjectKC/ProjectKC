#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCActivatableWidget.h"
#include "KCPauseMenuScreen.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCPauseMenuScreen : public UKCActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnResumeRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnSettingsRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnExitRequested();
};
