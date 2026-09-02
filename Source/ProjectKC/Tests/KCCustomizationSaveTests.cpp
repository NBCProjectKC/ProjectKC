#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Customization/KCCustomizationSaveGame.h"
#include "Customization/KCCustomizationWorldSubsystem.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FKCCustomizationSaveSerializationTest,
	"ProjectKC.Customization.LocalSave.Serialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKCCustomizationSaveSerializationTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("커스터마이징 런타임은 월드 서브시스템으로 자동 생성된다."),
		UKCCustomizationWorldSubsystem::StaticClass()->IsChildOf(UWorldSubsystem::StaticClass()));

	UKCCustomizationSaveGame* Source = NewObject<UKCCustomizationSaveGame>();
	Source->bUseDefaultAppearance = false;
	Source->PaintHistory.Version = 1;
	Source->PaintHistory.LastSequenceId = 7;

	FRuntimeMeshPaintPatchHistoryEntry& Entry = Source->PaintHistory.Entries.AddDefaulted_GetRef();
	Entry.PaintTargetName = TEXT("PaintTarget_Customization");
	Entry.MeshTargetName = TEXT("ApronPaintMesh");
	Entry.MeshTargetIndex = 2;
	Entry.SequenceId = 7;
	Entry.Width = 1;
	Entry.Height = 1;
	Entry.RTWidth = 512;
	Entry.RTHeight = 512;
	Entry.UncompressedByteCount = sizeof(FColor);
	Entry.PixelBytes = { 1, 2, 3, 4 };

	TArray<uint8> Bytes;
	TestTrue(TEXT("SaveGame 데이터를 메모리에 직렬화할 수 있다."),
		UGameplayStatics::SaveGameToMemory(Source, Bytes));

	UKCCustomizationSaveGame* Loaded = Cast<UKCCustomizationSaveGame>(
		UGameplayStatics::LoadGameFromMemory(Bytes));
	TestNotNull(TEXT("직렬화된 커스터마이징 데이터를 다시 읽을 수 있다."), Loaded);
	if (!Loaded)
	{
		return false;
	}

	TestEqual(TEXT("저장 버전이 유지된다."), Loaded->SaveVersion,
		UKCCustomizationSaveGame::CurrentSaveVersion);
	TestFalse(TEXT("기본 외형 여부가 유지된다."), Loaded->bUseDefaultAppearance);
	TestEqual(TEXT("패치 개수가 유지된다."), Loaded->PaintHistory.Entries.Num(), 1);
	TestEqual(TEXT("대상 메시 이름이 유지된다."),
		Loaded->PaintHistory.Entries[0].MeshTargetName,
		FString(TEXT("ApronPaintMesh")));
	TestEqual(TEXT("픽셀 데이터가 유지된다."),
		Loaded->PaintHistory.Entries[0].PixelBytes.Num(),
		4);

	return true;
}

#endif
