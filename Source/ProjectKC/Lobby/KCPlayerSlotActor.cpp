/**
 * @file KCPlayerSlotActor.cpp
 * @brief AKCPlayerSlotActor 구현부
 */

#include "ProjectKC/Lobby/KCPlayerSlotActor.h"
#include "ProjectKC/Lobby/KCLobbyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/World.h"

AKCPlayerSlotActor::AKCPlayerSlotActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void AKCPlayerSlotActor::AssignPlayer(const FKCPlayerInfoStruct& InPlayerInfo)
{
	CurrentPlayerInfo = InPlayerInfo;
	bIsOccupied = true;

	if (!HasAuthority())
	{
		return;
	}

	// 기존에 스폰되어 있던 캐릭터가 있다면 파괴
	if (SpawnedCharacter)
	{
		SpawnedCharacter->Destroy();
		SpawnedCharacter = nullptr;
	}

	// 로비 캐릭터 클래스 동적 로드 (CDO 순환 참조 방지)
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
		// 캐릭터 원래 인게임 스케일 (1.0, 1.0, 1.0) 보장
		SpawnTransform.SetScale3D(FVector::OneVector);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedCharacter = GetWorld()->SpawnActor<AKCLobbyCharacter>(CharacterClass, SpawnTransform, SpawnParams);
		if (SpawnedCharacter)
		{
			// 캡슐 절반 높이(Z) 보정으로 발바닥 높이 일치
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

	if (HasAuthority() && SpawnedCharacter)
	{
		SpawnedCharacter->Destroy();
		SpawnedCharacter = nullptr;
	}
}
