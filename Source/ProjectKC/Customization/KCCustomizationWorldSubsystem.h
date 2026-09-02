#pragma once

#include "CoreMinimal.h"
#include "Customization/KCCustomizationSaveGame.h"
#include "Subsystems/WorldSubsystem.h"
#include "KCCustomizationWorldSubsystem.generated.h"

class URuntimeMeshPaintTargetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FKCCustomizationWorldOperationCompleted,
	bool, bSucceeded,
	EKCCustomizationSaveResult, Result);

/**
 * 플레이 중인 월드에서 커스터마이징 페인트 대상을 자동으로 찾아 저장 기능을 연결합니다.
 * Blueprint 컴포넌트 추가나 이벤트 그래프 연결이 필요하지 않습니다.
 */
UCLASS()
class PROJECTKC_API UKCCustomizationWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void OnWorldEndPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintCallable, Category = "KC|Customization|Save")
	bool SaveCustomization();

	UFUNCTION(BlueprintCallable, Category = "KC|Customization|Save")
	bool LoadCustomization();

	UFUNCTION(BlueprintCallable, Category = "KC|Customization|Save")
	bool ResetCustomization();

	UFUNCTION(BlueprintPure, Category = "KC|Customization|Save")
	bool IsCustomizationAvailable() const { return IsValid(PaintTarget); }

	UFUNCTION(BlueprintPure, Category = "KC|Customization|Save")
	bool IsUsingDefaultAppearance() const { return bUseDefaultAppearance; }

	UFUNCTION(BlueprintPure, Category = "KC|Customization|Save")
	URuntimeMeshPaintTargetComponent* GetPaintTarget() const { return PaintTarget; }

	UPROPERTY(BlueprintReadOnly, Category = "KC|Customization|Save")
	bool bUseDefaultAppearance = true;

	UPROPERTY(BlueprintReadOnly, Category = "KC|Customization|Save")
	bool bLastLoadFoundSave = false;

	UPROPERTY(BlueprintReadOnly, Category = "KC|Customization|Save")
	EKCCustomizationSaveResult LastResult = EKCCustomizationSaveResult::NoSaveFound;

	UPROPERTY(BlueprintAssignable, Category = "KC|Customization|Save")
	FKCCustomizationWorldOperationCompleted OnSaveCompleted;

	UPROPERTY(BlueprintAssignable, Category = "KC|Customization|Save")
	FKCCustomizationWorldOperationCompleted OnLoadCompleted;

	UPROPERTY(BlueprintAssignable, Category = "KC|Customization|Save")
	FKCCustomizationWorldOperationCompleted OnResetCompleted;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void HandleActorsBegunPlay();
	bool ResolvePaintTarget();
	class UKCCustomizationSaveSubsystem* GetSaveSubsystem() const;
	void RegisterDevelopmentHotkeys();
	void ShowDevelopmentResult(const TCHAR* Operation, bool bSucceeded) const;

	UFUNCTION()
	void HandlePaintApplied(FRuntimeMeshPaintSampleResult PaintResult);

	void HandleSaveHotkey();
	void HandleLoadHotkey();
	void HandleResetHotkey();

	FDelegateHandle WorldBeginPlayHandle;

	UPROPERTY(Transient)
	TObjectPtr<URuntimeMeshPaintTargetComponent> PaintTarget;
};
