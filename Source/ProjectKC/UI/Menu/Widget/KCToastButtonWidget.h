#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Widget/KCUserWidget.h"
#include "KCToastButtonWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCToastButtonWidget : public UKCUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void Configure(const FText& Message, FSimpleDelegate InPositiveAction);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UTextBlock> ContentText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UButton> PositiveButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "KC|UI")
	TObjectPtr<UButton> NegativeButton;

private:
	UFUNCTION()
	void HandlePositiveClicked();

	UFUNCTION()
	void HandleNegativeClicked();

	FSimpleDelegate PositiveAction;
};