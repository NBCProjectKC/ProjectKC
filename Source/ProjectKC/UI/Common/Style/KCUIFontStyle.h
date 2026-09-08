#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Fonts/SlateFontInfo.h"
#include "KCUIFontStyle.generated.h"

UCLASS(BlueprintType)
class PROJECTKC_API UKCUIFontStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "KC|UI|Font")
	bool HasDefaultFontOverride() const;

	UFUNCTION(BlueprintPure, Category = "KC|UI|Font")
	bool FindFont(FName FontId, FSlateFontInfo& OutFont) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Font")
	FSlateFontInfo DefaultFont;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Font")
	TMap<FName, FSlateFontInfo> Fonts;

private:
	static bool HasFontOverride(const FSlateFontInfo& Font);
};
