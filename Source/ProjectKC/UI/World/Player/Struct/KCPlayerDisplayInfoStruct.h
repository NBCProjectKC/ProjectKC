#pragma once

#include "CoreMinimal.h"
#include "KCPlayerDisplayInfoStruct.generated.h"

USTRUCT(BlueprintType)
struct PROJECTKC_API FKCPlayerDisplayInfoStruct
{
	GENERATED_BODY()

public:
	FKCPlayerDisplayInfoStruct() = default;

	FKCPlayerDisplayInfoStruct(
		const FText& InDisplayName,
		int32 InTeamId,
		const FString& InUniqueNetId,
		bool bInVisible)
		: DisplayName(InDisplayName)
		, TeamId(InTeamId)
		, UniqueNetId(InUniqueNetId)
		, bVisible(bInVisible)
	{
	}

	bool operator==(const FKCPlayerDisplayInfoStruct& Other) const
	{
		return DisplayName.EqualTo(Other.DisplayName)
			&& TeamId == Other.TeamId
			&& UniqueNetId == Other.UniqueNetId
			&& bVisible == Other.bVisible;
	}

	bool operator!=(const FKCPlayerDisplayInfoStruct& Other) const
	{
		return !(*this == Other);
	}

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|UI")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|UI")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|UI")
	FString UniqueNetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KC|UI")
	bool bVisible = false;
};
