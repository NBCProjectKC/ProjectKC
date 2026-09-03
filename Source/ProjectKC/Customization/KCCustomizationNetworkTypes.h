#pragma once

#include "CoreMinimal.h"
#include "Customization/KCCustomizationSaveGame.h"
#include "KCCustomizationNetworkTypes.generated.h"

/**
 * PlayerState에 복제되는 작은 외형 식별자입니다.
 * 실제 픽셀 데이터는 PlayerController의 요청형 청크 RPC로 전송합니다.
 */
USTRUCT()
struct PROJECTKC_API FKCCustomizationDescriptor
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 Revision = 0;

	UPROPERTY()
	uint32 ContentHash = 0;

	UPROPERTY()
	int32 TargetSchemaVersion = UKCCustomizationSaveGame::CurrentTargetSchemaVersion;

	UPROPERTY()
	bool bUseDefaultAppearance = true;

	bool IsPublished() const { return Revision > 0; }

	bool Matches(const FKCCustomizationDescriptor& Other) const
	{
		return Revision == Other.Revision &&
			ContentHash == Other.ContentHash &&
			TargetSchemaVersion == Other.TargetSchemaVersion &&
			bUseDefaultAppearance == Other.bUseDefaultAppearance;
	}
};

namespace KCCustomizationNetwork
{
	inline constexpr int32 ChunkSizeBytes = 32 * 1024;
	inline constexpr int32 MaxPayloadBytes = 5 * 1024 * 1024;
	// CompactPaintPatchHistory는 메시마다 Color와 MaterialSettings를 각각 1개로 압축합니다.
	inline constexpr int32 MaxPatchEntries = 8;
	inline constexpr int32 ExpectedRenderTargetSize = 512;

	PROJECTKC_API uint32 ComputePayloadHash(const TArray<uint8>& Payload);

	/**
	 * 런타임 액터 인스턴스 이름 때문에 동일한 메시 패치가 서로 다른
	 * 압축 그룹으로 분리되지 않도록 PaintTargetName을 통일합니다.
	 */
	PROJECTKC_API void NormalizePaintTargetIdentity(
		FRuntimeMeshPaintPatchHistory& PaintHistory,
		const FString& PaintTargetIdentity);

	PROJECTKC_API bool SerializePayload(
		const FRuntimeMeshPaintPatchHistory& PaintHistory,
		bool bUseDefaultAppearance,
		TArray<uint8>& OutPayload);

	PROJECTKC_API bool DeserializePayload(
		const TArray<uint8>& Payload,
		FRuntimeMeshPaintPatchHistory& OutPaintHistory,
		bool& bOutUseDefaultAppearance);

	PROJECTKC_API bool ValidateCustomizationData(
		const FRuntimeMeshPaintPatchHistory& PaintHistory,
		bool bUseDefaultAppearance);
}
