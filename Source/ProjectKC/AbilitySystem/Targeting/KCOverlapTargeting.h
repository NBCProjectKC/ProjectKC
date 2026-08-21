#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"
#include "KCOverlapTargeting.generated.h"

class AActor;
class UPrimitiveComponent;

/**
 * 소스의 충돌 볼륨 안에 있는 대상을 수집한다.
 * 에디터에서 그린 볼륨이 곧 판정 범위다.
 *
 * 겹침은 부피 대 부피 판정이라 볼륨에 닿기만 해도 잡히지만,
 * 그러면 캐릭터 캡슐 크기만큼(반지름·반높이) 보이는 것보다 넓어진다.
 * 그래서 수집한 뒤 대상의 중심이 볼륨 안에 있는지 다시 확인한다.
 *
 * 전방 개념이 없는 지면 함정에 쓴다.
 */
UCLASS(meta = (DisplayName = "Overlap Targeting"))
class PROJECTKC_API UKCOverlapTargeting : public UKCActionTargeting
{
	GENERATED_BODY()

public:
	UKCOverlapTargeting();

	virtual bool Validate(FString& OutError) const override;

	virtual void GatherTargets(
		const FKCActionTargetingContext& Context,
		TArray<FKCActionTarget>& OutTargets) const override;

	/** 판정에 쓸 볼륨이다. 비우면 소스의 Root Primitive를 쓴다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Filter")
	FComponentReference OverlapComponent;

	/** 수집할 Actor 종류. 비우면 겹친 모든 Actor를 수집한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Filter")
	TSubclassOf<AActor> TargetActorClass;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Filter",
		meta = (ClampMin = "1", ClampMax = "32", UIMin = "1", UIMax = "8"))
	int32 MaxTargets = 8;

private:
	UPrimitiveComponent* ResolveOverlapComponent(AActor& SourceActor) const;
};
