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
	 * @param bPersistAcrossLevelTravel true면 레벨 파괴에도 이 위젯은 자동으로 안 사라짐
	 * (로딩화면처럼 레벨 트래블 구간 내내 떠 있어야 하는 위젯 전용)
	 */
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	UKCUserWidget* SetScreenWidget(TSubclassOf<UKCUserWidget> WidgetClass, bool bPersistAcrossLevelTravel = false);
	
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void ClearScreenWidget();

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
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
	TObjectPtr<UKCUserWidget> ActiveScreenWidget;

	UPROPERTY(Transient)
	TArray<FText> PendingToastMessages;
};
