#pragma once

#include "CoreMinimal.h"
#include "Player/KCPlayerCharacter.h"
#include "KCCapsulePlayerCharacter.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/**
 * Manny skeleton을 보이지 않는 애니메이션 드라이버로 유지하면서
 * 몸통, 머리, 손, 발만 표시하는 분리형 캡슐 캐릭터다.
 */
UCLASS()
class PROJECTKC_API AKCCapsulePlayerCharacter : public AKCPlayerCharacter
{
	GENERATED_BODY()

public:
	AKCCapsulePlayerCharacter();

	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void ConfigureDriverMesh();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Avatar",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> AvatarBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Avatar",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> AvatarHead;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Avatar",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> AvatarHandLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Avatar",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> AvatarHandRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Avatar",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> AvatarFootLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Avatar",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> AvatarFootRight;

	/** 추후 눈 Static Mesh를 붙일 기준점이다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KC|Avatar",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> FaceAnchor;
};
