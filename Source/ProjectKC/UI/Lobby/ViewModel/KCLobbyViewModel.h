#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "KCLobbyViewModel.generated.h"

USTRUCT(BlueprintType)
struct FKCLobbyPlayerViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bReady = false;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	int32 TeamId = INDEX_NONE;
};

UCLASS(BlueprintType)
class PROJECTKC_API UKCLobbyViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	TArray<FKCLobbyPlayerViewData> Players;

	UPROPERTY(BlueprintReadWrite, Category = "KC|UI")
	bool bInviteEnabled = true;
};
