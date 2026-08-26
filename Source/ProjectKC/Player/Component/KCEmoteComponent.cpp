#include "ProjectKC/Player/Component/KCEmoteComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

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
	if (!IsRequestOwnerAllowed())
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
		ServerStopEmote(SafeBlendOutTime);
	}
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
	OnEmotePlayed.Broadcast(EmoteIndex, EmoteMontages[EmoteIndex]);
}

void UKCEmoteComponent::StopEmoteLocal(const float BlendOutTime)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UAnimInstance* AnimInstance = Character && Character->GetMesh()
		? Character->GetMesh()->GetAnimInstance()
		: nullptr;
	UAnimMontage* ActiveMontage = IsConfiguredEmote(ActiveEmoteIndex)
		? EmoteMontages[ActiveEmoteIndex]
		: nullptr;
	if (AnimInstance && ActiveMontage)
	{
		AnimInstance->Montage_Stop(BlendOutTime, ActiveMontage);
	}

	ActiveEmoteIndex = INDEX_NONE;
	OnEmoteStopped.Broadcast();
}
