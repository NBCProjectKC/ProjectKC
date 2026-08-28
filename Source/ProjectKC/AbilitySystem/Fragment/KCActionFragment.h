#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectKC/AbilitySystem/Fragment/KCActionExecutionContext.h"
#include "UObject/Object.h"
#include "KCActionFragment.generated.h"


/** Ability Action Hook에 인라인으로 조립되는 결과 기능의 기반 클래스다. */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced)
class PROJECTKC_API UKCActionFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual bool Validate(FString& OutError) const;
	virtual bool DeclaresSetByCallerTag(FGameplayTag DataTag) const;
	virtual void AppendDeclaredSetByCallerTags(
		FGameplayTagContainer& OutTags) const;
	virtual bool CanExecute(
		const FKCActionExecutionContext& Context,
		FString& OutError) const;
	virtual bool Execute(const FKCActionExecutionContext& Context) const
		PURE_VIRTUAL(UKCActionFragment::Execute, return false;);

	/**
	 * 결과를 Hook이 지목한 대상에게 줄지, 행동한 소스에게 줄지 정한다.
	 * 같은 Hook 안에서 대상 피해와 소스 회복을 함께 조립할 수 있다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apply")
	EKCActionScope ApplicationScope = EKCActionScope::Target;

	/**
	 * true면 이 Fragment가 실패할 때 Hook 전체를 취소한다.
	 * 대가나 전제처럼 다른 Fragment와 묶인 경우에만 켠다.
	 * 그 외에는 실패한 Fragment만 건너뛴다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apply")
	bool bRequired = false;
};
