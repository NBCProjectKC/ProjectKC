#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KCItemOutlineComponent.generated.h"

class UPrimitiveComponent;

UCLASS(ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCItemOutlineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKCItemOutlineComponent();

	void SetOutlineMesh(UPrimitiveComponent* InOutlineMesh);
	void ApplyDefaultOutline();
	void ApplyInteractionOutlineForTeam(int32 TeamId);
	void DisableOutline();

private:
	int32 ResolveTeamOutlineStencilValue(int32 TeamId) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Item|Outline", meta = (AllowPrivateAccess = "true"))
	bool bUseOutline = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Item|Outline", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "255"))
	int32 DefaultOutlineStencilValue = 250;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Item|Outline", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "255"))
	int32 Team0OutlineStencilValue = 252;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Item|Outline", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "255"))
	int32 Team1OutlineStencilValue = 251;

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> OutlineMesh;
};