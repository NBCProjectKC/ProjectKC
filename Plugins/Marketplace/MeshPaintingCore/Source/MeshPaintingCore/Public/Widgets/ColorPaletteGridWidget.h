// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ColorPaletteGridWidget.generated.h"

class UColorSwatchWidget;
class UUniformGridPanel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorPaletteColorSelected, FLinearColor, Color, int32, ColorIndex);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Color Palette Grid Widget"))
class MESHPAINTINGCORE_API UColorPaletteGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UColorPaletteGridWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Palette")
	TArray<FLinearColor> PaletteColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Palette", meta = (ClampMin = "1", ClampMax = "64"))
	int32 MaxColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Palette", meta = (ClampMin = "1"))
	int32 Columns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Palette", meta = (ClampMin = "8.0"))
	float SwatchSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Palette")
	TSubclassOf<UColorSwatchWidget> SwatchWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Palette")
	int32 SelectedIndex;

	UPROPERTY(BlueprintAssignable, Category = "Color Palette|Events")
	FOnColorPaletteColorSelected OnColorSelected;

	UFUNCTION(BlueprintCallable, Category = "Color Palette")
	void SetPaletteColors(const TArray<FLinearColor>& NewPaletteColors);

	UFUNCTION(BlueprintCallable, Category = "Color Palette")
	void RebuildPalette();

	UFUNCTION(BlueprintCallable, Category = "Color Palette")
	void SetSelectedIndex(int32 NewSelectedIndex, bool bBroadcast = false);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

private:
	void BuildWidgetTree();
	void UpdateSelectedStates();

	UFUNCTION()
	void HandleSwatchSelected(FLinearColor Color, int32 SwatchIndex);

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> GridPanel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UColorSwatchWidget>> Swatches;

	bool bNativeTreeBuilt;
};
