#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "KCKnockbackComponent.generated.h"

class UPrimitiveComponent;

/** 소스 종류와 무관한 서버 권위 넉백 요청이다. */
USTRUCT(BlueprintType)
struct PROJECTKC_API FKCKnockbackRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "KC|Knockback")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(BlueprintReadWrite, Category = "KC|Knockback")
	float HorizontalSpeed = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "KC|Knockback")
	float VerticalSpeed = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "KC|Knockback")
	bool bOverrideHorizontalVelocity = true;

	UPROPERTY(BlueprintReadWrite, Category = "KC|Knockback")
	bool bOverrideVerticalVelocity = true;

};

/** Character Launch와 물리 Velocity Change를 한 계약으로 제공하는 대상 측 Adapter다. */
UCLASS(BlueprintType, ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCKnockbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "KC|Knockback")
	bool CanApplyKnockback(const FKCKnockbackRequest& Request) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "KC|Knockback")
	bool ApplyKnockback(const FKCKnockbackRequest& Request);

protected:
	/** 비어 있으면 Owner의 Root Primitive를 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Knockback")
	FComponentReference PhysicsComponent;

private:
	UPrimitiveComponent* ResolvePhysicsComponent() const;
};
