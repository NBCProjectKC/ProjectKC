#pragma once

#include "CoreMinimal.h"
#include "KCItemPresentationStruct.generated.h"

class UStaticMesh;

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

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "World",
		meta = (CollisionProfileName = true))
	FName WorldCollisionProfile = TEXT("PhysicsActor");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	bool bSimulatePhysicsInWorld = true;

	/**
	 * Holder의 Hand 소켓에 Item Root를 붙인 뒤 적용할 상대 Transform을 만든다.
	 * 결과 Transform은 이 Presentation의 Grip 소켓을 Hand 소켓과 일치시킨다.
	 */
	bool TryGetGripAlignmentTransform(FTransform& OutTransform) const;

	bool Validate(FString& OutError) const;
};
