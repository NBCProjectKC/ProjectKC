#pragma once

#include "CoreMinimal.h"
#include "Customization/KCCustomizationSaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "KCCustomizationSaveSubsystem.generated.h"

class URuntimeMeshPaintTargetComponent;

/**
 * 로컬 커스터마이징 슬롯의 저장, 불러오기, 초기화를 담당합니다.
 * 플레이어 클래스와 독립적이며 어느 레벨의 블루프린트에서도 접근할 수 있습니다.
 */
UCLASS()
class PROJECTKC_API UKCCustomizationSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** 현재 페인트를 로컬 슬롯에 저장합니다. 초기화 상태면 bUseDefaultAppearance를 true로 전달합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Customization|Save")
	bool SaveCustomization(
		URuntimeMeshPaintTargetComponent* PaintTarget,
		bool bUseDefaultAppearance,
		EKCCustomizationSaveResult& OutResult);

	/** 로컬 슬롯을 읽어 PaintTarget에 즉시 적용합니다. 저장이 없으면 기본 외형으로 초기화합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Customization|Save")
	bool LoadCustomization(
		URuntimeMeshPaintTargetComponent* PaintTarget,
		bool& bOutSaveFound,
		bool& bOutUseDefaultAppearance,
		EKCCustomizationSaveResult& OutResult);

	/** 렌더 리소스를 만들지 않고 로컬 슬롯의 외형 종류만 확인합니다. */
	bool GetSavedAppearanceMode(
		bool& bOutSaveFound,
		bool& bOutUseDefaultAppearance,
		EKCCustomizationSaveResult& OutResult) const;

	/** 렌더 타깃과 패치 기록을 초기 상태로 되돌립니다. 파일은 지우지 않습니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Customization|Save")
	bool ResetCustomization(
		URuntimeMeshPaintTargetComponent* PaintTarget,
		EKCCustomizationSaveResult& OutResult);

	UFUNCTION(BlueprintPure, Category = "KC|Customization|Save")
	bool DoesCustomizationSaveExist() const;

	/** 로컬 저장 파일을 삭제합니다. 이미 없으면 성공으로 처리합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Customization|Save")
	bool DeleteCustomizationSave(EKCCustomizationSaveResult& OutResult);

	UFUNCTION(BlueprintPure, Category = "KC|Customization|Save")
	FString GetCustomizationSlotName() const;

private:
	static const FString CustomizationSlotName;
	static constexpr int32 CustomizationUserIndex = 0;
};
