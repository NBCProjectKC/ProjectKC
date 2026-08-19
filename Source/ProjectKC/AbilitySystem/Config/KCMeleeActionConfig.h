#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "ProjectKC/AbilitySystem/Config/KCActionConfig.h"
#include "KCMeleeActionConfig.generated.h"

/** 서버가 전방 근접 Sweep을 수행하는 데에만 필요한 설정이다. */
UCLASS(
	BlueprintType,
	EditInlineNew,
	DefaultToInstanced,
	meta = (DisplayName = "Melee Sweep Config"))
class PROJECTKC_API UKCMeleeActionConfig : public UKCActionConfig
{
	GENERATED_BODY()

public:
	UKCMeleeActionConfig();

	virtual bool Validate(FString& OutError) const override;

	/** Sweep 시작점부터 전방으로 검사할 거리다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Melee|Sweep",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float SweepDistance = 180.0f;

	/** 근접 판정에 사용하는 구의 반지름이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Melee|Sweep",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float SweepRadius = 60.0f;

	/** Avatar 원점에서 전방으로 떨어진 Sweep 시작 위치다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Melee|Sweep",
		meta = (ClampMin = "0.0", UIMin = "0.0", Units = "cm"))
	float StartForwardOffset = 45.0f;

	/** Avatar 원점에서 Sweep 중심을 올릴 높이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Melee|Sweep",
		meta = (Units = "cm"))
	float HeightOffset = 45.0f;

	/** 한 번의 공격으로 Action Hook 실행에 성공할 수 있는 최대 대상 수다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Melee|Target",
		meta = (ClampMin = "1", ClampMax = "32", UIMin = "1", UIMax = "8"))
	int32 MaxTargets = 1;

	/** Sweep이 검색할 Collision Object Type 목록이다. 기본값은 Pawn이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Melee|Target")
	TArray<TEnumAsByte<EObjectTypeQuery>> TargetObjectTypes;

	/** 벽과 같은 차폐물 너머의 Sweep 대상을 제외할지 결정한다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Melee|Obstruction")
	bool bRequireUnobstructedPath = true;

	/** 차폐 검사에 사용할 Trace Channel이다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "KC|Melee|Obstruction",
		meta = (EditCondition = "bRequireUnobstructedPath"))
	TEnumAsByte<ETraceTypeQuery> ObstructionTraceChannel;
};
