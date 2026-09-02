#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ProjectKC/Item/Struct/KCItemDurabilityStruct.h"
#include "ProjectKC/Item/Struct/KCItemPresentationStruct.h"
#include "KCItemDefinition.generated.h"

class UKCAbilityDefinition;
class UTexture2D;

/** 아이템의 게임플레이상 역할이다. 자세나 장착 방식과는 독립적이다. */
UENUM(BlueprintType)
enum class EKCItemCategory : uint8
{
	Unspecified UMETA(DisplayName = "Unspecified"),
	Ingredient UMETA(DisplayName = "Ingredient"),
	Tool UMETA(DisplayName = "Tool"),
	Special UMETA(DisplayName = "Special")
};

/** 성공한 사용 뒤에도 원본 아이템 인스턴스를 유지할지 정한다. */
UENUM(BlueprintType)
enum class EKCItemUseLifecycle : uint8
{
	/** 도구·발사기처럼 사용 뒤에도 손에 남는다. */
	Persistent UMETA(DisplayName = "Persistent"),

	/** 일회용 투척물·소모품처럼 첫 성공 실행 뒤 원본이 제거된다. */
	ConsumeOnSuccessfulExecute UMETA(DisplayName = "Consume On Successful Execute")
};

/** 인벤토리와 무관한 단일 월드 아이템의 불변 구성 데이터다. */
UCLASS(BlueprintType)
class PROJECTKC_API UKCItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(
		FDataValidationContext& Context) const override;
#endif

	/** 세이브, 레시피, 드롭 테이블 등에서 사용하는 안정적인 아이템 종류 식별자다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		AssetRegistrySearchable,
		Category = "Item",
		meta = (Categories = "Item.Id"))
	FGameplayTag ItemId;

	/** 냄비 투입, UI 분류 등에서 사용하는 의미상 아이템 종류다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EKCItemCategory ItemCategory = EKCItemCategory::Unspecified;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	/** 레시피, HUD 등 UI에서 이 아이템을 나타내는 아이콘이다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Item",
		meta = (ShowOnlyInnerProperties))
	FKCItemPresentationStruct Presentation;

	/** 비어 있으면 좌클릭 사용 기능이 없는 운반 전용 아이템이다. */
	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "Item")
	TObjectPtr<UKCAbilityDefinition> UseAction;

	/** UseAction이 성공적으로 결과를 실행한 뒤 원본 아이템을 처리하는 방식이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Item|Use")
	EKCItemUseLifecycle UseLifecycle = EKCItemUseLifecycle::Persistent;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Item",
		meta = (ShowOnlyInnerProperties))
	FKCItemDurabilityStruct Durability;

	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsUsable() const;

	bool Validate(FString& OutError) const;
	
	// AssetManager 인식용 Item 타입 추가
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("Item"), GetFName());
	}
};
