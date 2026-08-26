#include "ProjectKC/Player/Component/KCEmoteComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "ProjectKC/Player/Animation/KCPlayerAnimInstance.h"

UKCEmoteComponent::UKCEmoteComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool UKCEmoteComponent::RequestPlayEmote(const int32 EmoteIndex)
{
	if (!IsRequestOwnerAllowed() || !IsConfiguredEmote(EmoteIndex))
	{
		return false;
	}

	if (GetOwner()->HasAuthority())
	{
		ServerPlayEmote_Implementation(EmoteIndex);
	}
	else
	{
		ServerPlayEmote(EmoteIndex);
	}
	return true;
}

bool UKCEmoteComponent::RequestPlayNextEmote()
{
	if (!IsRequestOwnerAllowed() ||
		FindNextConfiguredEmoteIndex() == INDEX_NONE)
	{
		return false;
	}

	if (GetOwner()->HasAuthority())
	{
		ServerPlayNextEmote_Implementation();
	}
	else
	{
		ServerPlayNextEmote();
	}
	return true;
}

void UKCEmoteComponent::RequestStopEmote(const float BlendOutTime)
{
	if (!IsRequestOwnerAllowed() || ActiveEmoteIndex == INDEX_NONE)
	{
		return;
	}

	const float SafeBlendOutTime = FMath::Clamp(BlendOutTime, 0.0f, 2.0f);
	if (GetOwner()->HasAuthority())
	{
		ServerStopEmote_Implementation(SafeBlendOutTime);
	}
	else
	{
		// 이동과 공격은 입력 즉시 보여야 하므로 소유 클라이언트에서 먼저 끊고
		// Reliable RPC로 서버와 다른 클라이언트에 같은 결과를 전파한다.
		StopEmoteLocal(SafeBlendOutTime);
		ServerStopEmote(SafeBlendOutTime);
	}
}

void UKCEmoteComponent::RequestInterruptEmote()
{
	RequestStopEmote(InterruptionBlendOutTime);
}

int32 UKCEmoteComponent::GetEmoteCount() const
{
	return EmoteMontages.Num();
}

bool UKCEmoteComponent::IsPlayingEmote() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UAnimMontage* ActiveMontage = IsConfiguredEmote(ActiveEmoteIndex)
		? EmoteMontages[ActiveEmoteIndex]
		: nullptr;
	const UAnimInstance* AnimInstance = Character && Character->GetMesh()
		? Character->GetMesh()->GetAnimInstance()
		: nullptr;
	return AnimInstance && ActiveMontage &&
		AnimInstance->Montage_IsPlaying(ActiveMontage);
}

void UKCEmoteComponent::ServerPlayEmote_Implementation(const int32 EmoteIndex)
{
	TryPlayEmoteOnServer(EmoteIndex);
}

void UKCEmoteComponent::ServerPlayNextEmote_Implementation()
{
	const int32 EmoteIndex = FindNextConfiguredEmoteIndex();
	if (EmoteIndex != INDEX_NONE)
	{
		TryPlayEmoteOnServer(EmoteIndex);
	}
}

void UKCEmoteComponent::ServerStopEmote_Implementation(const float BlendOutTime)
{
	MulticastStopEmote(FMath::Clamp(BlendOutTime, 0.0f, 2.0f));
}

void UKCEmoteComponent::MulticastPlayEmote_Implementation(const int32 EmoteIndex)
{
	PlayEmoteLocal(EmoteIndex);
}

void UKCEmoteComponent::MulticastStopEmote_Implementation(const float BlendOutTime)
{
	StopEmoteLocal(BlendOutTime);
}

bool UKCEmoteComponent::IsRequestOwnerAllowed() const
{
	const APawn* PawnOwner = Cast<APawn>(GetOwner());
	return PawnOwner && (PawnOwner->HasAuthority() || PawnOwner->IsLocallyControlled());
}

bool UKCEmoteComponent::TryPlayEmoteOnServer(const int32 EmoteIndex)
{
	if (!CanServerAcceptEmote(EmoteIndex))
	{
		return false;
	}

	LastAcceptedRequestTimeSeconds = GetWorld()->GetTimeSeconds();
	NextEmoteIndex = (EmoteIndex + 1) % EmoteMontages.Num();
	MulticastPlayEmote(EmoteIndex);
	return true;
}

bool UKCEmoteComponent::CanServerAcceptEmote(const int32 EmoteIndex) const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UAnimInstance* AnimInstance = Character && Character->GetMesh()
		? Character->GetMesh()->GetAnimInstance()
		: nullptr;
	if (!Character || !Character->HasAuthority() || !GetWorld() ||
		!AnimInstance || !IsConfiguredEmote(EmoteIndex))
	{
		return false;
	}

	if (bBlockWhenAnyMontagePlaying && AnimInstance->IsAnyMontagePlaying())
	{
		return false;
	}

	return LastAcceptedRequestTimeSeconds < 0.0 ||
		GetWorld()->GetTimeSeconds() - LastAcceptedRequestTimeSeconds >=
			MinimumRequestInterval;
}

bool UKCEmoteComponent::IsConfiguredEmote(const int32 EmoteIndex) const
{
	return EmoteMontages.IsValidIndex(EmoteIndex) &&
		IsValid(EmoteMontages[EmoteIndex]);
}

int32 UKCEmoteComponent::FindNextConfiguredEmoteIndex() const
{
	if (EmoteMontages.IsEmpty())
	{
		return INDEX_NONE;
	}

	const int32 StartIndex = EmoteMontages.IsValidIndex(NextEmoteIndex)
		? NextEmoteIndex
		: 0;
	for (int32 Offset = 0; Offset < EmoteMontages.Num(); ++Offset)
	{
		const int32 CandidateIndex =
			(StartIndex + Offset) % EmoteMontages.Num();
		if (IsConfiguredEmote(CandidateIndex))
		{
			return CandidateIndex;
		}
	}

	return INDEX_NONE;
}

void UKCEmoteComponent::PlayEmoteLocal(const int32 EmoteIndex)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !IsConfiguredEmote(EmoteIndex))
	{
		return;
	}

	if (Character->PlayAnimMontage(EmoteMontages[EmoteIndex]) <= 0.0f)
	{
		return;
	}

	ActiveEmoteIndex = EmoteIndex;
	SetGroundIKAllowed(false);
	if (UAnimInstance* AnimInstance = Character->GetMesh()
		? Character->GetMesh()->GetAnimInstance()
		: nullptr)
	{
		FOnMontageEnded MontageEndedDelegate;
		MontageEndedDelegate.BindUObject(
			this,
			&UKCEmoteComponent::HandleEmoteMontageEnded);
		AnimInstance->Montage_SetEndDelegate(
			MontageEndedDelegate,
			EmoteMontages[EmoteIndex]);
	}
	OnEmotePlayed.Broadcast(EmoteIndex, EmoteMontages[EmoteIndex]);
}

void UKCEmoteComponent::StopEmoteLocal(const float BlendOutTime)
{
	if (ActiveEmoteIndex == INDEX_NONE)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character && Character->GetMesh()
		? Character->GetMesh()->GetAnimInstance()
		: nullptr;
	UAnimMontage* ActiveMontage = IsConfiguredEmote(ActiveEmoteIndex)
		? EmoteMontages[ActiveEmoteIndex]
		: nullptr;
	// Montage_Stop이 종료 델리게이트를 동기적으로 호출해도 중복 이벤트가
	// 발생하지 않도록 상태를 먼저 비운다.
	ActiveEmoteIndex = INDEX_NONE;
	SetGroundIKAllowed(true);
	if (AnimInstance && ActiveMontage)
	{
		AnimInstance->Montage_Stop(BlendOutTime, ActiveMontage);
	}

	OnEmoteStopped.Broadcast();
}

void UKCEmoteComponent::SetGroundIKAllowed(const bool bAllowed) const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	UKCPlayerAnimInstance* AnimInstance = Character && Character->GetMesh()
		? Cast<UKCPlayerAnimInstance>(Character->GetMesh()->GetAnimInstance())
		: nullptr;
	if (AnimInstance)
	{
		AnimInstance->SetEmoteActive(!bAllowed);
	}
}

void UKCEmoteComponent::HandleEmoteMontageEnded(
	UAnimMontage* Montage,
	const bool bInterrupted)
{
	if (!IsConfiguredEmote(ActiveEmoteIndex) ||
		EmoteMontages[ActiveEmoteIndex] != Montage)
	{
		return;
	}

	ActiveEmoteIndex = INDEX_NONE;
	SetGroundIKAllowed(true);
	OnEmoteStopped.Broadcast();
}
