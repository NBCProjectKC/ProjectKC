#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Painting/RuntimeMeshPaintTargetComponent.h"
#include "KCCustomizationSaveGame.generated.h"

UENUM(BlueprintType)
enum class EKCCustomizationSaveResult : uint8
{
	Success UMETA(DisplayName = "Success"),
	NoSaveFound UMETA(DisplayName = "No Save Found"),
	InvalidPaintTarget UMETA(DisplayName = "Invalid Paint Target"),
	NoPaintData UMETA(DisplayName = "No Paint Data"),
	SaveFailed UMETA(DisplayName = "Save Failed"),
	LoadFailed UMETA(DisplayName = "Load Failed"),
	IncompatibleVersion UMETA(DisplayName = "Incompatible Version"),
	ApplyFailed UMETA(DisplayName = "Apply Failed")
};

/** 로컬 캐릭터 커스터마이징을 저장하는 단일 슬롯 데이터입니다. */
UCLASS()
class PROJECTKC_API UKCCustomizationSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentSaveVersion = 1;
	static constexpr int32 CurrentTargetSchemaVersion = 1;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "KC|Customization|Save")
	int32 SaveVersion = CurrentSaveVersion;

	/** 대상 메시 구성이나 UV가 변경될 때 올려 기존 그림을 안전하게 거부합니다. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "KC|Customization|Save")
	int32 TargetSchemaVersion = CurrentTargetSchemaVersion;

	/** 초기화 상태는 빈 패치로 표현할 수 없으므로 별도로 저장합니다. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "KC|Customization|Save")
	bool bUseDefaultAppearance = true;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "KC|Customization|Save")
	FRuntimeMeshPaintPatchHistory PaintHistory;
};
