#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "ProjectKC/AbilitySystem/Targeting/KCActionTargeting.h"
#include "KCSweepTargeting.generated.h"

/** 소스 전방을 Sweep해 대상을 수집한다. 근접 공격이 사용한다. */
UCLASS(meta = (DisplayName = "Sweep Targeting"))
class PROJECTKC_API UKCSweepTargeting : public UKCActionTargeting
{
	GENERATED_BODY()

public:
	UKCSweepTargeting();

	virtual bool Validate(FString& OutError) const override;

	virtual void GatherTargets(
		const FKCActionTargetingContext& Context,
		TArray<FKCActionTarget>& OutTargets) const override;

	/** Sweep 시작점부터 전방으로 검사할 거리다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Shape",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float SweepDistance = 180.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Shape",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float SweepRadius = 60.0f;

	/** Avatar 원점에서 전방으로 떨어진 Sweep 시작 위치다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Shape",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float StartForwardOffset = 45.0f;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Shape",
		meta = (Units = "cm"))
	float HeightOffset = 45.0f;

	/** 한 번에 수집할 최대 대상 수다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Filter",
		meta = (ClampMin = "1", ClampMax = "32", UIMin = "1", UIMax = "8"))
	int32 MaxTargets = 1;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Filter")
	TArray<TEnumAsByte<EObjectTypeQuery>> TargetObjectTypes;

	/** 벽 너머의 대상을 제외할지 결정한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Obstruction")
	bool bRequireUnobstructedPath = true;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Obstruction",
		meta = (EditCondition = "bRequireUnobstructedPath"))
	TEnumAsByte<ETraceTypeQuery> ObstructionTraceChannel;

	/** 개발용. Sweep 범위와 수집된 대상을 월드에 그린다. Shipping에서는 그리지 않는다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Debug",
		meta = (DisplayName = "Draw Debug Sweep"))
	bool bDrawDebugSweep = false;

	/** 디버그 도형이 화면에 남는 시간(초). */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Debug",
		meta = (EditCondition = "bDrawDebugSweep", ClampMin = "0.0", Units = "s"))
	float DebugDrawDuration = 1.0f;

private:
	void DrawDebugSweep(
		const UWorld& World,
		const FVector& Start,
		const FVector& End,
		const TArray<FKCActionTarget>& Targets) const;

	bool IsPathUnobstructed(
		const FKCActionTargetingContext& Context,
		const AActor& TargetActor) const;
};
