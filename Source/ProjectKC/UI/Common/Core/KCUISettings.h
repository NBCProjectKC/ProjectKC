#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "KCUISettings.generated.h"

class UKCColorStyle;
class UKCHUDWidget;
class UKCToastWidget;
class UKCModalWidget;
class UKCWorldIndicatorWidget;
class UKCInteractionPromptWidget;
class UKCUserWidget;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ProjectKC UI"))
class PROJECTKC_API UKCUISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Style")
	TSoftObjectPtr<UKCColorStyle> DefaultColorStyle;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Screens")
	TSoftClassPtr<UKCUserWidget> LoadingScreenClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Screens")
	TSoftClassPtr<UKCUserWidget> MainMenuScreenClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Screens")
	TSoftClassPtr<UKCUserWidget> LobbyScreenClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Screens")
	TSoftClassPtr<UKCUserWidget> PauseMenuScreenClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Screens")
	TSoftClassPtr<UKCUserWidget> SettingsScreenClass;

	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Screens")
	TSoftClassPtr<UKCUserWidget> ResultScreenClass;

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
};
