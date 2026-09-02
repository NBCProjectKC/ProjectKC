// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "ColorEyedropperButtonWidget.generated.h"

class UButton;
class UColorPreviewWidget;
class UHorizontalBox;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnColorEyedropperButtonClicked);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Color Eyedropper Button Widget"))
class MESHPAINTINGCORE_API UColorEyedropperButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UColorEyedropperButtonWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyedropper")
	FLinearColor PreviewColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyedropper")
	FSlateBrush IconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyedropper")
	FText ButtonText;

	UPROPERTY(BlueprintAssignable, Category = "Eyedropper|Events")
	FOnColorEyedropperButtonClicked OnClicked;

	UFUNCTION(BlueprintCallable, Category = "Eyedropper")
	void SetPreviewColor(FLinearColor NewPreviewColor);

	UFUNCTION(BlueprintCallable, Category = "Eyedropper")
	void SetIconBrush(const FSlateBrush& NewIconBrush);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

private:
	void BuildWidgetTree();
	void SynchronizeChildWidgets();

	UFUNCTION()
	void HandleButtonClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> Button;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> ContentBox;

	UPROPERTY(Transient)
	TObjectPtr<UColorPreviewWidget> Preview;

	UPROPERTY(Transient)
	TObjectPtr<UImage> IconImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LabelText;

	bool bNativeTreeBuilt;
};
