# 액션 실행 이력 사용법

갱신: 2026-10-12  
대상: KataRuntime `UKataActionComponent`, KataGraph 그래프 실행, GameplayDebugger `KataAction` 카테고리  
적용 기준: [#58](https://github.com/jaykop/Kata/issues/58) 구현. 결정 이유는 [실행 이력 기록](../devlog/2026-10-11-Action-Trace.md)을 따른다  
확인 상태: 2026-10-11 사용자 빌드 성공, PIE에서 `Kata.ActionTrace.Dump` 출력과 GameplayDebugger 표시 확인. Shipping·Test 구성에서 빠지는지는 확인하지 않았다

## 목적과 준비

콤보가 이어지지 않거나 액션이 끊겼을 때 로그를 훑지 않고 원인을 찾기 위한 기능이다. 캐릭터마다 최근 액션 실행 결과(시작, 거절, 종료, 공용 중단, 창 열림과 닫힘, 그래프 트리거 처리 결과)를 정해진 필드로 남기고, 화면과 콘솔에서 읽는다.

- 액터에 `UKataActionComponent`가 있으면 따로 설정하지 않아도 기록한다. 그래프 이벤트는 `UKataGraphComponent`가 같은 액터의 액션 컴포넌트에 남긴다.
- Debug·Development·Editor 구성에서 상시 기록한다. Shipping·Test 구성에는 기록 코드, 콘솔 명령·변수, 디버거 카테고리가 없다.
- 직접 재생한 Kata는 `showdebug abilitysystem`과 GAS Inspector에 보이지 않으므로 이 기록으로 확인한다.

## 사용 순서

### 화면에서 보기

1. PIE 중 `'` 키로 GameplayDebugger를 켠다.
2. 위쪽 카테고리 목록에서 `KataAction`이 꺼져 있으면 옆의 숫자 키(키보드 위쪽 숫자)로 켠다.
3. 디버거를 켤 때 화면 가운데에 있던 액터가 대상이다. 대상이 없으면 플레이어 Pawn의 기록을 보인다.

화면에는 대상 액터, 현재 액션과 시간·길이·반복 회차, 보관 중인 기록 수, 최근 기록 12개가 보인다. 각 줄 앞의 `-1.47s`는 지금부터 거슬러 간 시간이다.

| 색 | 기록 |
|---|---|
| 빨강 | 시작 거절, 트리거 불일치·무시, 보관 트리거 재평가 실패 |
| 주황 | 완료·전이(Branched)가 아닌 종료, 공용 중단 요청 |
| 초록 | 액션 시작, 트리거 전이·예약 |
| 노랑 | 트리거 보관 |
| 흰색 | 완료·전이로 끝난 종료 |
| 회색 | 창 열림·닫힘, 그래프 시작 |

### 로그로 꺼내기

콘솔(`~`) 또는 에디터 Output Log 입력란에서 실행한다. 에디터 쪽에서 실행해도 실행 중인 PIE 월드의 기록을 읽는다.

```
Kata.ActionTrace.Dump                  플레이어 Pawn의 기록 전부
Kata.ActionTrace.Dump 10               플레이어 Pawn의 최근 10개
Kata.ActionTrace.Dump BlackKnight      이름(또는 에디터 라벨)에 BlackKnight가 들어간 액터들
Kata.ActionTrace.Dump All 20           기록이 있는 모든 액터의 최근 20개씩
```

결과는 Output Log의 `LogKataActionTrace` 카테고리에 `trace actor=... entries=... shown=... enabled=...` 머리줄과 기록 한 줄씩으로 나온다. 대상을 찾지 못하면 경고와 사용법을 한 줄 출력한다.

`Kata.ActionTrace.LogLive 1`을 켜 두면 기록이 생길 때마다 같은 형식으로 로그에 쓴다. 사망처럼 액터가 곧 파괴되는 장면은 이 방법으로 남긴다.

### 에이전트가 읽기

unreal-mcp의 `EditorToolset.LogsToolset.GetLogEntries`에 카테고리 `LogKataActionTrace`를 주면 Dump나 LogLive가 로그에 쓴 줄을 읽을 수 있다. 2026-10-11에 확인한 unreal-mcp 도구에는 콘솔 명령을 실행하는 도구가 없으므로, 명령은 사용자가 입력한다.

### C++에서 읽기

인게임 디버그 메뉴 패널처럼 기록을 직접 그리는 코드는 C++ 조회 API를 쓴다. 리플렉션이 없어 Blueprint에서는 쓸 수 없다.

```cpp
#include "Runtime/KataActionComponent.h"

#if KATA_WITH_ACTION_TRACE
// 오래된 것부터 복사한다. 매 프레임 그린다면 GetTraceSerial()이 바뀌었을 때만 다시 복사한다.
TArray<FKataActionTraceEntry> Entries;
ActionComponent->GetTraceEntries(Entries);
#endif
```

소스 기준이며 이 예제는 실행하지 않았다.

## 콘솔 명령과 변수

| 이름 | 기본값 | 의미 |
|---|---|---|
| `Kata.ActionTrace.Dump [All\|액터 이름 일부] [개수]` | - | 기록을 `LogKataActionTrace`에 출력한다. 대상이 없으면 로컬 플레이어 Pawn이다. 개수 0이나 생략은 전부다 |
| `Kata.ActionTrace.Enable` | 1 | 0이면 새 기록을 남기지 않는다. 이미 남은 기록은 지우지 않는다 |
| `Kata.ActionTrace.Capacity` | 64 | 액터 하나가 보관하는 기록 수(1~1024). 넘치면 가장 오래된 것부터 덮어쓴다. 바꾸면 다음 기록 때 최근 기록만 남기고 크기를 다시 잡는다 |
| `Kata.ActionTrace.LogLive` | 0 | 1이면 새 기록을 남길 때마다 로그에도 쓴다. 반복으로 합쳐진 기록은 다시 쓰지 않는다 |

## 기록 읽는 법

### 출력 형식

한 줄이 기록 하나다. 키 이름과 순서는 고정이며 쓰지 않는 키는 뺀다.

```
seq=12 t=34.512 f=10233 actor=BP_X ev=ActionRejected src=Transition asset=KA_Attack02 result=BlockedByActiveKata detail=CannotInterrupt
```

| 키 | 내용 | 나오는 조건 |
|---|---|---|
| `seq` | 액터별 일련번호. 1부터 증가 | 항상 |
| `t`, `f` | 기록한 월드 시각(초)과 엔진 프레임. 합쳐진 기록은 마지막 시각 | 항상 |
| `actor` | 기록을 가진 액터 | 항상 |
| `ev` | 이벤트 종류(아래 표) | 항상 |
| `src` | 시작 경로. `Direct`, `Ability`(GA가 재생), `Transition`(그래프 전이) | 시작·거절 |
| `id` | 같은 실행의 시작·창·종료를 짝짓는 번호 | 실행과 연결된 기록 |
| `asset` | 액션 에셋. 그래프 이벤트는 그래프 에셋 | 에셋이 있을 때 |
| `node` | 그래프 노드. 액션 노드는 액션 에셋 이름 | 그래프 트리거·종료 |
| `tag` | 트리거·창 태그, 또는 거절에서 처음 걸린 태그 | 태그가 있을 때 |
| `result` | 시작 거절 사유(`EKataStartResult`) | `ActionRejected` |
| `end` | 종료 사유(`EKataEndReason`). 공용 중단은 요청한 사유 | 종료·공용 중단 |
| `detail` | 이벤트별 보충 ID(아래 표) | 값이 있을 때 |
| `cond`, `reason` | 조건 결과(`Pass`·`Fail`·`Invalid`)와 조건의 Reason | 조건 실패를 남겼을 때 |
| `rep` | 같은 내용이 연달아 와서 합친 횟수 | 2 이상일 때 |

### 이벤트

| `ev` | 뜻 |
|---|---|
| `ActionStarted` | 액션이 시작됐다 |
| `ActionRejected` | 시작 요청이 거절됐다. `result`가 사유다 |
| `ActionEnded` | 액션이 끝났다. `end`가 사유다 |
| `CancelRequested` | `CancelAbilitiesAndKata`가 호출됐다. 사망 처리도 이 경로다 |
| `TransitionWindowOpened`·`Closed` | 전이 창이 열리고 닫혔다. 같은 태그를 겹쳐 연 창은 처음 열릴 때와 마지막으로 닫힐 때만 남긴다 |
| `CancelWindowOpened`·`Closed` | 캔슬 창이 열리고 닫혔다. 겹침 규칙은 전이 창과 같다 |
| `GraphStarted`·`GraphEnded` | 그래프 실행이 시작되고 끝났다 |
| `TriggerTransition` | 트리거로 엣지를 골라 지금 전이를 시도했다. 결과는 이어지는 `ActionStarted`·`ActionRejected`다 |
| `TriggerReserved` | 현재 액션이 끝난 뒤의 전이로 예약했다 |
| `TriggerBuffered` | 요구하는 전이 창이 닫혀 있어 트리거를 보관했다. `node`는 현재 노드다 |
| `TriggerUnmatched` | 트리거를 받을 엣지가 없거나 조건이 맞지 않았다. `detail`이 이유다 |
| `TriggerBufferUnmatched` | 보관한 트리거를 기다리던 창이 열려 다시 평가했지만 받아들이지 못했다. `detail`이 이유다. 그 창을 요구하는 엣지가 있을 때만 남는다. 같은 액션에서 다른 창이 나중에 열릴 수 있어 보관은 유지된다 |
| `TriggerIgnored` | 그래프가 트리거를 평가하지 않았다. `detail`이 이유다 |

### `detail` 값

| 이벤트 | 값 | 뜻 |
|---|---|---|
| `TriggerUnmatched` | `NoEdge` | 출발 노드(현재 노드와 이를 포함하는 Alias, 대기 중이면 Entry)에 이 트리거를 받는 엣지가 없다. `node`는 현재 노드다 |
| | `WindowClosed` | 엣지가 요구하는 창이 닫혀 있고 보관할 수도 없었다(실행 중인 액션이 없음) |
| `TriggerBufferUnmatched` | `TooEarly` | 창은 열렸지만 입력이 창의 PreAcceptSeconds보다 앞서 도착했다 |
| `TriggerUnmatched`·`TriggerBufferUnmatched` | `SourceCondition` | Entry·Alias 노드 자체의 조건이 실패했다. `node`는 그 노드다 |
| | `EdgeCondition` | 엣지 조건이 실패했다. `node`는 엣지의 도착 노드다 |
| | `NodeCondition` | 도착 노드의 진입 조건이 실패했다 |
| | `NoExecutableTarget` | 엣지는 통과했지만 Conduit 등을 지나 머무를 액션 노드까지 가지 못했다 |
| `TriggerIgnored` | `GraphNotRunning`, `ChangingAction`, `InvalidTrigger` | 그래프 미실행, 액션 교체 중, 빈 태그 |
| `TriggerTransition`·`Reserved` | `FromBuffer` | 보관했던 트리거가 창이 열려 받아들여졌다 |
| `ActionRejected` | `CannotInterrupt` | 새 액션이 실행 중인 액션을 끊을 수 없다(`bCanInterruptActiveKata` 꺼짐) |
| | `BlockedByActiveTags` | 실행 중인 액션의 BlockedKataTags가 막았다. `tag`가 걸린 KataTag다 |
| | `TransitionSourceChanged` | 그래프 전이가 넘겨받으려던 액션이 이미 바뀌었다 |
| | 그 밖의 값 | `ResolveFailed`일 때 첫 Error 진단 코드 |
| `CancelRequested` | `All`, `Filtered` | 태그 필터 없는 전체 중단, 필터가 있는 중단(`tag`는 WithTags의 첫 태그) |

시작 거절의 `tag`는 사유에 따라 다르다. `MissingRequiredTags`는 없는 첫 필수 태그, `BlockedByTags`는 가진 첫 차단 태그, `BlockedByAbilityTags`는 막힌 첫 KataTag, `OnCooldown`은 ASC가 가진 첫 쿨다운 그룹 태그다. `ConditionFailed`는 `cond`·`reason`에 StartCondition의 결과를 남긴다.

### 읽는 예

2026-10-11 샘플 PC에서 공격 중 회피를 눌렀을 때의 화면 기록을 시간순으로 읽으면 다음과 같다.

| 시각 | 기록 | 해석 |
|---|---|---|
| -1.62s | `TransitionWindowOpened KA_BlackKnight_Attack01 [Window.Transition.Combo]` | 공격 1의 콤보 창이 열렸다 |
| -1.47s | `TriggerBuffered ... [Trigger.Dodge]` | 회피 입력이 왔지만 회피 창이 아직 닫혀 있어 보관됐다 |
| -1.10s | `TransitionWindowOpened ... [Window.Transition.Dodge]` | 회피 창이 열렸다. 보관한 입력은 쓰이지 않았다. 이 창의 PreAcceptSeconds가 0이라 창이 열리기 전 입력은 받지 않는다. 당시에는 이유가 기록되지 않았고, 이후 추가한 `TriggerBufferUnmatched`가 이 자리에 `TooEarly`를 남긴다 |
| -0.91s | `TriggerTransition ... -> KA_BlackKnight_Backstep [Trigger.Dodge]` | 새로 누른 회피 입력으로 전이했다(`FromBuffer` 없음) |
| -0.91s | `ActionEnded KA_BlackKnight_Attack01 Branched`, `ActionStarted KA_BlackKnight_Backstep` | 공격 1이 전이로 끝나고 백스텝이 시작됐다 |

## 기록 시점과 순서

- 액션 시작은 시각 0 태스크보다 먼저 남긴다. 그래서 시작 직후 열리는 창과 즉시 종료가 시작 기록 뒤에 온다.
- 액션을 교체하면 이전 액션의 종료(`Branched`·`Interrupted`)가 새 액션의 시작보다 먼저 남는다.
- 트리거 전이·예약은 전이를 적용하기 전에 남긴다. 이어지는 시작·거절 기록이 그 결과다.
- 공용 중단 요청은 그 요청으로 생긴 종료보다 먼저 남는다.
- 직전 기록과 내용이 같으면 새 줄 대신 `rep`를 올린다. AI Task처럼 매 프레임 같은 트리거를 보내는 호출자가 보관 개수를 다 쓰지 않게 하기 위해서다.
- `CanPlayKataAction` 같은 조회는 기록하지 않는다. 실제 시작 요청만 남긴다.
- 거절 사유의 상세(태그, 조건 결과)와 트리거 불일치 이유는 기록이 켜져 있을 때만 실패 경로에서 다시 구한다. 시작 판정과 전이 선택 경로에는 비용을 더하지 않는다.
- 기록은 액션 컴포넌트에 있으므로 액터가 파괴되면 함께 사라진다. 저장 공간은 첫 기록 때 잡는다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 그래프가 없을 때 누른 입력이 기록되지 않는다 | 입력 처리 컴포넌트와 AI Task는 그래프 실행 여부를 먼저 확인하고 그래프에 트리거를 보내지 않는다 | `GraphEnded`와 그 뒤의 기록 공백으로 판단한다 |
| 조건 실패에 어떤 말단 조건이 실패했는지 없다 | 말단 조건의 Fail에는 Reason이 비어 있다. Group(All)은 실패한 자식의 결과를 그대로 전파하고 Group(Any)는 `NoConditionSatisfied`를 남긴다. `cond=Invalid`면 Reason에 설정·문맥 오류가 있다 | 조건 구성을 에디터에서 확인한다 |
| `BlockedByAbilityTags`의 `tag`가 실제 원인과 다를 수 있다 | 전이에서는 교체될 액션이 건 차단 태그를 통째로 빼는 근사를 쓴다([GAS 차단](Runtime-Usage.md#gas-차단)) | 같은 태그를 막는 다른 GA·GE가 있는지 확인한다 |
| 화면에 그래프 현재 노드가 없다 | 디버거 카테고리는 KataRuntime에 있어 KataGraph 상태를 모른다 | 그래프 에디터의 PIE 디버거를 쓴다([에디터 사용법](Editor-Usage.md)) |
| 사망 뒤 기록을 볼 수 없다 | 액터가 파괴되면 기록도 사라진다 | `Kata.ActionTrace.LogLive 1`을 켜고 재현한다 |
| 창 기록이 없는 실행이 있다 | `UKataActionComponent`가 만들지 않은 액션 인스턴스(에디터 프리뷰 등)는 창을 기록하지 않는다 | 게임 월드에서 확인한다 |
| 그래프 에디터 Debug 탭과 내용이 다르다 | Debug 탭은 그래프 인스턴스별 에디터 전용 기록(32개)이고, 이 기록과 따로 관리된다 | 둘을 함께 본다 |

## 확인 상태와 근거

- 2026-10-11 사용자: 빌드 성공, PIE에서 `Kata.ActionTrace.Dump` 출력과 GameplayDebugger 표시, 공격 중 회피 입력의 보관·전이 기록(위의 읽는 예). Shipping·Test 제외, AI 반복 합치기, 공용 중단 기록은 별도 확인 보고가 없다.
- 같은 날 추가한 `TriggerBufferUnmatched`: 사용자 PIE에서 백스텝 중 회피를 일찍 누르자 회피 창이 열리는 시점에 `TriggerBufferUnmatched ... (TooEarly)`가 남았다. 샘플의 공격 1·백스텝 회피 창은 PreAcceptSeconds가 0이다(에디터 MCP로 에셋 확인).
- [KataActionTrace.h](../../Plugins/Kata/Source/KataRuntime/Public/Runtime/KataActionTrace.h): 기록 구조체, 보관소, 빌드 매크로 `KATA_WITH_ACTION_TRACE`.
- [KataActionTrace.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionTrace.cpp): 콘솔 명령·변수, 출력 형식.
- [KataActionComponent.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionComponent.cpp): 시작·거절·종료·공용 중단 기록과 거절 상세.
- [KataGraphInstance.cpp](../../Plugins/Kata/Source/KataGraph/Private/KataGraphInstance.cpp): 그래프·트리거 기록과 불일치 이유.
- [GameplayDebuggerCategory_KataAction.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Debug/GameplayDebuggerCategory_KataAction.cpp): 화면 표시.
- [실행 이력 기록](../devlog/2026-10-11-Action-Trace.md): 결정 이유.
- [작업 상태](https://github.com/jaykop/Kata/issues/58).
