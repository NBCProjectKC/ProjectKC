// Copyright Shared Orbit 2026. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "ColorHistoryBarWidget.generated.h"

class SRuntimeColorHistoryBar;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorHistoryColorSelected, FLinearColor, Color, int32, ColorIndex);

USTRUCT(BlueprintType)
struct MESHPAINTINGCORE_API FColorHistoryTheme
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color History")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color History")
	TArray<FLinearColor> Colors;
};

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Color History Bar Widget"))
class MESHPAINTINGCORE_API UColorHistoryBarWidget : public UWidget
{
	GENERATED_BODY()

public:
	UColorHistoryBarWidget();

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Color History")
	TArray<FLinearColor> HistoryColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color History", meta = (ClampMin = "1", ClampMax = "46"))
	int32 MaxColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color History")
	bool bUseSRGB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color History")
	bool bUseAlpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color History")
	FLinearColor ActiveColor;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Color History")
	TArray<FColorHistoryTheme> SavedThemes;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Color History")
	int32 SelectedThemeIndex;

	UPROPERTY(BlueprintAssignable, Category = "Color History|Events")
	FOnColorHistoryColorSelected OnColorSelected;

	UFUNCTION(BlueprintCallable, Category = "Color History")
	void SetHistoryColors(const TArray<FLinearColor>& NewHistoryColors);

	UFUNCTION(BlueprintCallable, Category = "Color History")
	void SetUseSRGB(bool bNewUseSRGB);

	UFUNCTION(BlueprintCallable, Category = "Color History")
	void SetUseAlpha(bool bNewUseAlpha);

	UFUNCTION(BlueprintCallable, Category = "Color History")
	void SetActiveColor(FLinearColor NewActiveColor);

	UFUNCTION(BlueprintCallable, Category = "Color History")
	void SelectRecents();

	UFUNCTION(BlueprintCallable, Category = "Color History")
	void SelectTheme(int32 ThemeIndex);

	UFUNCTION(BlueprintCallable, Category = "Color History")
	void CreateNewTheme();

	UFUNCTION(BlueprintCallable, Category = "Color History")
	void RenameCurrentTheme(const FString& NewThemeName);

	UFUNCTION(BlueprintCallable, Category = "Color History")
	void DuplicateCurrentTheme();

	UFUNCTION(BlueprintCallable, Category = "Color History")
	void DeleteCurrentTheme();

	UFUNCTION(BlueprintCallable, Category = "Color History")
	void AddActiveColorToCurrentTheme();

	UFUNCTION(BlueprintCallable, Category = "Color History")
	void LoadThemesFromConfig();

	UFUNCTION(BlueprintCallable, Category = "Color History")
	void SaveThemesToConfig() const;

	void HandleColorBlockClicked(int32 ColorIndex);

	bool IsShowingRecents() const { return SelectedThemeIndex == INDEX_NONE; }
	const TArray<FLinearColor>& GetDisplayedColors() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

private:
	void RefreshHistoryBar();
	int32 FindThemeByName(const FString& ThemeName) const;
	FString MakeUniqueThemeName(const FString& ThemeName) const;

	TSharedPtr<SRuntimeColorHistoryBar> MyHistoryBar;
};
