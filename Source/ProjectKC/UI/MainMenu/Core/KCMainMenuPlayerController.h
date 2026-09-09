#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "KCMainMenuPlayerController.generated.h"

class UUserWidget;

UCLASS()
class PROJECTKC_API AKCMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void InitializeMainMenuInput();
	void ApplyMainMenuCamera();
	void ShowSplashScreen();
	void CheckPendingSessionNotification();
	void ClearMainMenuUI();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveSplashScreen;
};
