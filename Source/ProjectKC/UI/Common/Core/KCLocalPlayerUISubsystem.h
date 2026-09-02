#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "KCLocalPlayerUISubsystem.generated.h"

class APlayerController;
class UKCUserWidget;

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

protected:
	virtual void Deinitialize() override;

private:
	APlayerController* GetOwningPlayerController() const;

	UPROPERTY(Transient)
	TObjectPtr<UKCUserWidget> ActiveScreenWidget;

	UPROPERTY(Transient)
	TArray<FText> PendingToastMessages;
};
