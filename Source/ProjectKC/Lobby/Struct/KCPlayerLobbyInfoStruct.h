#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/Lobby/Enum/KCLobbySlotStateType.h"
#include "KCPlayerLobbyInfoStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTKC_API FKCPlayerLobbyInfoStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	FString PlayerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	FString UniqueNetIdStr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	int32 SlotIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	int32 TeamId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	bool bIsHost = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	bool bIsReady = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	EKCLobbySlotStateType SlotState = EKCLobbySlotStateType::Empty;
};
