/**
 * @file KCPlayerSlotActor.cpp
 * @brief AKCPlayerSlotActor 구현부
 */

#include "ProjectKC/Lobby/KCPlayerSlotActor.h"
#include "ProjectKC/Lobby/KCLobbyCharacter.h"
#include "ProjectKC/ProjectKC.h"
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

	UE_LOG(LogKCLobby, Log, TEXT("[KCPlayerSlotActor] Slot %d: Player '%s' assigned (Ready: %s)"),
		SlotIndex, *InPlayerInfo.PlayerName, InPlayerInfo.bReady ? TEXT("TRUE") : TEXT("FALSE"));

	if (!HasAuthority())
	{
		return;
	}

	// [튕김 및 크래시 방지] 이미 스폰된 캐릭터가 있고 유효하다면 정보만 갱신
	if (SpawnedCharacter && IsValid(SpawnedCharacter))
	{
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCPlayerSlotActor] Slot %d: Updating existing SpawnedCharacter info"), SlotIndex);
		SpawnedCharacter->UpdatePlayerInfo(InPlayerInfo);
		return;
	}

	if (!CharacterClass)
	{
		static UClass* LoadedClass = StaticLoadClass(AKCLobbyCharacter::StaticClass(), nullptr, TEXT("/Game/KC/SteamLobbySystem/Blueprints/Lobby/BP_Lobby_PlayerCharacter.BP_Lobby_PlayerCharacter_C"));
		CharacterClass = LoadedClass;
		if (!CharacterClass)
		{
			UE_LOG(LogKCLobby, Error, TEXT("[KCPlayerSlotActor] Slot %d Failed to load BP_Lobby_PlayerCharacter class!"), SlotIndex);
		}
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
			UE_LOG(LogKCLobby, Log, TEXT("[KCPlayerSlotActor] Slot %d: Successfully spawned lobby character for '%s'"),
				SlotIndex, *InPlayerInfo.PlayerName);
		}
		else
		{
			UE_LOG(LogKCLobby, Error, TEXT("[KCPlayerSlotActor] Slot %d Failed to spawn lobby character!"), SlotIndex);
		}
	}
}

void AKCPlayerSlotActor::DestroySpawnedCharacter()
{
	if (HasAuthority() && SpawnedCharacter)
	{
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCPlayerSlotActor] Slot %d: Destroying spawned character"), SlotIndex);
		SpawnedCharacter->Destroy();
		SpawnedCharacter = nullptr;
	}
}

void AKCPlayerSlotActor::ClearSlot()
{
	UE_LOG(LogKCLobby, Log, TEXT("[KCPlayerSlotActor] Slot %d cleared (Previous Player: '%s')"), SlotIndex, *CurrentPlayerInfo.PlayerName);

	CurrentPlayerInfo = FKCPlayerInfoStruct();
	bIsOccupied = false;
	SlotState = EKCLobbySlotStateType::Empty;
	OnRep_SlotState();

	DestroySpawnedCharacter();
}

void AKCPlayerSlotActor::SetSlotClosed(bool bClosed)
{
	if (bClosed)
	{
		CurrentPlayerInfo = FKCPlayerInfoStruct();
		bIsOccupied = false;
		SlotState = EKCLobbySlotStateType::Closed;
		OnRep_SlotState();

		DestroySpawnedCharacter();
		UE_LOG(LogKCLobby, Verbose, TEXT("[KCPlayerSlotActor] Slot %d state set to CLOSED"), SlotIndex);
	}
	else
	{
		if (SlotState == EKCLobbySlotStateType::Closed)
		{
			SlotState = EKCLobbySlotStateType::Empty;
			OnRep_SlotState();
			UE_LOG(LogKCLobby, Verbose, TEXT("[KCPlayerSlotActor] Slot %d state reopened to EMPTY"), SlotIndex);
		}
	}
}

void AKCPlayerSlotActor::OnRep_SlotState()
{
	UE_LOG(LogKCLobby, Verbose, TEXT("[KCPlayerSlotActor] Slot %d OnRep_SlotState: %d, Occupied: %s"),
		SlotIndex, static_cast<int32>(SlotState), bIsOccupied ? TEXT("TRUE") : TEXT("FALSE"));
	OnSlotStateChanged(SlotState);
}

