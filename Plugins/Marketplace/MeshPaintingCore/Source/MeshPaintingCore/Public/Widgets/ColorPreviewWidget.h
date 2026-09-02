// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "ColorPreviewWidget.generated.h"

class SColorPreviewSlate;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorPreviewClicked, FLinearColor, Color);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Color Preview Widget"))
class MESHPAINTINGCORE_API UColorPreviewWidget : public UWidget
{
	GENERATED_BODY()

public:
	UColorPreviewWidget();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Preview")
	FLinearColor Color;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Preview")
	bool bShowCheckerboardForAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bIsClickable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bIsSelected;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FVector2D DesiredPreviewSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0.0"))
	float CornerRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0.0"))
	float BorderThickness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor NormalBorderColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor HoveredBorderColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor PressedBorderColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FLinearColor SelectedBorderColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "1.0"))
	float CheckerCellSize;

	UPROPERTY(BlueprintAssignable, Category = "Color Preview|Events")
	FOnColorPreviewClicked OnClicked;

	UFUNCTION(BlueprintCallable, Category = "Color Preview")
	virtual void SetColor(FLinearColor NewColor);

	UFUNCTION(BlueprintCallable, Category = "Color Preview")
	virtual void SetSelected(bool bNewSelected);

	UFUNCTION(BlueprintPure, Category = "Color Preview")
	FLinearColor GetColor() const { return Color; }

	virtual void HandleSlateClicked();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

private:
	TSharedPtr<SColorPreviewSlate> MyPreview;
};
