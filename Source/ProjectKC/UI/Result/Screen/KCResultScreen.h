#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCActivatableWidget.h"
#include "KCResultScreen.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCResultScreen : public UKCActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnRestartRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnReturnToLobbyRequested();
};
