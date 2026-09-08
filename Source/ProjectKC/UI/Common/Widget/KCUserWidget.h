#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KCUserWidget.generated.h"

class UKCColorStyle;
class UKCUIFontStyle;
class UTextBlock;
class UWidget;

USTRUCT(BlueprintType)
struct FKCWidgetFontOverride
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Style")
	FName TextWidgetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Style")
	FName FontId;
};

UCLASS(Abstract, Blueprintable)
class PROJECTKC_API UKCUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;

	UFUNCTION(BlueprintPure, Category = "KC|UI|Style")
	UKCColorStyle* GetColorStyle() const;

	UFUNCTION(BlueprintPure, Category = "KC|UI|Style")
	UKCUIFontStyle* GetFontStyle() const;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "KC|UI|Style", meta = (DisplayName = "Apply Color Style"))
	void BP_ApplyColorStyle(UKCColorStyle* InColorStyle);

	virtual void NativeApplyColorStyle(const UKCColorStyle* InColorStyle);
	virtual void NativeApplyFontStyle(const UKCUIFontStyle* InFontStyle);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Style")
	FName DefaultFontId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|UI|Style")
	TArray<FKCWidgetFontOverride> FontOverrides;

private:
	void ApplyFontStyleToWidgetTree(const UKCUIFontStyle* InFontStyle);
	void ApplyFontStyleToWidget(UWidget* Widget, const UKCUIFontStyle* InFontStyle);
	void ApplyFontStyleToTextBlock(UTextBlock* TextBlock, const UKCUIFontStyle* InFontStyle);
	FName ResolveFontIdForTextBlock(const UTextBlock* TextBlock) const;
};
