#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "KCPlayerController.generated.h"

struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
class UKCCustomizationNetworkComponent;
struct FGameplayTag;
struct FKCEmptyMessageStruct;
struct FKCGamePhaseChangedStruct;

UCLASS()
class PROJECTKC_API AKCPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AKCPlayerController();

	virtual void ReceivedPlayer() override;
	UFUNCTION(BlueprintPure, Category = "KC|Network")
	float GetServerTime() const; 

	UKCCustomizationNetworkComponent* GetCustomizationNetworkComponent() const
	{
		return CustomizationNetworkComponent;
	}
	
	// 결과화면에서 "클릭하여 이동" 같은 버튼 클릭 시 블루프린트에서 호출.
	// 조기 트래블을 서버에 요청합니다.
	UFUNCTION(BlueprintCallable, Category = "KC|Result")
    	void RequestSkipResultScreen();
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaSeconds) override;
	// 로딩화면 끝 콜백
	void HandleLoadingScreenHidden(FGameplayTag Channel, const FKCEmptyMessageStruct& Message);
	void HandleGamePhaseChanged(FGameplayTag Channel, const FKCGamePhaseChangedStruct& Message);

private:
	void InitializeInGameHUD();
	void ClearInGameHUD();
	void ShowResultScreen();
	void Move(const FInputActionValue& InputValue);
	void Dash(const FInputActionValue& InputValue);
	void Emote(const FInputActionValue& InputValue);
	void BeginUseHeldItem(const FInputActionValue& InputValue);
	void EndUseHeldItem(const FInputActionValue& InputValue);
	void Interact(const FInputActionValue& InputValue);
	void DropHeldItem(const FInputActionValue& InputValue);
	void UpdateCharacterFacing(float DeltaSeconds);
	
	// 서버 시간 동기화
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.0f;
	float TimeSinceLastServerTimeSync = 0.0f;

	UPROPERTY(VisibleAnywhere, Category = "KC|Customization|Network")
	TObjectPtr<UKCCustomizationNetworkComponent> CustomizationNetworkComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> EmoteAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DropHeldItemAction;

	FGameplayMessageListenerHandle LoadingScreenHiddenListenerHandle;
	FGameplayMessageListenerHandle GamePhaseChangedListenerHandle;
	
	UFUNCTION(Server, Reliable)
	void Server_RequestSkipResultScreen();
};
