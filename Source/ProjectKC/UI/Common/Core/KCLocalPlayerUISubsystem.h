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
	/**
	* @param bPersistAcrossLevelTravel =true 일 때, 레벨 파괴에도 위젯이 파괴되지 않음
	*/
	UKCUserWidget* SetHUDWidget(TSubclassOf<UKCUserWidget> WidgetClass, bool bPersistAcrossLevelTravel = false);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void ClearHUDWidget();

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void QueueToast(const FText& Message);

protected:
	virtual void Deinitialize() override;

private:
	APlayerController* GetOwningPlayerController() const;

	UPROPERTY(Transient)
	TObjectPtr<UKCUserWidget> ActiveHUDWidget;

	UPROPERTY(Transient)
	TArray<FText> PendingToastMessages;
};
