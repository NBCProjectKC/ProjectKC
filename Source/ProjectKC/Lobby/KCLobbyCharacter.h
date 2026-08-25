/**
 * @file KCLobbyCharacter.h
 * @brief 로비 슬롯에 스폰되어 플레이어 외형 및 머리 위 UI(닉네임, 레디 상태)를 표시하는 로비 캐릭터 클래스
 */

#pragma once

#include "CoreMinimal.h"
#include "ProjectKC/Player/KCPlayerCharacter.h"
#include "ProjectKC/Lobby/Struct/KCPlayerInfoStruct.h"
#include "KCLobbyCharacter.generated.h"

class UWidgetComponent;
class UKCPlayerInfoWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKCLobbyPlayerInfoUpdatedDelegate, const FKCPlayerInfoStruct&, NewInfo);

/**
 * @class AKCLobbyCharacter
 * @brief 기존 BP_Lobby_PlayerCharacter를 1:1 매핑한 C++ 로비 캐릭터 클래스 (AKCPlayerCharacter 상속)
 */
UCLASS()
class PROJECTKC_API AKCLobbyCharacter : public AKCPlayerCharacter
{
	GENERATED_BODY()

public:
	AKCLobbyCharacter();

	//~AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface

	/** @brief 서버에서 슬롯 배정 시 플레이어 정보를 갱신합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Lobby")
	void UpdatePlayerInfo(const FKCPlayerInfoStruct& InNewInfo);

	/** @brief 현재 배정된 플레이어 정보를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "KC|Lobby")
	const FKCPlayerInfoStruct& GetPlayerInfo() const { return PlayerInfo; }

public:
	/** @brief 플레이어 정보가 갱신되었을 때 발생하는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "KC|Lobby|Events")
	FOnKCLobbyPlayerInfoUpdatedDelegate OnPlayerInfoUpdated;

protected:
	//~AActor interface
	virtual void BeginPlay() override;
	//~End of AActor interface

	/** @brief 네트워크 복제되는 플레이어 정보 구조체 */
	UPROPERTY(ReplicatedUsing = OnRep_PlayerInfo, EditAnywhere, BlueprintReadWrite, Category = "KC|Lobby")
	FKCPlayerInfoStruct PlayerInfo;

	/** @brief PlayerInfo 프로퍼티 복제 수신 시 호출 */
	UFUNCTION()
	virtual void OnRep_PlayerInfo();

	/** @brief 머리 위 3D 위젯 컴포넌트(KCPlayerInfoWidget)의 텍스트 및 가시성을 갱신합니다. */
	void RefreshPlayerInfoWidget();
};
