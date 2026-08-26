/**
 * @file KCPlayerSlotActor.cpp
 * @brief AKCPlayerSlotActor 구현부
 */

#include "ProjectKC/Lobby/KCPlayerSlotActor.h"
#include "ProjectKC/Lobby/KCLobbyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

AKCPlayerSlotActor::AKCPlayerSlotActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SlotState = EKCLobbySlotStateType::Empty;
}

void AKCPlayerSlotActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKCPlayerSlotActor, SlotState);
	DOREPLIFETIME(AKCPlayerSlotActor, bIsOccupied);
	DOREPLIFETIME(AKCPlayerSlotActor, CurrentPlayerInfo);
}

void AKCPlayerSlotActor::AssignPlayer(const FKCPlayerInfoStruct& InPlayerInfo)
{
	CurrentPlayerInfo = InPlayerInfo;
	bIsOccupied = true;
	SlotState = InPlayerInfo.bReady ? EKCLobbySlotStateType::Ready : EKCLobbySlotStateType::Occupied;
	OnRep_SlotState();

	if (!HasAuthority())
	{
		return;
	}

	// [튕김 및 크래시 방지] 이미 스폰된 캐릭터가 있고 유효하다면 정보만 갱신
	if (SpawnedCharacter && IsValid(SpawnedCharacter))
	{
		SpawnedCharacter->UpdatePlayerInfo(InPlayerInfo);
		return;
	}

	if (!CharacterClass)
	{
		CharacterClass = StaticLoadClass(AKCLobbyCharacter::StaticClass(), nullptr, TEXT("/Game/KC/SteamLobbySystem/Blueprints/Lobby/BP_Lobby_PlayerCharacter.BP_Lobby_PlayerCharacter_C"));
	}

	if (CharacterClass)
	{
		FTransform SpawnTransform = GetActorTransform();
		if (UArrowComponent* ArrowComp = FindComponentByClass<UArrowComponent>())
		{
			SpawnTransform = ArrowComp->GetComponentTransform();
		}

		SpawnTransform.SetScale3D(FVector::OneVector);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedCharacter = GetWorld()->SpawnActor<AKCLobbyCharacter>(CharacterClass, SpawnTransform, SpawnParams);
		if (SpawnedCharacter)
		{
			if (UCapsuleComponent* Capsule = SpawnedCharacter->GetCapsuleComponent())
			{
				FVector AdjustedLoc = SpawnTransform.GetLocation();
				AdjustedLoc.Z += Capsule->GetScaledCapsuleHalfHeight();
				SpawnTransform.SetLocation(AdjustedLoc);
				SpawnedCharacter->SetActorTransform(SpawnTransform);
			}

			SpawnedCharacter->UpdatePlayerInfo(InPlayerInfo);
		}
	}
}

void AKCPlayerSlotActor::ClearSlot()
{
	CurrentPlayerInfo = FKCPlayerInfoStruct();
	bIsOccupied = false;
	SlotState = EKCLobbySlotStateType::Empty;
	OnRep_SlotState();

	if (HasAuthority() && SpawnedCharacter)
	{
		SpawnedCharacter->Destroy();
		SpawnedCharacter = nullptr;
	}
}

void AKCPlayerSlotActor::SetSlotClosed(bool bClosed)
{
	if (bClosed)
	{
		CurrentPlayerInfo = FKCPlayerInfoStruct();
		bIsOccupied = false;
		SlotState = EKCLobbySlotStateType::Closed;
		OnRep_SlotState();

		if (HasAuthority() && SpawnedCharacter)
		{
			SpawnedCharacter->Destroy();
			SpawnedCharacter = nullptr;
		}
	}
	else
	{
		if (SlotState == EKCLobbySlotStateType::Closed)
		{
			SlotState = EKCLobbySlotStateType::Empty;
			OnRep_SlotState();
		}
	}
}

void AKCPlayerSlotActor::OnRep_SlotState()
{
	OnSlotStateChanged(SlotState);
}
