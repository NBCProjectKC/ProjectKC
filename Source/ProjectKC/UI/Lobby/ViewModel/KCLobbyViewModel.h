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

UCLASS(BlueprintType, Blueprintable, meta = (MVVMAllowedContextCreationType = "Manual|CreateInstance"))
class PROJECTKC_API UKCLobbyViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "KC|UI")
	const TArray<FKCLobbyPlayerViewData>& GetPlayers() const { return Players; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetPlayers(const TArray<FKCLobbyPlayerViewData>& NewPlayers);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	bool IsInviteEnabled() const { return bInviteEnabled; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetInviteEnabled(bool bNewInviteEnabled);

	UFUNCTION(BlueprintCallable, Category = "KC|UI|Preview")
	void SetPreviewData(const TArray<FKCLobbyPlayerViewData>& NewPlayers, bool bNewInviteEnabled);

private:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	TArray<FKCLobbyPlayerViewData> Players;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter = "IsInviteEnabled", Setter = "SetInviteEnabled", Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	bool bInviteEnabled = true;
};
