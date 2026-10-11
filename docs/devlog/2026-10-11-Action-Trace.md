# 액션 실행 이력

작성: 2026-10-11  
갱신: 2026-10-11  
유형: 결정 기록, 구현 기록  
대상: KataRuntime(`UKataActionComponent`, `UKataActionInstance`, 모듈·GameplayDebugger 카테고리), KataGraph(`UKataGraphInstance`)  
기준: 2026-10-11 작업 트리. #58 구현과 사용자 확인 결과

## 배경과 결론

캡콤 RE ENGINE 발표 요약을 검토하며 Kata에 가져올 만한 과정을 찾았다. 그중 RE:Log의 원리, 즉 문장 로그 대신 구조화된 기록을 남겨 판정 결과를 놓치지 않게 하는 방식을 액션 디버깅에 적용하기로 했다. 녹화·재생과 원격 수집은 가져오지 않았다.

당시 Kata에서 "왜 콤보가 안 이어졌나"를 알려면 로그를 훑거나 디버거를 붙여야 했다. 시작 결과(`EKataStartResult`)와 종료 사유는 호출자에게 전달된 뒤 남지 않았고, 그래프 트리거가 맞는 엣지가 없거나 엣지 조건에서 막히면 `SendTrigger`가 false만 돌려줬다. 그래프 에디터 Debug 탭 기록은 에디터 빌드 전용이었고 이 두 경우를 남기지 않았다. 직접 재생한 Kata는 `showdebug abilitysystem`과 GAS Inspector에도 보이지 않았다([#46 기록](2026-10-11-Kata-Action-Ability-Boundary.md)).

설계 초안과 결정을 거쳐([#58](https://github.com/jaykop/Kata/issues/58)) 캐릭터별 실행 이력을 코어에 넣었다. 사용법은 [실행 이력 사용법](../manual/Action-Trace.md)에 있다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 실행 결과 보관 | 없음 | `UKataActionComponent`가 고정 필드 기록(`FKataActionTraceEntry`)을 순환 보관한다. 기본 64개 |
| 기록 지점 | 없음 | 액션 시작·거절·종료, `CancelAbilitiesAndKata`, 전이·캔슬 창 열림과 닫힘, 그래프 시작·종료, 트리거 전이·예약·보관·불일치·무시 |
| 거절 상세 | 결과 코드만 반환 | 처음 걸린 태그, 차단 종류, StartCondition의 `FKataConditionResult`(L1) |
| 트리거 불일치 이유 | 남지 않음 | 엣지 없음, 창 닫힘, 출발 노드 조건, 엣지 조건, 도착 노드 조건, 실행 노드 없음 |
| 보관 트리거 재평가 실패 | 보관이 조용히 유지됨 | 후속 수정에서 `TriggerBufferUnmatched`와 이유를 남긴다. 창이 열렸는데 PreAcceptSeconds보다 앞선 입력은 `TooEarly` |
| 콘솔 | 없음 | `Kata.ActionTrace.Dump`와 `Enable`·`Capacity`·`LogLive` 변수, 로그 카테고리 `LogKataActionTrace` |
| 화면 | 코어에 디버거 카테고리 없음 | KataRuntime GameplayDebugger 카테고리 `KataAction` |
| 빌드 구성 | - | `KATA_WITH_ACTION_TRACE`(Shipping·Test에서 0) 안에 모두 둔다 |

## 주요 결정과 이유

### 위치와 형식

- 기록은 코어 KataRuntime의 `UKataActionComponent`가 가진다. 캐릭터별 실행 상태이므로 공유 에셋에 둘 수 없고, Kata 실행이 GAS 바깥에 있어 GAS 도구로는 보이지 않기 때문이다. KataGraph는 이미 KataRuntime에 의존하므로 그래프 이벤트도 같은 기록에 넣는다. 의존 방향은 바뀌지 않는다.
- 기록은 문자열과 배열이 없는 고정 크기 C++ 구조체다. 기록할 때 문자열을 만들지 않고 출력할 때만 글로 바꾼다. 에셋은 이름과 약한 참조로 남겨 GC를 막지 않는다.
- UHT는 `WITH_EDITOR`·`WITH_EDITORONLY_DATA` 이외의 전처리 조건 안에 둔 `UFUNCTION`·`UPROPERTY`를 받지 않는다. 기록을 Shipping에서 빼려면 리플렉션을 포기해야 해서 조회 API를 C++ 전용으로 정했다. 소비자(디버거 카테고리, 콘솔 명령, 샘플 디버그 메뉴 패널)가 모두 C++이라 문제가 없다. 그래프 에디터의 `FKataGraphDebugRecord`도 같은 방식이다.
- 실행 ID는 액션 인스턴스의 `GetUniqueID()`를 쓴다. 인스턴스에 필드를 더하지 않고 같은 실행의 시작·창·종료를 짝지을 수 있다.

### 2026-10-11 사용자 결정

| 항목 | 결정 | 고려한 대안 |
|---|---|---|
| 기본 활성 | 컴파일된 구성에서 상시 기록하고 `Kata.ActionTrace.Enable`로 끈다 | 켤 때만 기록. 문제가 생긴 뒤 되돌아보는 용도라 상시 기록을 택했다 |
| 보관 개수 | 캐릭터당 64개, 전역 콘솔 변수 | 컴포넌트 프로퍼티로 PC만 늘리기 |
| 기록 범위 | 필수 이벤트와 전이·캔슬 창 열림·닫힘 | 캔슬 요청 결과(`TryCancelKata`), 태스크 단위 시작·종료. 기록이 많아 제외했다 |
| 조건 실패 사유 | L1: 조건 결과(Fail·Invalid와 Reason)와 태그 거절의 첫 태그 | L2: 복합 조건 안의 말단 조건까지. KataConditions의 결과 반환 규정을 바꿔야 하고 Any 모드에서 의미가 모호하다 |
| 그래프 에디터 기록 | 그대로 둔다 | Debug 탭이 이 기록을 읽게 합치기. Debug 탭은 노드 약한 참조와 출처 GUID로 노드를 찾아가므로 이 기록이 자리 잡은 뒤 다시 본다 |
| 콘솔 명령 대상 | 기본 로컬 플레이어 Pawn, 액터 이름 일부, `All` | 치트 매니저(#54)의 대상 선택자. 코어는 KataTargeting에 의존할 수 없어 쓸 수 없다 |
| 실시간 로그 | `Kata.ActionTrace.LogLive`(기본 꺼짐)를 둔다 | 두지 않기. unreal-mcp에 콘솔 명령 실행 도구가 없어 에이전트가 명령 없이 로그를 읽는 경로가 필요했다 |

### 구현 세부

- 시작 판정 함수(`CanStartResolved`)는 바꾸지 않았다. 거절이고 기록이 켜져 있을 때만 사유에 맞춰 조건을 다시 평가하거나 걸린 태그를 찾는다. 조건 평가에 부작용이 없어 가능하고, 시작 판정 경량화([#47](https://github.com/jaykop/Kata/issues/47)) 경로에 비용을 더하지 않는다.
- 트리거 불일치 이유도 같은 원칙이다. 전이 선택(`SelectTransition`)은 그대로 두고, 불일치일 때만 같은 출발 노드를 다시 훑어 첫 이유를 찾는다.
- 직전 기록과 내용이 같으면 새 항목 대신 반복 수를 올린다. AI Task가 받아들여질 때까지 매 프레임 같은 트리거를 보내기 때문이다. 그래프 에디터 기록이 보관 기록을 합치는 것과 같은 이유다.
- `CanPlayKataAction` 같은 조회는 기록하지 않는다. AI·UI가 매 프레임 부를 수 있어 기록이 넘친다.
- 기록 순서를 읽기 쉽게 맞췄다. 액션 시작은 시각 0 태스크보다 먼저, 트리거 전이·예약은 전이 적용 전, 공용 중단 요청은 그 결과 종료보다 먼저 남긴다.
- GameplayDebugger는 엔진 모듈이다. `SetupGameplayDebuggerSupport`로 위성 플러그인과 같은 방식으로 연결해 코어에 엔진 플러그인 의존을 더하지 않았다.
- 그래프 컴포넌트 단계의 "그래프 없음" 트리거는 기록하지 않았다. 입력 처리 컴포넌트와 AI Task가 그래프 실행 여부를 먼저 확인해 트리거를 보내지 않으므로 실익이 작다.

### 기존 UE_LOG와의 역할

| 기록 대상 | 수단 |
|---|---|
| 실행마다 일어나는 판정 결과(시작·거절·종료·트리거·중단) | 실행 이력. 기본으로 로그에 쓰지 않는다 |
| 설정 오류, 실행 규정 위반, 수명 이상 | 기존 `UE_LOG` Warning·Error |
| 이력을 글로 꺼낸 결과 | `LogKataActionTrace` |

기존 로그 가운데 이력과 같은 판정 결과를 문장으로 남기는 줄(예: 그래프 액션 시작 거절의 Log)은 이번에 바꾸지 않았다.

### 후속 수정: 보관 트리거 재평가 실패

사용자 확인 스크린샷에서 빈틈이 드러났다. 회피 입력이 보관된 뒤 회피 창이 0.37초 뒤에 열렸지만 입력은 쓰이지 않았고, 창이 열릴 때 다시 평가해 실패한 사실이 기록에 없어 시각 차이로 추정해야 했다. 같은 날 `HandleTransitionWindowOpened`의 실패 경로에 `TriggerBufferUnmatched`를 더했다.

- 이유를 `detail`에 담아야 하므로 `TriggerTransition`의 `FromBuffer`처럼 보충 값으로 표시하지 않고 별도 이벤트로 두었다.
- 열린 창을 요구하는 엣지가 있을 때만 남긴다. 보관 중에는 트리거와 무관한 창이 열릴 때도 재평가가 일어나므로 모두 남기면 `NoEdge` 기록이 쌓인다.
- 창 검사에서 실패했는데 그 창이 열려 있으면 입력이 PreAcceptSeconds보다 앞선 것이므로 `TooEarly`로 `WindowClosed`와 구분한다. 일반 트리거 불일치에도 같은 구분이 적용된다.
- 확인 과정에서 샘플의 원인이 드러났다. 에디터 MCP로 읽은 `KA_BlackKnight_Attack01`(1.51초)과 `KA_BlackKnight_Backstep`(0.60초)의 회피 전이 창은 PreAcceptSeconds가 0이다. 콤보 창(0.2)과 달리 창이 열리기 전의 회피 입력은 보관돼도 항상 버려진다. 값 조정은 콘텐츠 결정이라 이번에 바꾸지 않았다.

### 주의한 함정

`KATA_WITH_ACTION_TRACE`는 `KataActionTrace.h`가 정의한다. 이 헤더를 `#if KATA_WITH_ACTION_TRACE` 안에서만 include하면 매크로가 정의되지 않은 채 0으로 평가되어 블록이 경고 없이 빠진다. 기록하는 cpp는 헤더를 조건 밖에서 include한다.

## 근거

- [KataActionTrace.h](../../Plugins/Kata/Source/KataRuntime/Public/Runtime/KataActionTrace.h)·[.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionTrace.cpp): 기록 구조체, 보관소, 콘솔 명령·변수, 출력 형식.
- [KataActionComponent.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionComponent.cpp): `AddTraceEntry`, `TraceStartResult`, 종료·공용 중단 기록.
- [KataActionInstance.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionInstance.cpp): 창 열림·닫힘 기록.
- [KataGraphInstance.cpp](../../Plugins/Kata/Source/KataGraph/Private/KataGraphInstance.cpp): 그래프·트리거 기록, `DescribeUnmatchedTrigger`.
- [GameplayDebuggerCategory_KataAction.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Debug/GameplayDebuggerCategory_KataAction.cpp), [KataRuntimeModule.cpp](../../Plugins/Kata/Source/KataRuntime/Private/KataRuntimeModule.cpp): 디버거 카테고리와 등록.
- unreal-mcp 도구 목록(2026-10-11): `EditorToolset.LogsToolset.GetLogEntries`는 카테고리·정규식으로 로그를 읽는다. `EditorAppToolset`에는 콘솔 변수 검색만 있고 명령 실행 도구가 없다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 빌드 | 사용자 빌드(2026-10-11) | 성공 | Shipping·Test 구성(사용자 결정으로 확인 대상에서 제외) |
| 로그 출력 | 사용자 PIE, `Kata.ActionTrace.Dump` | 기록 줄이 출력됨 | `All`·이름 지정, `Capacity`·`LogLive` 변수 |
| 화면 표시 | 사용자 PIE, GameplayDebugger | `KataAction` 카테고리에 현재 액션과 기록 표시 | NPC 대상 표시 |
| 콤보 끊김 사유 | 사용자 스크린샷 | 공격 중 회피 입력이 창 닫힘으로 보관됐다가, 새 입력으로 백스텝에 전이한 과정이 순서대로 보임 | 엣지 조건 실패·시작 거절·공용 중단 기록, AI 반복 합치기 |
| 보관 트리거 재평가 실패 기록 | 사용자 빌드·PIE 스크린샷(2026-10-11) | 백스텝 중 일찍 누른 회피가 창이 열릴 때 `TriggerBufferUnmatched ... (TooEarly)`로 남음 | 엣지·노드 조건 실패로 인한 재평가 실패 |

에이전트는 빌드·테스트를 실행하지 않았다.

## 남은 제한과 후속 작업

- 인게임 디버그 메뉴의 실행 이력 패널(계획의 AT-5)은 [#55](https://github.com/jaykop/Kata/issues/55)에서 패널 확장 규격(B2) 뒤에 만든다. 조회 API는 `GetTraceEntries`·`GetTraceSerial`이다.
- 조건 실패의 말단 조건 식별(L2)은 L1로 부족한 사례가 생길 때 다시 정한다.
- 기존 `UE_LOG` 가운데 이력과 겹치는 판정 결과 로그의 정리는 하지 않았다.
- 그래프 에디터 Debug 탭 기록과의 통합은 보류했다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#58](https://github.com/jaykop/Kata/issues/58) | 구현·확인 결과 댓글과 종료 |
| [실행 이력 사용법](../manual/Action-Trace.md) | 새 manual |
| [런타임 사용법](../manual/Runtime-Usage.md) | GAS 도구에 보이지 않는 Kata와 시작 거절 절에서 실행 이력으로 연결 |
| 액션 실행 이력 계획(`docs/plan/Action-Trace-Plan.md`) | #58을 닫으며 결정과 대안을 이 기록으로 옮기고 삭제했다 |
| [인게임 디버그 메뉴 계획(당시, 삭제됨)](https://github.com/jaykop/Kata/blob/406fcf9/docs/plan/In-Game-Debug-Menu-Plan.md) | 미반영. 다른 작업의 미커밋 변경이 있어 이번에 수정하지 않았다. 패널 추가는 #55에서 반영한다 |
