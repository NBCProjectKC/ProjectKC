#pragma once

#include "CoreMinimal.h"
#include "KCItemDurabilityStruct.generated.h"

class UNiagaraSystem;
class USoundBase;

/** 아이템 한 번의 사용 수명주기에서 내구도를 소모하는 시점이다. */
UENUM(BlueprintType)
enum class EKCItemDurabilityConsumeMode : uint8
{
	/** 내구도를 사용하지 않는다. */
	None UMETA(DisplayName = "None"),

	/** Ability 사용이 서버에서 확정될 때 한 번 소모한다. */
	OnUse UMETA(DisplayName = "On Use"),

	/** 한 번의 사용에서 공격 가능한 대상을 처음 명중했을 때 한 번 소모한다. */
	OnFirstHit UMETA(DisplayName = "On First Hit"),

	/** Ability가 활성화된 실제 시간에 비례해 초당 소모한다. */
	WhileActive UMETA(DisplayName = "While Active")
};

/** 내구도가 0이 되었을 때 아이템 Actor를 처리하는 방식이다. */
UENUM(BlueprintType)
enum class EKCItemBreakBehavior : uint8
{
	/** Actor를 남겨 파손 상태 표시나 수리 기능에 사용할 수 있게 한다. */
	RemainBroken UMETA(DisplayName = "Remain Broken"),

	/** 파괴 연출을 재생한 뒤 서버가 Actor를 제거한다. */
	Destroy UMETA(DisplayName = "Destroy")
};

/** Item Definition에 저작하는 불변 내구도 소모 규칙이다. */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCItemDurabilityStruct
{
	GENERATED_BODY()

	static constexpr float MaximumDurability = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Durability")
	EKCItemDurabilityConsumeMode ConsumeMode =
		EKCItemDurabilityConsumeMode::None;

	/**
	 * OnUse/OnFirstHit에서는 1회 소모량, WhileActive에서는 초당 소모량이다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Durability",
		meta = (
			EditCondition = "ConsumeMode != EKCItemDurabilityConsumeMode::None",
			EditConditionHides,
			ClampMin = "0.001",
			ClampMax = "100.0",
			UIMin = "1.0",
			UIMax = "100.0"))
	float ConsumeAmount = 0.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Durability",
		meta = (
			EditCondition = "ConsumeMode != EKCItemDurabilityConsumeMode::None",
			EditConditionHides))
	EKCItemBreakBehavior BreakBehavior =
		EKCItemBreakBehavior::RemainBroken;

	/** Destroy일 때 파괴 위치에서 한 번 재생할 사운드다. 비워도 된다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Durability|Break Effects",
		meta = (
			EditCondition = "ConsumeMode != EKCItemDurabilityConsumeMode::None && BreakBehavior == EKCItemBreakBehavior::Destroy",
			EditConditionHides))
	TObjectPtr<USoundBase> BreakSound;

	/** Destroy일 때 파괴 위치에 한 번 스폰할 Niagara 시스템이다. 비워도 된다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Durability|Break Effects",
		meta = (
			EditCondition = "ConsumeMode != EKCItemDurabilityConsumeMode::None && BreakBehavior == EKCItemBreakBehavior::Destroy",
			EditConditionHides))
	TObjectPtr<UNiagaraSystem> BreakVFX;

	bool IsEnabled() const;
	bool Validate(FString& OutError) const;
};
