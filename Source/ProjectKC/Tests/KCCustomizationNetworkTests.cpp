#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Customization/KCCustomizationNetworkComponent.h"
#include "Customization/KCCustomizationNetworkTypes.h"
#include "Lobby/KCLobbyPlayerController.h"
#include "Player/KCPlayerController.h"

namespace
{
	FRuntimeMeshPaintPatchHistory MakeValidPaintHistory()
	{
		FRuntimeMeshPaintPatchHistory History;
		History.Version = 1;
		History.LastSequenceId = 1;

		FRuntimeMeshPaintPatchHistoryEntry& Entry = History.Entries.AddDefaulted_GetRef();
		Entry.PaintTargetName = TEXT("PaintTarget_Customization");
		Entry.MeshTargetName = TEXT("ApronPaintMesh");
		Entry.MeshTargetIndex = 2;
		Entry.TextureType = ERuntimeMeshPaintPatchTextureType::Color;
		Entry.X = 10;
		Entry.Y = 20;
		Entry.Width = 1;
		Entry.Height = 1;
		Entry.RTWidth = KCCustomizationNetwork::ExpectedRenderTargetSize;
		Entry.RTHeight = KCCustomizationNetwork::ExpectedRenderTargetSize;
		Entry.RTFormat = RTF_RGBA16f;
		Entry.UVChannel = 0;
		Entry.SequenceId = 1;
		Entry.bCompressed = false;
		Entry.UncompressedByteCount = sizeof(FColor);
		Entry.PixelBytes = { 10, 20, 30, 255 };
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

#endif
