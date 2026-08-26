#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "KCLocalPlayerUISubsystem.generated.h"

class UCommonActivatableWidget;
class APlayerController;
class UKCPrimaryGameLayout;
class UKCUISettings;
class UKCUserWidget;
class UKCToastWidget;

UCLASS()
class PROJECTKC_API UKCLocalPlayerUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetPrimaryGameLayout(UKCPrimaryGameLayout* InPrimaryGameLayout);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	UKCPrimaryGameLayout* EnsurePrimaryGameLayout();

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	UKCPrimaryGameLayout* GetPrimaryGameLayout() const { return PrimaryGameLayout; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	UCommonActivatableWidget* PushWidgetToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	UKCUserWidget* SetHUDWidget(TSubclassOf<UKCUserWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void ClearHUDWidget();

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void QueueToast(const FText& Message);

protected:
	virtual void Deinitialize() override;

private:
	APlayerController* GetOwningPlayerController() const;

	UPROPERTY(Transient)
	TObjectPtr<UKCPrimaryGameLayout> PrimaryGameLayout;

	UPROPERTY(Transient)
	TObjectPtr<UKCUserWidget> ActiveHUDWidget;

	UPROPERTY(Transient)
	TArray<FText> PendingToastMessages;
};
