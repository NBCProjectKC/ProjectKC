#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "KCUISettings.generated.h"

class UKCActivatableWidget;
class UKCHUDWidget;
class UKCPrimaryGameLayout;
class UKCToastWidget;
class UKCModalWidget;
class UKCWorldIndicatorWidget;
class UKCInteractionPromptWidget;
class UDataTable;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ProjectKC UI"))
class PROJECTKC_API UKCUISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Layout")
	TSoftClassPtr<UKCPrimaryGameLayout> PrimaryGameLayoutClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Screens")
	TSoftClassPtr<UKCActivatableWidget> LoadingScreenClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Screens")
	TSoftClassPtr<UKCActivatableWidget> MainMenuScreenClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Screens")
	TSoftClassPtr<UKCActivatableWidget> LobbyScreenClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Screens")
	TSoftClassPtr<UKCActivatableWidget> PauseMenuScreenClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Screens")
	TSoftClassPtr<UKCActivatableWidget> SettingsScreenClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Screens")
	TSoftClassPtr<UKCActivatableWidget> ResultScreenClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|HUD")
	TSoftClassPtr<UKCHUDWidget> HUDWidgetClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Dialog")
	TSoftClassPtr<UKCModalWidget> ModalDialogClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Toast")
	TSoftClassPtr<UKCToastWidget> ToastWidgetClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Indicator")
	TSoftClassPtr<UKCWorldIndicatorWidget> WorldIndicatorWidgetClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Indicator")
	TSoftClassPtr<UKCInteractionPromptWidget> InteractionPromptWidgetClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|HUD")
	TSoftObjectPtr<UDataTable> RecipeDataTable;
};
