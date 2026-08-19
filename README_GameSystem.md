# GameSystem

## 무슨 역할을 하는가?

"게임 규칙" 담당 모듈. 팀 인원이 다 찰 때까지 대기하다가 매치를 자동 시작하고, 재료가 냄비에 들어갈 때마다 점수를 매기고, 승리 조건(목표점수 도달 / 콜드게임)을 판정해서 게임을 종료시킴.

## 폴더 구조

```
Source/ProjectKC/
├── GameSystem/
│   ├── KCGameMode.h / .cpp        서버 전용 판정 로직 (승리 조건, 매치 시작/종료)
│   ├── KCGameState.h / .cpp       서버+클라 공용 데이터 (점수, 게임 페이즈)
│   └── KCGameSystemTags.h / .cpp  이벤트 채널(GameplayTag) 목록
├── Struct/
│   ├── KCIngredientSubmittedStruct.h   냄비 투입 이벤트 데이터
│   ├── KCScoreChangedStruct.h          점수 변경 이벤트 데이터
│   └── KCGamePhaseChangedStruct.h      게임 페이즈 변경 이벤트 데이터
└── Enum/
    └── KCGamePhaseType.h                대기중 / 진행중 / 종료
```

## 승리 조건 두 가지

1. **목표점수 도달** (IsTargetScoreReached): 어느 팀이든 `TargetScore`(기본 3점) 먼저 찍으면 즉시 승리
2. **콜드게임** (IsColdGameTriggered): 목표점수까지 안 갔어도, 1등과 2등의 점수 차이가 "남은 재료 개수"보다 크면 — 즉 2등이 남은 재료를 전부 가져가도 수학적으로 역전 불가능한 상황이면 — 그 시점에 조기 종료

두 함수 다 N팀(2팀 이상)에도 그대로 동작하도록 짰음(`TeamCount` 값만 늘리면 됨).

## 동작 흐름

```
[플레이어 접속] → GameMode가 매 틱 ReadyToStartMatch() 확인
     ↓ (GetNumPlayers() >= RequiredPlayerCount가 되는 순간)
[HandleMatchHasStarted()] → KCGameState에 팀 수 알려줌 + 이벤트 리스너 등록 + Phase를 Playing으로

[서버] 클라이언트가 냄비에 재료 넣음
   └─ (여기까지 오는 방법은 2번/4번 담당 — RPC 등)

[서버] FKCIngredientSubmittedStruct 이벤트 Broadcast
   └─ KCGameMode가 Listen → 점수 계산 → KCGameState에 반영 → 승리 조건 체크

[서버] KCGameState의 TeamScores, CurrentPhase 값 변경
   └─ Replication으로 모든 클라이언트에 자동 전파

[서버 + 모든 클라] OnRep 함수 호출됨
   └─ FKCScoreChangedStruct / FKCGamePhaseChangedStruct 로컬 Broadcast

[각자 컴퓨터] UI(6번) 등이 이 이벤트를 Listen해서 화면 갱신

[승리 조건 충족] → EndGame() → EndMatch() 호출 → HandleMatchHasEnded()에서 리스너 해제
```

## 설치 방법 (.Build.cs 수정 필요)

`Source/ProjectKC/ProjectKC.Build.cs` 열어서 `PublicDependencyModuleNames`에 추가:

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine", "InputCore",
    "GameplayTags",              // 추가
    "GameplayMessageRuntime"     // 추가 (Gameplay Message Router 플러그인)
});
```

`.uproject`의 `Plugins` 목록에 `GameplayMessageRuntime` 없으면 에디터 `Edit > Plugins`에서 "Gameplay Message Router" 검색해서 키기.

## TO. 고은님

1. **`Event.Ingredient.Submitted`를 어디서 Broadcast할지**
   클라이언트가 냄비 앞에서 상호작용 → Server RPC → 서버 검증 후 이 이벤트를 Broadcast하는 흐름이 될 텐데, RPC를 어디(냄비 액터? 컨트롤러?)에 만들지는 인철님과 이야기해보시면 될 것 같습니다.

2. **TeamId 출처: PlayerState**
   "이 플레이어가 몇 번 팀인지"는 `PlayerState`에 `int32 GetTeamId() const` 같은 조회 함수 형태로 있으면 충분합니다. 이 PlayerState 클래스 자체(로비 배정 로직 포함)는 4번이 만드는 게 자연스럽고, GameSystem은 그 함수만 갖다 쓰는 소비자 입장이에요.

## TO. 승재님

`TAG_Event_Game_ScoreChanged`, `TAG_Event_Game_PhaseChanged` 이 두 채널만 Listen하면 점수/상태 UI 갱신 가능합니다.

## TO. 인철님

냄비에 재료 투입이 서버에서 최종 확정되는 시점에 `FKCIngredientSubmittedStruct`(필드: `TeamId`, `SubmittedCount`)를 채워서 `TAG_Event_Ingredient_Submitted`로 Broadcast 해주시면 될 것 같습니다.

## TODO

- [v] 컴파일/PIE 테스트 — 완료
- [ ] **Asset Manager + 비동기 로드 적용**: MVP 단계에선 해당 없음.
- [ ] 게임 종료 후 처리(로비 복귀, 결과 화면 UI 연동)

