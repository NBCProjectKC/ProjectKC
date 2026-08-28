#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KCColorStyle.generated.h"

UCLASS(BlueprintType)
class PROJECTKC_API UKCColorStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Color")
	FLinearColor TextPrimary = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Color")
	FLinearColor TextSecondary = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Color")
	FLinearColor ScoreText = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Color")
	FLinearColor RecipeText = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Color")
	FLinearColor RecipeCheck = FLinearColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Color")
	FLinearColor PanelBackground = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Color")
	FLinearColor Warning = FLinearColor(1.0f, 0.1f, 0.1f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Color")
	TArray<FLinearColor> TeamColors;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|UI|Color")
	FLinearColor UIPointColor;
};
