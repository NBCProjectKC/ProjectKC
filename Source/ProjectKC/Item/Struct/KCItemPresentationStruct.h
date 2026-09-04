#pragma once

#include "CoreMinimal.h"
#include "KCItemPresentationStruct.generated.h"

class UStaticMesh;

/** 아이템을 들고 있는 동안 AnimBP가 선택할 공용 자세 키다. */
UENUM(BlueprintType)
enum class EKCHeldPose : uint8
{
	Default UMETA(DisplayName = "Default"),
	IngredientCarry UMETA(DisplayName = "Ingredient Carry"),
	OneHandTool UMETA(DisplayName = "One Hand Tool"),
	TwoHandTool UMETA(DisplayName = "Two Hand Tool"),
	ThrowReady UMETA(DisplayName = "Throw Ready")
};

/** 월드와 손에서 아이템을 표현하는 데 필요한 공통 데이터다. */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCItemPresentationStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	TObjectPtr<UStaticMesh> StaticMesh;

	/**
	 * 이 메시 소켓이 Holder의 Hand 소켓과 일치하도록 아이템을 정렬한다.
	 * 선택 사항이다. 소켓이 없으면 아이템 원점을 Hand 소켓에 맞춘다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	FName GripSocketName = TEXT("Grip");

	/**
	 * 이 아이템을 Holder의 어느 소켓에 붙일지 덮어쓴다.
	 * 비어 있으면 Holder의 HeldItemComponent가 정한 HandSocketName을 쓴다.
	 * 등에 메거나 허리에 차는 등 손이 아닌 곳에 붙는 아이템에만 지정한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	FName HolderSocketNameOverride = NAME_None;

	/** 실제 Anim Sequence는 AnimBP가 이 키에 매핑한다. 아이템에는 자세 종류만 저장한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Animation")
	EKCHeldPose HeldPose = EKCHeldPose::Default;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "World",
		meta = (CollisionProfileName = true))
	FName WorldCollisionProfile = TEXT("KCWorldItem");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	bool bSimulatePhysicsInWorld = true;

	/**
	 * Holder의 Hand 소켓에 Item Root를 붙인 뒤 적용할 상대 Transform을 만든다.
	 * 결과 Transform은 이 Presentation의 Grip 소켓을 Hand 소켓과 일치시킨다.
	 */
	bool TryGetGripAlignmentTransform(FTransform& OutTransform) const;

	bool Validate(FString& OutError) const;
};
