#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Customization/KCCustomizationSaveGame.h"
#include "KCPlayerCustomizationComponent.generated.h"

class UMaterialInterface;
class URuntimeMeshPaintTargetComponent;
class UStaticMesh;
class UStaticMeshComponent;
class APlayerController;
class AKCPlayerState;
struct FKCCustomizationDescriptor;

/**
 * 플레이어의 그림 커스터마이징 외형을 독립적으로 생성하고 적용합니다.
 * 로컬 저장 적용과 향후 네트워크 데이터 적용이 같은 진입점을 사용합니다.
 */
UCLASS(ClassGroup = (KC), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECTKC_API UKCPlayerCustomizationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKCPlayerCustomizationComponent();

	/** Pawn의 소유권이 정해지거나 갱신될 때 호출합니다. 중복 호출해도 안전합니다. */
	void InitializeForPawn();

	/**
	 * Possess되지 않는 로비 표시 캐릭터를 특정 PlayerState에 연결합니다.
	 * LocalPlayerController는 해당 PlayerState가 로컬 플레이어일 때만 전달합니다.
	 */
	void InitializeForPresentation(
		AKCPlayerState* InPlayerState,
		APlayerController* LocalPlayerController);

	/** 로컬 플레이어의 SaveGame 데이터를 현재 외형에 적용합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Customization|Player")
	bool ApplyLocalSavedCustomization();

	/** 향후 네트워크에서 받은 동일한 패치 데이터를 현재 외형에 적용합니다. */
	UFUNCTION(BlueprintCallable, Category = "KC|Customization|Player")
	bool ApplyCustomizationData(
		const FRuntimeMeshPaintPatchHistory& PaintHistory,
		bool bUseDefaultAppearance);

	/** 다운로드 검증을 마친 서버 권위 외형을 적용하고 Revision을 기록합니다. */
	bool ApplyNetworkCustomizationData(
		const FRuntimeMeshPaintPatchHistory& PaintHistory,
		bool bUseDefaultAppearance,
		const FKCCustomizationDescriptor& Descriptor);

	UFUNCTION(BlueprintPure, Category = "KC|Customization|Player")
	bool IsRuntimeAppearanceReady() const;

	UFUNCTION(BlueprintPure, Category = "KC|Customization|Player")
	bool HasAppliedLocalSave() const { return bLocalSaveApplied; }

	/** 256 RT 시각 비교 모드에서는 기존 SaveGame/네트워크 데이터를 변경하지 않습니다. */
	bool IsRenderTargetPreviewMode() const;

	UFUNCTION(BlueprintPure, Category = "KC|Customization|Player")
	URuntimeMeshPaintTargetComponent* GetRuntimePaintTarget() const { return RuntimePaintTarget; }

	/** 로컬 편집 중에만 패치 기록과 페인트용 충돌을 활성화합니다. */
	bool BeginLocalCustomizationEditing();

	/** 로컬 편집용 패치 기록과 충돌을 원래 표시 전용 상태로 되돌립니다. */
	void EndLocalCustomizationEditing();

	UPROPERTY(BlueprintReadOnly, Category = "KC|Customization|Player")
	EKCCustomizationSaveResult LastApplyResult = EKCCustomizationSaveResult::NoSaveFound;

	/** 액세서리는 SM_PillBody와 같은 로컬 원점을 기준으로 제작되어 몸체에 직접 부착됩니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Customization|Assets")
	TObjectPtr<UStaticMesh> EyeMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Customization|Assets")
	TObjectPtr<UStaticMesh> ApronMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Customization|Assets")
	TObjectPtr<UStaticMesh> ChefHatMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Customization|Assets")
	TObjectPtr<UMaterialInterface> PaintMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Customization|Placement")
	FTransform LeftEyeTransform;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Customization|Placement")
	FTransform RightEyeTransform;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Customization|Placement")
	FTransform ApronTransform;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KC|Customization|Placement")
	FTransform ChefHatTransform;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool CreateRuntimeVisuals();
	bool CreateRuntimeAppearance();
	int32 GetDesiredRenderTargetSize() const;
	bool CompactLocalPaintHistory();
	void ReleaseRuntimePaintTarget();
	UStaticMeshComponent* CreatePaintMeshComponent(
		FName ComponentName,
		UStaticMesh* Mesh,
		const FTransform& RelativeTransform,
		UStaticMeshComponent* AttachParent);
	UStaticMeshComponent* FindAvatarBody() const;
	void HideLegacyEyeMesh() const;
	void DestroyRuntimeAppearance();
	void InitializeForPlayerState(
		AKCPlayerState* InPlayerState,
		APlayerController* LocalPlayerController);
	void BindCustomizationPlayerState(AKCPlayerState* InPlayerState);
	void UnbindCustomizationPlayerState();
	APlayerController* ResolveLocalPlayerController() const;
	void TryUploadLocalCustomization();
	void HandleCustomizationDescriptorChanged(const FKCCustomizationDescriptor& Descriptor);

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> EyesPaintMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> EyesPaintMesh_R;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ApronPaintMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ChefHatPaintMesh;

	UPROPERTY(Transient)
	TObjectPtr<URuntimeMeshPaintTargetComponent> RuntimePaintTarget;

	bool bLocalSaveApplied = false;
	bool bLocalCustomizationEditing = false;
	bool bCurrentUseDefaultAppearance = true;
	uint32 AppliedCustomizationRevision = 0;
	uint32 AppliedCustomizationHash = 0;
	TWeakObjectPtr<AKCPlayerState> BoundCustomizationPlayerState;
	TWeakObjectPtr<APlayerController> PresentationLocalPlayerController;
};
