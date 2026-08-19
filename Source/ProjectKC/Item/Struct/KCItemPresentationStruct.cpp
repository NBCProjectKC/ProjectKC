#include "ProjectKC/Item/Struct/KCItemPresentationStruct.h"

#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"

/**
 * @brief Computes the transform that aligns the item with its configured grip socket.
 *
 * The grip alignment uses the socket's position and rotation; socket scale is ignored.
 * The output is set to the identity transform when the mesh or socket is invalid.
 *
 * @param OutTransform Receives the grip alignment transform.
 * @return `true` if the configured grip socket was found, `false` otherwise.
 */
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

/**
 * @brief Validates the static mesh, grip socket, and world collision profile.
 *
 * @param OutError Receives a description of the first validation failure, or is cleared when validation succeeds.
 * @return true if all required item presentation data is valid, false otherwise.
 */
bool FKCItemPresentationStruct::Validate(FString& OutError) const
{
	OutError.Reset();
	if (!IsValid(StaticMesh))
	{
		OutError = TEXT("StaticMesh가 비어 있습니다.");
		return false;
	}

	if (GripSocketName.IsNone() || !StaticMesh->FindSocket(GripSocketName))
	{
		OutError = FString::Printf(
			TEXT("StaticMesh에 Grip 소켓 '%s'가 없습니다."),
			*GripSocketName.ToString());
		return false;
	}

	if (WorldCollisionProfile.IsNone())
	{
		OutError = TEXT("WorldCollisionProfile이 비어 있습니다.");
		return false;
	}

	return true;
}
