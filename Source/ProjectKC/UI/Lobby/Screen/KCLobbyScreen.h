#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCActivatableWidget.h"
#include "KCLobbyScreen.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCLobbyScreen : public UKCActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnReadyRequested(bool bReady);

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnInviteRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnLeaveLobbyRequested();
};
