#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "KCLocalPlayerUISubsystem.generated.h"

class APlayerController;
class UKCUserWidget;
class UKCEscMenuWidget;

UCLASS()
class PROJECTKC_API UKCLocalPlayerUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	UKCUserWidget* SetScreenWidget(TSubclassOf<UKCUserWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void ClearScreenWidget();

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	UKCUserWidget* SetHUDWidget(TSubclassOf<UKCUserWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void ClearHUDWidget();

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void QueueToast(const FText& Message);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	UKCEscMenuWidget* ShowEscMenu(bool bRestoreGameOnlyWhenHidden = true);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void HideEscMenu();

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void ToggleEscMenu(bool bRestoreGameOnlyWhenHidden = true);

protected:
	virtual void Deinitialize() override;

private:
	APlayerController* GetOwningPlayerController() const;

	UPROPERTY(Transient)
	TObjectPtr<UKCUserWidget> ActiveScreenWidget;

	UPROPERTY(Transient)
	TObjectPtr<UKCEscMenuWidget> ActiveEscMenuWidget;

	UPROPERTY(Transient)
	TArray<FText> PendingToastMessages;

	bool bRestoreGameOnlyOnEscMenuHidden = true;
};
