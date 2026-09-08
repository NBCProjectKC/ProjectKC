#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KCMainMenu3DManager.generated.h"

class ACameraActor;
class AKCMainMenu3DButton;
class UKCUserWidget;

UCLASS()
class PROJECTKC_API AKCMainMenu3DManager : public AActor
{
	GENERATED_BODY()

public:
	AKCMainMenu3DManager();

	virtual void BeginPlay() override;

	void NotifyButtonHovered(AKCMainMenu3DButton* Button);
	void NotifyButtonUnhovered(AKCMainMenu3DButton* Button);
	void HandleButtonClicked(AKCMainMenu3DButton* Button);

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "KC|MainMenu")
	TObjectPtr<ACameraActor> MainMenuCamera;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "KC|MainMenu")
	TArray<TObjectPtr<AKCMainMenu3DButton>> MenuButtons;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|MainMenu")
	TSubclassOf<UKCUserWidget> OptionWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|MainMenu|Session")
	int32 DefaultPublicConnections = 6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|MainMenu|Session")
	bool bCreateLANSession = false;

private:
	UPROPERTY()
	TObjectPtr<AKCMainMenu3DButton> CurrentHoveredButton;

	FTimerHandle CameraSetupRetryTimerHandle;
	int32 RemainingCameraSetupAttempts = 0;

	void InitializeMainMenu();
	bool TrySetMainMenuCamera();
	void RetrySetMainMenuCamera();
	void InitializeButtons();
	void OpenCreateLobbyUI();
	void OpenOptionUI();
	void ExitGame();
	bool ShowScreenWidget(TSubclassOf<UKCUserWidget> WidgetClass);
	APlayerController* ResolvePlayerController() const;
};
