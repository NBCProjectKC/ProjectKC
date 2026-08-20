# GameSystem

## 무슨 역할을 하는가?

"게임 규칙" 담당 모듈. 팀 인원이 다 찰 때까지 대기하다가 매치를 자동 시작하고, 재료가 냄비에 들어갈 때마다 점수를 매기고, 승리 조건(목표점수 도달 / 콜드게임)을 판정해서 게임을 종료시킴.

## 폴더 구조

```
Source/ProjectKC/
├── GameSystem/
│   ├── KCGameMode.h / .cpp        서버 전용 판정 로직 (승리 조건, 매치 시작/종료)
│   ├── KCGameState.h / .cpp       서버+클라 공용 데이터 (점수, 게임 페이즈)
│   └── KCGamePhaseType.h          대기중 / 진행중 / 종료 (Enum)
└── Messages/
    ├── KCGameplayTags.h / .cpp    이벤트 채널(GameplayTag) 목록 (프로젝트 공용)
    └── Struct/
        ├── KCIngredientSubmittedStruct.h   냄비 투입 이벤트 데이터
        ├── KCScoreChangedStruct.h          점수 변경 이벤트 데이터
        └── KCGamePhaseChangedStruct.h      게임 페이즈 변경 이벤트 데이터
```

## 승리 조건 두 가지

1. **목표점수 도달** (IsTargetScoreReached): 어느 팀이든 `TargetScore`(기본 3점) 먼저 찍으면 즉시 승리
2. **콜드게임** (IsColdGameTriggered): 목표점수까지 안 갔어도, 1등과 2등의 점수 차이가 "남은 재료 개수"보다 크면 — 즉 2등이 남은 재료를 전부 가져가도 수학적으로 역전 불가능한 상황이면 — 그 시점에 조기 종료

두 함수 다 N팀(2팀 이상)에도 그대로 동작하도록 짰음(`TeamCount` 값만 늘리면 됨).

## 동작 흐름

```
[플레이어 접속] → GameMode가 매 틱 ReadyToStartMatch() 확인
     ↓ (GetNumPlayers() >= GetRequiredPlayerCount()가 되는 순간)
[HandleMatchHasStarted()] → KCGameState에 팀 수 알려줌 + 이벤트 리스너 등록 + Phase를 Playing으로

[클라이언트] 플레이어가 냄비 앞에서 재료 투입 시도(상호작용)
   └─ 창훈님 Server RPC로 서버에 요청 전달

[서버] KCGameplayTags::Message_Ingredient_Submitted 채널로
FKCIngredientSubmittedStruct(TeamId, SubmittedCount) 채워서 Broadcast

[서버] KCGameMode가 Listen → 점수 계산 → KCGameState에 반영 → 승리 조건 체크
   
[서버] KCGameState의 TeamScores, CurrentPhase 값 변경
   └─ Replication으로 모든 클라이언트에 자동 전파

[서버 + 모든 클라] OnRep 함수 호출됨
   └─ FKCScoreChangedStruct / FKCGamePhaseChangedStruct 로컬 Broadcast

[각자 컴퓨터] 이 이벤트를 Listen해서 화면 갱신

[승리 조건 충족] → EndGame() → EndMatch() 호출 → HandleMatchHasEnded()에서 리스너 해제
```

## TO. 고은님

PlayerState `GetTeamId()` 스펙

**int32 GetTeamId() const;**

요구사항
- PlayerState 클래스에 public으로 존재
- 리턴값: 0부터 시작하는 정수 (GameSystem의 TeamCount, 기본값 2, 범위 안의 값)
- 매치 시작 시점(HandleMatchHasStarted 호출 시점)엔 모든 플레이어가 이 값을 이미 갖고 있어야 함 → 로비 단계에서 팀 배정이 끝나 있어야 함

말씀드렸던 함수 구현 이렇게 부탁드립니다!


## TO. 창훈님

냄비에 재료 투입이 확정되는 시점에 `FKCIngredientSubmittedStruct`(`TeamId`, `SubmittedCount`) 채워주시고
`KCGameplayTags::Message_Ingredient_Submitted` 채널로 Broadcast 해주시면 될 것 같습니다.

`TeamId` 채워주실 때 `PlayerState`의 `GetTeamId()` 호출하시면 됩니다.


## TO. 승재님

`KCGameplayTags::Message_Game_ScoreChanged`
`KCGameplayTags::Message_Game_PhaseChanged`
이 두 채널 Listen 하셔서 점수/상태 UI 갱신 하시면 될 것 같습니다.
