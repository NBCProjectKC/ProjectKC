#include "ProjectKC/Item/Struct/KCItemPresentationStruct.h"

#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"

bool FKCItemPresentationStruct::TryGetGripAlignmentTransform(
	FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;
	if (!IsValid(StaticMesh) || GripSocketName.IsNone())
	{
		return false;
	}

	const UStaticMeshSocket* GripSocket = StaticMesh->FindSocket(GripSocketName);
	if (!GripSocket)
	{
		return false;
	}

	// Scale은 장착 기준으로 사용하지 않는다. Grip은 위치와 방향만 정의한다.
	const FTransform GripTransform(
		GripSocket->RelativeRotation,
		GripSocket->RelativeLocation,
		FVector::OneVector);
	OutTransform = GripTransform.Inverse();
	return true;
}

bool FKCItemPresentationStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!IsValid(StaticMesh))
	{
		OutError = TEXT("StaticMesh가 비어 있습니다.");
		return false;
	}

	if (WorldCollisionProfile.IsNone())
	{
		OutError = TEXT("WorldCollisionProfile이 비어 있습니다.");
		return false;
	}

	return true;
}
