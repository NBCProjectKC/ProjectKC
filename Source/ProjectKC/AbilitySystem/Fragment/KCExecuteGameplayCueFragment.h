#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"
#include "KCExecuteGameplayCueFragment.generated.h"

/** GameplayCue 이펙트가 어느 쪽을 보고 재생될지 정한다. */
UENUM(BlueprintType)
enum class EKCGameplayCueDirectionMode : uint8
{
	/** 방향을 지정하지 않는다. HitResult가 있으면 그 Normal을, 없으면 Notify 기본 배치를 따른다. */
	FromContext,

	/** 소스 Actor의 정면이다. 손에 든 아이템에서 앞으로 뻗는 이펙트에 쓴다. */
	SourceForward,

	/** 소스 Pawn의 시선 방향이다. 상하 조준까지 따라간다. */
	SourceAim,

	/** 소스에서 대상으로 향하는 방향이다. */
	SourceToTarget,

	/** HitResult Normal을 파고드는 방향이다. */
	InverseHitNormal
};

/** 실행 문맥의 위치·표면·SourceObject를 담아 일회성 GameplayCue를 실행한다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Execute Gameplay Cue"))
class PROJECTKC_API UKCExecuteGameplayCueFragment : public UKCActionFragment
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const override;
	virtual bool SupportsDeferredExecution() const override;
	virtual bool CanExecute(
		const FKCActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FKCActionExecutionContext& Context) const override;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Cue",
		meta = (Categories = "GameplayCue"))
	FGameplayTag CueTag;

	/**
	 * Cue Notify가 이펙트를 어느 방향으로 회전시킬지 정한다.
	 * 엔진은 이 방향을 CueParameters.Normal로 받아 +X가 그쪽을 보도록 회전시키므로,
	 * Niagara System도 로컬 +X를 향해 뿜도록 만들어져 있어야 한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cue")
	EKCGameplayCueDirectionMode DirectionMode =
		EKCGameplayCueDirectionMode::FromContext;

	/** true면 방향의 상하 성분을 지워 수평으로 눕힌다. */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Cue",
		meta = (EditCondition =
			"DirectionMode != EKCGameplayCueDirectionMode::FromContext"))
	bool bFlattenDirection = false;

private:
	/** 설정한 모드로 이펙트가 바라볼 방향을 구한다. 못 구하면 0 벡터다. */
	FVector ResolveDirection(const FKCActionExecutionContext& Context) const;
};
