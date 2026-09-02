#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Customization/KCCustomizationNetworkComponent.h"
#include "Customization/KCCustomizationNetworkTypes.h"
#include "Lobby/KCLobbyCharacter.h"
#include "Lobby/KCLobbyPlayerController.h"
#include "Player/Component/KCPlayerCustomizationComponent.h"
#include "Player/KCPlayerController.h"

namespace
{
	const TArray<FString> ValidMeshTargetNames = {
		TEXT("EyesPaintMesh"),
		TEXT("EyesPaintMesh_R"),
		TEXT("ApronPaintMesh"),
		TEXT("ChefHatPaintMesh")
	};

	FRuntimeMeshPaintPatchHistoryEntry MakeValidPaintEntry(
		const int32 MeshTargetIndex,
		const ERuntimeMeshPaintPatchTextureType TextureType,
		const int32 SequenceId)
	{
		FRuntimeMeshPaintPatchHistoryEntry Entry;
		Entry.PaintTargetName = TEXT("PaintTarget_Customization");
		Entry.MeshTargetName = ValidMeshTargetNames[MeshTargetIndex];
		Entry.MeshTargetIndex = MeshTargetIndex;
		Entry.TextureType = TextureType;
		Entry.X = 10;
		Entry.Y = 20;
		Entry.Width = 1;
		Entry.Height = 1;
		Entry.RTWidth = KCCustomizationNetwork::ExpectedRenderTargetSize;
		Entry.RTHeight = KCCustomizationNetwork::ExpectedRenderTargetSize;
		Entry.RTFormat = RTF_RGBA16f;
		Entry.UVChannel = 0;
		Entry.SequenceId = SequenceId;
		Entry.bCompressed = false;
		Entry.UncompressedByteCount = sizeof(FColor);
		Entry.PixelBytes = { 10, 20, 30, 255 };
		return Entry;
	}

	FRuntimeMeshPaintPatchHistory MakeValidPaintHistory()
	{
		FRuntimeMeshPaintPatchHistory History;
		History.Version = 1;
		History.LastSequenceId = 1;
		History.Entries.Add(MakeValidPaintEntry(
			2,
			ERuntimeMeshPaintPatchTextureType::Color,
			1));
		return History;
	}

	FRuntimeMeshPaintPatchHistory MakeFullPaintHistory()
	{
		FRuntimeMeshPaintPatchHistory History;
		History.Version = 1;
		int32 SequenceId = 0;
		for (int32 MeshTargetIndex = 0;
			MeshTargetIndex < ValidMeshTargetNames.Num();
			++MeshTargetIndex)
		{
			History.Entries.Add(MakeValidPaintEntry(
				MeshTargetIndex,
				ERuntimeMeshPaintPatchTextureType::Color,
				++SequenceId));
			History.Entries.Add(MakeValidPaintEntry(
				MeshTargetIndex,
				ERuntimeMeshPaintPatchTextureType::MaterialSettings,
				++SequenceId));
		}
		History.LastSequenceId = SequenceId;
		return History;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCCustomizationNetworkDefaultRoundTripTest,
	"ProjectKC.Customization.Network.DefaultRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCCustomizationNetworkDefaultRoundTripTest::RunTest(const FString& Parameters)
{
	TArray<uint8> Payload;
	TestTrue(
		TEXT("기본 외형 페이로드를 직렬화한다."),
		KCCustomizationNetwork::SerializePayload(
			FRuntimeMeshPaintPatchHistory(),
			true,
			Payload));
	TestTrue(TEXT("직렬화 결과에 해시가 생성된다."),
		KCCustomizationNetwork::ComputePayloadHash(Payload) != 0);

	FRuntimeMeshPaintPatchHistory LoadedHistory;
	bool bUseDefaultAppearance = false;
	TestTrue(
		TEXT("기본 외형 페이로드를 역직렬화한다."),
		KCCustomizationNetwork::DeserializePayload(
			Payload,
			LoadedHistory,
			bUseDefaultAppearance));
	TestTrue(TEXT("기본 외형 플래그가 유지된다."), bUseDefaultAppearance);
	TestTrue(TEXT("기본 외형은 빈 패치 기록을 유지한다."), LoadedHistory.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCCustomizationNetworkPaintRoundTripTest,
	"ProjectKC.Customization.Network.PaintRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCCustomizationNetworkPaintRoundTripTest::RunTest(const FString& Parameters)
{
	const FRuntimeMeshPaintPatchHistory SourceHistory = MakeValidPaintHistory();
	TArray<uint8> Payload;
	TestTrue(
		TEXT("페인트 외형 페이로드를 직렬화한다."),
		KCCustomizationNetwork::SerializePayload(SourceHistory, false, Payload));

	FRuntimeMeshPaintPatchHistory LoadedHistory;
	bool bUseDefaultAppearance = true;
	TestTrue(
		TEXT("페인트 외형 페이로드를 역직렬화한다."),
		KCCustomizationNetwork::DeserializePayload(
			Payload,
			LoadedHistory,
			bUseDefaultAppearance));
	TestFalse(TEXT("페인트 외형 플래그가 유지된다."), bUseDefaultAppearance);
	TestEqual(TEXT("패치 개수가 유지된다."), LoadedHistory.Entries.Num(), 1);
	TestEqual(
		TEXT("메시 대상이 유지된다."),
		LoadedHistory.Entries[0].MeshTargetName,
		FString(TEXT("ApronPaintMesh")));
	TestEqual(
		TEXT("픽셀 데이터가 유지된다."),
		LoadedHistory.Entries[0].PixelBytes,
		SourceHistory.Entries[0].PixelBytes);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCCustomizationNetworkValidationTest,
	"ProjectKC.Customization.Network.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCCustomizationNetworkValidationTest::RunTest(const FString& Parameters)
{
	const FRuntimeMeshPaintPatchHistory FullHistory = MakeFullPaintHistory();
	TestEqual(
		TEXT("실제 압축 형식은 메시 4개와 텍스처 2종의 패치 8개를 가진다."),
		FullHistory.Entries.Num(),
		KCCustomizationNetwork::MaxPatchEntries);
	TestTrue(
		TEXT("Color와 MaterialSettings를 포함한 실제 8개 패치를 허용한다."),
		KCCustomizationNetwork::ValidateCustomizationData(FullHistory, false));

	FRuntimeMeshPaintPatchHistory InvalidHistory = MakeValidPaintHistory();
	InvalidHistory.Entries[0].MeshTargetName = TEXT("UnexpectedMesh");
	TestFalse(
		TEXT("허용되지 않은 메시 대상은 거부한다."),
		KCCustomizationNetwork::ValidateCustomizationData(InvalidHistory, false));

	InvalidHistory = MakeValidPaintHistory();
	InvalidHistory.Entries[0].RTWidth = 1024;
	TestFalse(
		TEXT("스키마와 다른 렌더 타깃 크기는 거부한다."),
		KCCustomizationNetwork::ValidateCustomizationData(InvalidHistory, false));

	TestFalse(
		TEXT("기본 외형 플래그와 페인트 데이터의 혼용을 거부한다."),
		KCCustomizationNetwork::ValidateCustomizationData(MakeValidPaintHistory(), true));

	InvalidHistory = FullHistory;
	const FRuntimeMeshPaintPatchHistoryEntry DuplicateEntry =
		InvalidHistory.Entries[0];
	InvalidHistory.Entries.Add(DuplicateEntry);
	TestFalse(
		TEXT("동일한 메시와 텍스처 종류의 중복 압축 패치를 거부한다."),
		KCCustomizationNetwork::ValidateCustomizationData(InvalidHistory, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCCustomizationNetworkControllerComponentsTest,
	"ProjectKC.Customization.Network.ControllerComponents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCCustomizationNetworkControllerComponentsTest::RunTest(const FString& Parameters)
{
	const AKCPlayerController* InGameControllerDefaults = GetDefault<AKCPlayerController>();
	const AKCLobbyPlayerController* LobbyControllerDefaults =
		GetDefault<AKCLobbyPlayerController>();

	const UKCCustomizationNetworkComponent* InGameNetworkComponent =
		InGameControllerDefaults
			? InGameControllerDefaults->GetCustomizationNetworkComponent()
			: nullptr;
	const UKCCustomizationNetworkComponent* LobbyNetworkComponent =
		LobbyControllerDefaults
			? LobbyControllerDefaults->GetCustomizationNetworkComponent()
			: nullptr;

	TestNotNull(
		TEXT("인게임 Controller에 공용 외형 네트워크 컴포넌트가 생성된다."),
		InGameNetworkComponent);
	TestNotNull(
		TEXT("로비 Controller에 공용 외형 네트워크 컴포넌트가 생성된다."),
		LobbyNetworkComponent);
	if (InGameNetworkComponent)
	{
		TestTrue(
			TEXT("인게임 외형 네트워크 컴포넌트는 복제된다."),
			InGameNetworkComponent->GetIsReplicated());
	}
	if (LobbyNetworkComponent)
	{
		TestTrue(
			TEXT("로비 외형 네트워크 컴포넌트는 복제된다."),
			LobbyNetworkComponent->GetIsReplicated());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCCustomizationNetworkTransportContractTest,
	"ProjectKC.Customization.Network.TransportContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCCustomizationNetworkTransportContractTest::RunTest(
	const FString& Parameters)
{
	const UClass* ComponentClass =
		UKCCustomizationNetworkComponent::StaticClass();
	const UFunction* UploadRequestFunction = ComponentClass->FindFunctionByName(
		TEXT("ClientRequestCustomizationUploadChunk"));
	const UFunction* DownloadAckFunction = ComponentClass->FindFunctionByName(
		TEXT("ServerAcknowledgeCustomizationDownloadChunk"));

	TestNotNull(
		TEXT("업로드의 다음 청크를 요청하는 Client RPC가 존재한다."),
		UploadRequestFunction);
	TestNotNull(
		TEXT("다운로드 청크를 확인하는 Server RPC가 존재한다."),
		DownloadAckFunction);
	if (UploadRequestFunction)
	{
		TestTrue(
			TEXT("업로드 청크 요청은 Reliable Client RPC다."),
			UploadRequestFunction->HasAllFunctionFlags(
				FUNC_Net | FUNC_NetReliable | FUNC_NetClient));
	}
	if (DownloadAckFunction)
	{
		TestTrue(
			TEXT("다운로드 청크 확인은 Reliable Server RPC다."),
			DownloadAckFunction->HasAllFunctionFlags(
				FUNC_Net | FUNC_NetReliable | FUNC_NetServer));
	}

	constexpr int32 RepresentativePayloadBytes = 240 * 1024;
	TestEqual(
		TEXT("240 KiB 외형은 한 프레임 일괄 전송 대신 8회 ACK 흐름으로 분할된다."),
		FMath::DivideAndRoundUp(
			RepresentativePayloadBytes,
			KCCustomizationNetwork::ChunkSizeBytes),
		8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCCustomizationNetworkFullPaintRoundTripTest,
	"ProjectKC.Customization.Network.FullPaintRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCCustomizationNetworkFullPaintRoundTripTest::RunTest(const FString& Parameters)
{
	const FRuntimeMeshPaintPatchHistory SourceHistory = MakeFullPaintHistory();
	TArray<uint8> Payload;
	TestTrue(
		TEXT("실제 8개 압축 패치를 네트워크 페이로드로 직렬화한다."),
		KCCustomizationNetwork::SerializePayload(SourceHistory, false, Payload));

	FRuntimeMeshPaintPatchHistory LoadedHistory;
	bool bUseDefaultAppearance = true;
	TestTrue(
		TEXT("실제 8개 압축 패치 페이로드를 역직렬화한다."),
		KCCustomizationNetwork::DeserializePayload(
			Payload,
			LoadedHistory,
			bUseDefaultAppearance));
	TestFalse(TEXT("페인트 외형 플래그가 유지된다."), bUseDefaultAppearance);
	TestEqual(
		TEXT("Color와 MaterialSettings 패치 8개가 유지된다."),
		LoadedHistory.Entries.Num(),
		KCCustomizationNetwork::MaxPatchEntries);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCCustomizationLobbyPresentationDefaultsTest,
	"ProjectKC.Customization.Lobby.PresentationDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCCustomizationLobbyPresentationDefaultsTest::RunTest(const FString& Parameters)
{
	const AKCLobbyCharacter* LobbyCharacterDefaults =
		GetDefault<AKCLobbyCharacter>();
	const UKCPlayerCustomizationComponent* CustomizationComponent =
		LobbyCharacterDefaults
			? LobbyCharacterDefaults->GetPlayerCustomizationComponent()
			: nullptr;

	TestNotNull(
		TEXT("로비 표시 캐릭터가 플레이어 외형 컴포넌트를 상속한다."),
		CustomizationComponent);
	return true;
}

#endif
