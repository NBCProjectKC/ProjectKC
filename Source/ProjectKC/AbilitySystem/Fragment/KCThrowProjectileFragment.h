#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionFragment.h"
#include "ProjectKC/AbilitySystem/Struct/KCProjectileConfigStruct.h"
#include "KCThrowProjectileFragment.generated.h"

/** OnExecute에서 투사체를 서버 권위로 생성하는 일회성 결과 Fragment다. */
UCLASS(EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Throw Projectile"))
class PROJECTKC_API UKCThrowProjectileFragment : public UKCActionFragment
{
	GENERATED_BODY()

public:
	UKCThrowProjectileFragment();

	virtual bool Validate(FString& OutError) const override;
	virtual bool DeclaresSetByCallerTag(FGameplayTag DataTag) const override;
	virtual void AppendDeclaredSetByCallerTags(
		FGameplayTagContainer& OutTags) const override;
	virtual bool CanExecute(
		const FKCActionExecutionContext& Context,
		FString& OutError) const override;
	virtual bool Execute(const FKCActionExecutionContext& Context) const override;

	/** 실제 투척과 로컬 궤적 미리보기가 공유하는 발사 위치·속도 계산이다. */
	bool BuildLaunchSolution(
		const AActor* SourceActor,
		const AActor* LaunchOrigin,
		float ChargeAlpha,
		FTransform& OutSpawnTransform,
		FVector& OutInitialVelocity) const;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Projectile",
		meta = (ShowOnlyInnerProperties))
	FKCProjectileLaunchConfigStruct LaunchConfig;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Projectile",
		meta = (ShowOnlyInnerProperties))
	FKCProjectileExplosionConfigStruct ExplosionConfig;

	/** 폭발 반경의 각 대상에게 지연 실행할 Target 전용 Fragment 목록이다. */
	UPROPERTY(
		EditDefaultsOnly,
		Instanced,
		BlueprintReadOnly,
		Category = "Projectile|Target Effects")
	TArray<TObjectPtr<UKCActionFragment>> ExplosionTargetFragments;

private:
	AActor* ResolveLaunchOrigin(const FKCActionExecutionContext& Context) const;
	UObject* ResolveEffectSourceObject(
		const FKCActionExecutionContext& Context,
		AActor* LaunchOrigin) const;
};
