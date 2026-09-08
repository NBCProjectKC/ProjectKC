#pragma once

#include "CoreMinimal.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "TimerManager.h"
#include "KCResultScreen.generated.h"

class UTextBlock;
class UKCResultViewModel;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCResultScreen : public UKCUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void RefreshResultScreen();

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnRestartRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI")
	void OnReturnToLobbyRequested();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "KC|UI")
	TObjectPtr<UTextBlock> BackToLobbySecondText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "KC|UI")
	TObjectPtr<UKCResultViewModel> ResultViewModel;

private:
	void StartBackToLobbyTimer();
	void StopBackToLobbyTimer();
	void UpdateBackToLobbyTimer();
	void ApplyBackToLobbyText();

	FTimerHandle BackToLobbyTimerHandle;
};
