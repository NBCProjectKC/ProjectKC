#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KCEmoteComponent.generated.h"

class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FKCEmotePlayedSignature,
	int32,
	EmoteIndex,
	UAnimMontage*,
	Montage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKCEmoteStoppedSignature);

/**
 * 플레이어가 선택한 감정표현 인덱스를 서버가 검증하고 모든 클라이언트에 재생한다.
 * 네트워크에는 에셋 포인터 대신 작은 인덱스만 전달한다.
 */
UCLASS(ClassGroup = (KC), meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCEmoteComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKCEmoteComponent();

	UFUNCTION(BlueprintCallable, Category = "KC|Emote")
	bool RequestPlayEmote(int32 EmoteIndex);

	/** 몽타주 풀의 다음 유효한 감정표현을 서버 승인 순서대로 요청한다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Emote")
	bool RequestPlayNextEmote();

	UFUNCTION(BlueprintCallable, Category = "KC|Emote")
	void RequestStopEmote(float BlendOutTime = 0.2f);

	UFUNCTION(BlueprintPure, Category = "KC|Emote")
	int32 GetEmoteCount() const;

	UFUNCTION(BlueprintPure, Category = "KC|Emote")
	bool IsPlayingEmote() const;

	UPROPERTY(BlueprintAssignable, Category = "KC|Emote")
	FKCEmotePlayedSignature OnEmotePlayed;

	UPROPERTY(BlueprintAssignable, Category = "KC|Emote")
	FKCEmoteStoppedSignature OnEmoteStopped;

protected:
	UFUNCTION(Server, Reliable)
	void ServerPlayEmote(int32 EmoteIndex);

	UFUNCTION(Server, Reliable)
	void ServerPlayNextEmote();

	UFUNCTION(Server, Reliable)
	void ServerStopEmote(float BlendOutTime);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayEmote(int32 EmoteIndex);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopEmote(float BlendOutTime);

private:
	bool IsRequestOwnerAllowed() const;
	bool TryPlayEmoteOnServer(int32 EmoteIndex);
	bool CanServerAcceptEmote(int32 EmoteIndex) const;
	bool IsConfiguredEmote(int32 EmoteIndex) const;
	int32 FindNextConfiguredEmoteIndex() const;
	void PlayEmoteLocal(int32 EmoteIndex);
	void StopEmoteLocal(float BlendOutTime);

	UPROPERTY(EditDefaultsOnly, Category = "KC|Emote")
	TArray<TObjectPtr<UAnimMontage>> EmoteMontages;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Emote", meta = (ClampMin = "0.01"))
	float MinimumRequestInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "KC|Emote")
	bool bBlockWhenAnyMontagePlaying = true;

	/** 서버가 다음에 승인할 몽타주 풀 인덱스다. */
	int32 NextEmoteIndex = 0;
	int32 ActiveEmoteIndex = INDEX_NONE;
	double LastAcceptedRequestTimeSeconds = -1.0;
};
