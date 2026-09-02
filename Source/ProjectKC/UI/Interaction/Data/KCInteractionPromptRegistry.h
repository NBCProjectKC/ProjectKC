#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
#include "KCInteractionPromptRegistry.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct PROJECTKC_API FKCInteractionPromptEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Interaction", meta = (Categories = "Interaction"))
	FGameplayTag PromptTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Interaction")
	FText InputText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Interaction")
	bool bUsesInputKey = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Interaction", meta = (EditCondition = "bUsesInputKey"))
	FKey InputKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Interaction")
	FText ActionText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KC|Interaction")
	TSoftObjectPtr<UTexture2D> Icon;
};

UCLASS(BlueprintType)
class PROJECTKC_API UKCInteractionPromptRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "KC|Interaction")
	bool FindPrompt(FGameplayTag PromptTag, FKCInteractionPromptEntry& OutEntry) const;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Interaction", meta = (AllowPrivateAccess = "true", TitleProperty = "PromptTag"))
	TArray<FKCInteractionPromptEntry> PromptEntries;
};
