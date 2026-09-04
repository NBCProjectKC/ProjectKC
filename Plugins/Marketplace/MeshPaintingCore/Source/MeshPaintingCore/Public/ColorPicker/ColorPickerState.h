// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "MeshPaintingCoreTypes.h"
#include "UObject/Object.h"
#include "ColorPickerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorPickerStateColorChanged, FLinearColor, LinearColor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorPickerStateSettingsChanged, FMeshPaintBrushMaterialSettings, BrushSettings);

UCLASS(BlueprintType)
class MESHPAINTINGCORE_API UColorPickerState : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Picker State")
	FLinearColor CurrentLinearColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Picker State")
	FLinearColor PreviousLinearColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Picker State", meta = (ClampMin = "0.001"))
	float BrushSize = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Picker State", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Metallic = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Picker State", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Roughness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Picker State")
	bool bSRGBPreview = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Picker State")
	bool bEraserActive = false;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker State|Events")
	FOnColorPickerStateColorChanged OnColorChanged;

	UPROPERTY(BlueprintAssignable, Category = "Color Picker State|Events")
	FOnColorPickerStateSettingsChanged OnBrushSettingsChanged;

	UFUNCTION(BlueprintCallable, Category = "Color Picker State")
	void SetCurrentColor(FLinearColor NewLinearColor, bool bCapturePrevious, bool bBroadcast);

	UFUNCTION(BlueprintCallable, Category = "Color Picker State")
	void RestorePreviousColor(bool bBroadcast);

	UFUNCTION(BlueprintCallable, Category = "Color Picker State")
	void SetBrushSize(float NewBrushSize, bool bBroadcast);

	UFUNCTION(BlueprintCallable, Category = "Color Picker State")
	void SetMetallic(float NewMetallic, bool bBroadcast);

	UFUNCTION(BlueprintCallable, Category = "Color Picker State")
	void SetRoughness(float NewRoughness, bool bBroadcast);

	UFUNCTION(BlueprintCallable, Category = "Color Picker State")
	void SetSRGBPreview(bool bNewSRGBPreview);

	UFUNCTION(BlueprintCallable, Category = "Color Picker State")
	void SetEraserActive(bool bNewEraserActive, bool bBroadcast);

	UFUNCTION(BlueprintPure, Category = "Color Picker State")
	bool IsEraserActive() const { return bEraserActive; }

	UFUNCTION(BlueprintPure, Category = "Color Picker State")
	FMeshPaintBrushMaterialSettings GetBrushMaterialSettings() const;

private:
	void BroadcastSettingsIfNeeded(bool bBroadcast);
};
