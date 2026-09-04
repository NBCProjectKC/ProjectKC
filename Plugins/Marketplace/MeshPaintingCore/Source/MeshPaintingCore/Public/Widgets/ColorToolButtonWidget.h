// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Styling/SlateBrush.h"
#include "ColorToolButtonWidget.generated.h"

class SColorToolButtonSlate;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorToolButtonClicked, FName, ToolId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorToolButtonToggled, FName, ToolId, bool, bIsChecked);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Color Tool Button Widget"))
class MESHPAINTINGCORE_API UColorToolButtonWidget : public UWidget
{
	GENERATED_BODY()

public:
	UColorToolButtonWidget();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Button")
	FName ToolId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Button")
	FSlateBrush IconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Button")
	bool bIsToggle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tool Button")
	bool bIsChecked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector2D ButtonSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0.0"))
	float CornerRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor NormalColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor HoveredColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor PressedColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor CheckedColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor DisabledColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor BorderColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor IconTint;

	UPROPERTY(BlueprintAssignable, Category = "Tool Button|Events")
	FOnColorToolButtonClicked OnClicked;

	UPROPERTY(BlueprintAssignable, Category = "Tool Button|Events")
	FOnColorToolButtonToggled OnToggled;

	UFUNCTION(BlueprintCallable, Category = "Tool Button")
	void SetIconBrush(const FSlateBrush& NewIconBrush);

	UFUNCTION(BlueprintCallable, Category = "Tool Button")
	void SetToggleMode(bool bNewIsToggle);

	UFUNCTION(BlueprintCallable, Category = "Tool Button")
	void SetIsChecked(bool bNewIsChecked, bool bBroadcast = false);

	UFUNCTION(BlueprintPure, Category = "Tool Button")
	bool GetIsChecked() const { return bIsChecked; }

	void HandleSlateClicked();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

private:
	TSharedPtr<SColorToolButtonSlate> MyButton;
};
