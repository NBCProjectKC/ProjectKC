// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/Object.h"
#include "ColorPickerPalette.generated.h"

UCLASS(BlueprintType)
class MESHPAINTINGCORE_API UColorPickerPaletteDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Palette")
	TArray<FLinearColor> DefaultColors;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorPickerPaletteChanged, const TArray<FLinearColor>&, Colors);

UCLASS(BlueprintType)
class MESHPAINTINGCORE_API UColorPickerPaletteStorage : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Palette")
	TObjectPtr<UColorPickerPaletteDataAsset> PaletteDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Palette")
	FString SaveSlotName = TEXT("MeshPaintingCore_ColorPalette");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Palette")
	int32 UserIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Palette", meta = (ClampMin = "1", ClampMax = "46"))
	int32 MaxColors = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Palette")
	bool bUseFallbackColorsWhenEmpty = true;

	UPROPERTY(BlueprintAssignable, Category = "Color Palette|Events")
	FOnColorPickerPaletteChanged OnPaletteChanged;

	UFUNCTION(BlueprintCallable, Category = "Color Palette")
	void Initialize(UColorPickerPaletteDataAsset* InPaletteDataAsset, const FString& InSaveSlotName);

	UFUNCTION(BlueprintCallable, Category = "Color Palette")
	void LoadCustomColors();

	UFUNCTION(BlueprintCallable, Category = "Color Palette")
	void SaveCustomColors() const;

	UFUNCTION(BlueprintCallable, Category = "Color Palette")
	void AddCustomColor(FLinearColor Color);

	UFUNCTION(BlueprintCallable, Category = "Color Palette")
	void AddRecentColor(FLinearColor Color);

	UFUNCTION(BlueprintCallable, Category = "Color Palette")
	void RemoveCustomColorAt(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Color Palette")
	void RestoreDefaultPalette();

	UFUNCTION(BlueprintPure, Category = "Color Palette")
	TArray<FLinearColor> GetCustomColors() const { return CustomColors; }

	UFUNCTION(BlueprintPure, Category = "Color Palette")
	TArray<FLinearColor> GetCombinedColors() const;

	static TArray<FLinearColor> GetFallbackDefaultColors();

private:
	UPROPERTY()
	TArray<FLinearColor> CustomColors;

	void TrimToMaxColors();
	TArray<FLinearColor> GetInitialColors() const;
	void BroadcastPalette();
};
