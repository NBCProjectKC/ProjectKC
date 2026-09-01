#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/UI/Common/Core/KCViewModelBase.h"
#include "ProjectKC/UI/World/Player/Struct/KCPlayerDisplayInfoStruct.h"
#include "KCPlayerOverHeadViewModel.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FKCPlayerOverHeadNameChangedNativeDelegate, const FText&);
DECLARE_MULTICAST_DELEGATE_OneParam(FKCPlayerOverHeadTeamIdChangedNativeDelegate, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FKCPlayerOverHeadVisibilityChangedNativeDelegate, bool);

UCLASS(BlueprintType, Blueprintable, meta = (MVVMAllowedContextCreationType = "Manual|CreateInstance"))
class PROJECTKC_API UKCPlayerOverHeadViewModel : public UKCViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "KC|UI")
	const FText& GetPlayerName() const { return PlayerName; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetPlayerName(const FText& NewPlayerName);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	int32 GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetTeamId(int32 NewTeamId);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	bool IsOverHeadVisible() const { return bOverHeadVisible; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetOverHeadVisible(bool bNewVisible);

	UFUNCTION(BlueprintPure, Category = "KC|UI")
	const FKCPlayerDisplayInfoStruct& GetPlayerDisplayInfo() const { return PlayerDisplayInfo; }

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void SetPlayerDisplayInfo(const FKCPlayerDisplayInfoStruct& NewDisplayInfo);

	UFUNCTION(BlueprintCallable, Category = "KC|UI")
	void ClearPlayerDisplayInfo();

	FKCPlayerOverHeadNameChangedNativeDelegate OnPlayerNameChangedNative;
	FKCPlayerOverHeadTeamIdChangedNativeDelegate OnTeamIdChangedNative;
	FKCPlayerOverHeadVisibilityChangedNativeDelegate OnVisibilityChangedNative;

private:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FKCPlayerDisplayInfoStruct PlayerDisplayInfo;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	FText PlayerName;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter, Setter, Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	int32 TeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Getter = "IsOverHeadVisible", Setter = "SetOverHeadVisible", Category = "KC|UI", meta = (AllowPrivateAccess = "true"))
	bool bOverHeadVisible = false;
};
