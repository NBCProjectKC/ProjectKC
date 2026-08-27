#pragma once

#include "CoreMinimal.h"
#include "KCAvatarTeamAppearanceStruct.generated.h"

class UMaterialInterface;

USTRUCT(BlueprintType)
struct PROJECTKC_API FKCAvatarTeamAppearanceStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Avatar")
	int32 TeamId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Avatar")
	TObjectPtr<UMaterialInterface> BodyMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Avatar")
	TObjectPtr<UMaterialInterface> HandMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Avatar")
	TObjectPtr<UMaterialInterface> FootMaterial;
};
