#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectKC/Item/Struct/KCItemPresentationStruct.h"
#include "KCItemDefinition.generated.h"

class UKCAbilityDefinition;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Item")
	FKCItemPresentationStruct Presentation;

	/** 비어 있으면 좌클릭 사용 기능이 없는 운반 전용 아이템이다. */
	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "KC|Item|Use")
	TObjectPtr<UKCAbilityDefinition> UseAction;

	UFUNCTION(BlueprintPure, Category = "KC|Item")
	bool IsUsable() const;

	bool Validate(FString& OutError) const;
};
