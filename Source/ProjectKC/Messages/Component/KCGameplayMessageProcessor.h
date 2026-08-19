/**
 * @file KCGameplayMessageProcessor.h
 * @brief 게임플레이 메시지를 구독하고 수명주기에 맞춰 자동 해제 및 재가공하는 액터 컴포넌트 베이스
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "KCGameplayMessageProcessor.generated.h"

namespace EEndPlayReason { enum Type : int; }

class UObject;

/**
 * @class UKCGameplayMessageProcessor
 * @brief GameplayMessageRouter 메시지 리스너의 등록 및 해제를 액터 수명주기(BeginPlay/EndPlay)와 동기화하는 기본 컴포넌트
 */
UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCGameplayMessageProcessor : public UActorComponent
{
	GENERATED_BODY()

public:
	UKCGameplayMessageProcessor();

	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	/** @brief 메시지 리스너 등록을 시작합니다. 파생 클래스에서 오버라이드하여 리스너를 등록합니다. */
	virtual void StartListening();

	/** @brief 등록된 메시지 리스너들을 해제합니다. */
	virtual void StopListening();

protected:
	/** @brief 컴포넌트 파괴 시 자동 해제되도록 리스너 핸들을 등록합니다. */
	void AddListenerHandle(FGameplayMessageListenerHandle&& Handle);

	/** @brief 현재 서버 월드 시간을 반환합니다. */
	double GetServerTime() const;

private:
	TArray<FGameplayMessageListenerHandle> ListenerHandles;
};
