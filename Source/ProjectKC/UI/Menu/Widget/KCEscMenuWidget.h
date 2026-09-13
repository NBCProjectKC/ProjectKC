#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "KCEscMenuWidget.generated.h"

class UButton;
class UKCToastButtonWidget;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCEscMenuWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UButton* ResolveButton(const TArray<FName>& CandidateNames) const;
	void ShowConfirmToast(const FText& Message, FSimpleDelegate PositiveAction);
	void ReturnToMainMenu();
	void QuitGame();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

	UFUNCTION()
	void HandleOptionClicked();

	UFUNCTION()
	void HandleExitClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> MainMenuButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> OptionButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ExitButton;

	UPROPERTY(Transient)
	TObjectPtr<UKCToastButtonWidget> ActiveConfirmToast;
};