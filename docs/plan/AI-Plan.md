# KataAI 구현 계획

작성: 2026-10-05  
갱신: 2026-10-07  
연결 이슈: [#22 KataAI](https://github.com/jaykop/Kata/issues/22)  
현재 상태 근거: [AI 사용법](../manual/AI.md), [AI 변경 기록](../devlog/2026-10-05-AI-Lifecycle.md), [Action Group 사용법](../manual/Action-Group.md)  
대체 관계: 없음. 2026-10-07에 구현된 사용 규칙을 manual·devlog로 넘기고 미확정 설계만 남겼다. 이전 판의 단계별 제안 원문은 Git 기록에 있다.

## 목적과 현재 상태

몬스터가 적대 대상을 인지하고 추적해 KataAction을 실행하도록 한다. 행동 정책은 StateTree가 결정하고, 인지·대상 선택·실행 수명은 KataAI 코드가 담당한다.

이미 구현된 범위와 사용 규칙은 [AI 사용법](../manual/AI.md)을 따른다. 아래 항목은 이 문서에서 다시 설명하지 않는다.

- KataAI 플러그인, `AKataAIController`, `UKataAIData`, NPC 행·`AKataAICharacter` 연결, StateTree 시작·정지 수명.
- Controller별 Perception, Sight 후보 → `Kata Select Perceived Actors` → Preset 필터·정렬 → 현재 대상 유지·교체, 마지막 감지 위치·Home 기억.
- `Kata AI Context` Evaluator, `StateTree.Event.AI.TargetAcquired / TargetLost / TargetChanged` 이벤트, `Clear Kata AI Target Memory` Task.
- `Play KataAction`·`Play KataGraph`·`Play KataActionGroup` Task와 Graph 종료 관측 API.
- 엔진 MoveTo·Delay Task로 구성하는 Chase·Attack·Search·Return 샘플 절차.

남은 것은 기본 루프의 실패 정책, StateTree 조립 구조, 행동 패턴 배치, 후속 감각·어그로·Pressure 설계와 Debugger다.

## 범위

- 포함: 인지·타게팅·StateTree의 역할 경계, 트리 조립 방식, 비전투·전투 행동 배치, 이동 실패·복귀 정책, Pressure·Capability 설계, AI Debugger.
- 제외: 피격·사망·스탯·GA 지급, 네트워크, BT 어댑터, 별도 콤보 실행기. 콤보는 기존 KataGraph를 사용한다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| StateTree 기반 | 확정 | #22·AGENTS. BT 의존·어댑터 없음. 엔진 `UStateTreeAIComponent`와 AI Component 스키마 재사용 |
| 첫 행동 범위 | 확정 | 2026-10-05 사용자: 시각 감지·추적·공격·복귀. 시야 상실 시 마지막 감지 위치로 이동 후 일정 시간 뒤 복귀 |
| 대상 이벤트 태그 | 확정 | `StateTree.Event.AI.*` 고정 태그. [Gameplay Tags](../manual/Gameplay-Tags.md) |
| 팩션·어그로 | 확정 | PC 전용 전제 없음. 어그로는 후속에 몬스터·대상 쌍별로 관리 |
| Pressure 수명 | 확정 | 순수 조건은 가능 여부만 조회하고 실행 Task가 예약, 실행 종료 시 자동 해제 |
| 역할 경계 | 제안 | 아래 「인지·타게팅·StateTree 역할」 |
| 트리 조립 | 확정 | 2026-10-07 사용자: 안 A. 마스터 트리 하나와 AI Data의 Linked 슬롯 오버라이드. 아래 「StateTree 조립 구조」 |
| 행동 배치 | 제안 | 아래 「행동 패턴 배치」 |
| 실패 처리의 책임 | 확정 | 2026-10-07 사용자: 시스템은 설정 손잡이와 안전 보장을 제공하고, 값과 옵션 선택은 몬스터 디자인이 정한다 |
| 기본 루프 실패 처리 | 확정 | 2026-10-07 사용자: 재시도 상한(0=무제한), 복귀 불가 시 제자리 유지 또는 순간이동, 캐릭터별 추격 한계, 복귀 중 재감지 시 재교전. 아래 「기본 루프 실패 처리」 |
| Pressure·Capability 상세 | 결정 필요 | 아래 「Pressure와 실행 허가」, 「미확정 결정」 |

## 인지·타게팅·StateTree 역할 (제안)

세 계층은 각각 "무엇을 알 수 있는가", "그중 누구를 노리는가", "그래서 무엇을 하는가"를 맡는다. StateTree는 Perception을 직접 읽지 않고 AI 타게팅 컴포넌트를 단일 정보원으로 사용한다.

| 계층 | 책임 | 하지 않는 일 |
|---|---|---|
| Perception (Controller) | 감각 자극 수신과 팀 소속 사전 필터, 감각별 기억 수명 | 공격 대상 선택, 우선순위 판단 |
| AI 타게팅 (Pawn 컴포넌트) | 인지 기록 보관, 후보 공급, Preset 필터·정렬, 현재 대상 유지·교체, 기억 위치·Home, 대상 변화 이벤트 | 이동·공격 결정, 실제 위치 투시 |
| StateTree | 사실 스냅샷과 이벤트로 행동 상태 선택, 실행 Task 호출 | 후보 수집, 인지 기록 수정(복귀 완료 시 기억 정리 Task만 예외) |

후보 원칙: 공격 대상은 인지 기록에서 온 후보 중에서만 고른다. PC 타게팅의 Preset은 월드 검색 Selection을 사용하고, AI Preset은 Selection만 `Kata Select Perceived Actors`로 바꾼다. 필터·정렬 태스크는 공유한다. 이렇게 하면 벽 너머 대상을 읽는 투시를 막고 PC와 AI가 같은 판정 부품을 쓴다.

후보 확장 제안: 현재 후보는 "지금 보이는 액터"뿐이다. 청각·피해 감각을 추가하면 인지 기록을 다음 두 종류로 나눈다.

- 확정 대상: 시각으로 확인했거나 피해를 준 액터. Preset 후보가 된다.
- 단서: 위치만 확실한 소리·흔적. 공격 후보가 아니며 Investigate 상태의 입력이 된다.

StateTree에 전달할 정보는 아래와 같다. 엔진 조건으로 계산 가능한 값(대상 거리, Home 거리)은 Evaluator에 중복 추가하지 않고 엔진 `Distance Compare` 조건에 위치를 바인딩한다.

| 종류 | 현재 제공 | 후속 제안 |
|---|---|---|
| 대상 결정 결과 | Target Actor, Has Visible Target | 교전 중 여부(Engaged): 대상이 잠시 사라져도 수색이 끝날 때까지 유지 |
| 기억 | Has Last Known Location, Last Known Location, Time Since Last Seen | 단서 위치·감각 종류·시각, 마지막 공격자 |
| 기준 위치 | Home Location | 순찰 경로·로밍 영역의 복귀 기준점 |
| 이벤트 | TargetAcquired, TargetLost, TargetChanged | ClueReceived(청각·흔적), Damaged |
| 자기 상태 | 실행 Task의 Result·Details | ASC 태그 조회 조건(사망·기절·무력화) |

이벤트는 상태 변경 알림이고 StateTree는 항상 Evaluator의 최신 스냅샷으로 판단한다. 이벤트 Payload를 장기 상태로 쓰지 않는다.

## StateTree 조립 구조 (확정: 안 A)

몬스터마다 트리 전체를 따로 만들지 않는다. 마스터 트리 하나를 두되, 몬스터별 차이를 세 층으로 흡수한다.

| 차이의 종류 | 담당 | 예시 |
|---|---|---|
| 수치 | AI Data의 StateTree 파라미터 오버라이드 | 공격 간격, 수색 시간, 추격 한계 거리 |
| 공격 패턴 | Action Group 에셋 | 물기·돌진 가중치, 콤보 Graph |
| 행동 구조 | Linked Asset 상태의 하위 트리 교체 | 비전투 루틴(대기·순찰·로밍), 전투 아키타입(근접·원거리·보스) |

마스터 트리는 Disabled·Combat·Search·Return·Routine 골격, 공통 이벤트 전이, 기억 정리를 소유한다. 몬스터별로 바뀌는 Routine과 Combat 내부는 Linked Asset 상태로 두고 하위 트리를 연결한다.

UE 5.8의 `UStateTreeComponent`는 `SetLinkedStateTreeOverrides`·`AddLinkedStateTreeOverrides`로 태그별 Linked Asset 교체를 제공한다(StateTreeComponent.h). 이 기능으로 슬롯을 연결한다.

- `UKataAIData`에 슬롯 태그 → `FStateTreeReference` 오버라이드 목록을 추가한다. 하위 트리는 AI Data와 함께 로드되도록 하드 참조로 둔다.
- Controller는 마스터 트리를 할당한 뒤 StartLogic 전에 목록 전체를 `SetLinkedStateTreeOverrides`로 적용한다. 빈 목록이면 마스터 트리의 Linked 상태에 지정된 기본 하위 트리를 사용한다.
- 슬롯 태그 루트는 `StateTree.Slot`이다(2026-10-07 사용자 결정). 예: `StateTree.Slot.Routine`, `StateTree.Slot.Combat`. 기존 `StateTree.Event.*`와 같은 `Config/Tags/Native/StateTree.ini`에 프로젝트가 정의하고, 플러그인은 `Categories="StateTree.Slot"` 메타로 선택기만 제한한다. 루트는 [Gameplay Tags](../manual/Gameplay-Tags.md) 목록에 추가한다.
- 데이터 검증은 중복 슬롯 태그와 빈 트리 참조를 진단한다. 하위 트리의 스키마가 AI Component 스키마와 맞는지도 확인한다.
- 실행 중 트리 교체는 지원하지 않는 현재 규칙을 유지한다. 오버라이드는 빙의·시작 시에만 적용하고, 재빙의 때 다시 적용한다.

아키타입별 마스터 트리를 복사하는 안 B는 골격 수정이 여러 트리로 퍼지므로 채택하지 않았다. 보스처럼 골격 자체가 다른 개체는 AI Data에 전용 마스터 트리를 지정할 수 있다.

### 몬스터별 사용 구조 (제안, 2026-10-08)

2026-10-08 사용자 요청으로 샘플 마스터 트리 `/Game/KataTest/AI/ST_KataAI_Master`를 만든다. 플러그인은 트리 에셋을 제공하지 않고 Task·조건·Evaluator만 제공하는 현재 원칙을 유지하므로 마스터 트리는 샘플 콘텐츠다.

몬스터는 자기 AI Data(`DA_AI_<Monster>`)만 갖고 마스터 트리를 공유한다. 몬스터별 차이는 아래 순서로 흡수하며, 앞 단계로 해결되면 뒤 단계를 쓰지 않는다.

| 단계 | 수단 | 몬스터별로 바꾸는 것 | 구현 상태 |
|---|---|---|---|
| 1 | AI Data 필드와 StateTree 파라미터 오버라이드(값) | 실패 처리 값은 AI Data 필드(MaxMoveRetries, MoveRetryInterval, LeashDistance), 그 외 AttackInterval·SearchDuration은 파라미터 오버라이드 | AI-5에서 실패 처리 값 이동 |
| 2 | AI Data의 StateTree 파라미터 오버라이드(에셋 참조) | AttackGroup(`UKataActionGroup`). 공격 패턴은 Action Group으로 바꾼다 | 기존 기능. 마스터 트리에 Object 파라미터가 필요 |
| 3 | AI Data의 Linked 슬롯 오버라이드 | `StateTree.Slot.Routine`(Idle·Patrol·Roam), `StateTree.Slot.Combat`(근접·원거리 등 전투 방식) | AI-5 |
| 4 | AI Data에 전용 마스터 트리 지정 | 골격 자체가 다른 보스 | 기존 기능 |

마스터 트리 파라미터는 다음과 같다. MCP는 루트 파라미터를 만들거나 바인딩할 수 없으므로 사용자가 에디터에서 추가·연결한다.

| 파라미터 | 타입 | 연결 위치 |
|---|---|---|
| AttackGroup | Object(KataActionGroup) | Attack의 Play KataActionGroup.Group |
| AttackInterval | Float | AfterAttack의 Delay.Duration |
| SearchDuration | Float | SearchWait의 Delay.Duration |
| MoveRetryInterval | AI Data 필드(안 2) | 세 Retry 상태의 Delay.Duration ← Kata AI Context 출력 |
| MaxMoveRetries | AI Data 필드(안 2) | 실패 전이의 상한 조건 ← Kata AI Context 출력 |
| LeashDistance | AI Data 필드(안 2) | 추격 한계 전이, 재교전 조건 ← Kata AI Context 출력 |

#### Linked 슬롯의 엔진 제약 (UE 5.8 소스 확인, 2026-10-08)

- 태그가 있는 Linked Asset 상태의 Parameters에 바인딩이 하나라도 있으면 컴파일러가 런타임 오버라이드를 끈다(`StateTreeCompiler.cpp`의 `bCanOverrideLinkedAssetAtRuntime`). 따라서 교체 가능한 슬롯은 마스터 트리 파라미터나 Evaluator 출력을 바인딩으로 받을 수 없다.
- 오버라이드된 하위 트리의 파라미터 값은 오버라이드 항목의 `FStateTreeReference` 파라미터에서 온다(`StateTreeExecutionContext.cpp`). 오버라이드하지 않은 기본 하위 트리는 Linked 상태에 적은 상수 값을 쓴다.
- 오버라이드 트리는 컴파일 완료, 마스터와 같은 스키마 클래스, 호환되는 컨텍스트 데이터를 만족해야 하며 아니면 오버라이드 전체가 무시된다.
- Linked Asset 하위 트리는 자기 Evaluator와 Global Task를 실행한다. Combat 하위 트리는 자체 `Kata AI Context`를 두어 대상·위치를 읽는다. Evaluator는 컴포넌트 값을 읽기만 하므로 마스터와 중복 실행해도 상태가 갈라지지 않는다.

이 제약 때문에 마스터 파라미터를 슬롯에 넘기는 방식은 쓸 수 없다. 2026-10-08 사용자 결정(안 2)으로 마스터와 슬롯이 함께 쓰는 실패 처리 값 (MaxMoveRetries, MoveRetryInterval, LeashDistance)은 `UKataAIData` 필드로 옮기고 `Kata AI Context` 출력으로 제공한다. 슬롯에서만 쓰는 값(AttackGroup, AttackInterval 등)은 슬롯 하위 트리의 파라미터로 두고 AI Data의 슬롯 항목에서 지정한다. 이 결정은 "행동 수치를 AI Data 필드로 중복 추가하지 않는다"는 2026-10-05 결정을 실패 처리 값에 한해 바꾼다. 대신 중복이 없어지고 MoveRetryInterval > 0을 데이터 검증할 수 있다.

슬롯 하위 트리는 마스터 상태로 직접 전이할 수 없다. Combat 하위 트리는 "다시 판단"을 Succeeded, "포기하고 복귀"를 Failed로 끝내고, 마스터의 Combat Linked 상태가 Succeeded → Root, Failed → Return으로 전이한다. TargetLost·TargetChanged는 Attack 보호를 위해 하위 트리의 Chase 안에서 처리한다.

AI-5 전까지 Combat과 Routine은 마스터 트리 안의 일반 상태로 둔다. AI-5에서 두 상태를 Linked Asset 상태로 바꾸고, 현재 내용을 기본 하위 트리(`ST_KataAI_Combat_Melee`, `ST_KataAI_Routine_Idle`)로 옮긴다. 이때 하위 트리가 마스터 파라미터를 받는 방식(Linked 상태 파라미터 바인딩)을 함께 정한다.

## 기본 루프 실패 처리 (확정 방향)

시스템은 설정 손잡이와 안전 보장만 제공한다. 몬스터별 값과 옵션은 StateTree 파라미터와 AI Data의 파라미터 오버라이드로 정한다. 모든 경우에 프레임 단위 재시도와 영구 정지가 없어야 한다.

| 항목 | 설정 | 동작 | 구현 |
|---|---|---|---|
| 이동 재시도 상한 | `MaxMoveRetries`, 0이면 무제한 | 이동 실패마다 `MoveRetryInterval`만큼 기다린 뒤 다시 시도하고, 상한에 도달하면 실패 처리로 넘어간다 | 코드 필요. 상한 값은 몬스터별 StateTree 파라미터, 현재 횟수는 `UKataAITargetingComponent`의 실행 상태로 보관한다(2026-10-07 사용자 결정). 이동 성공·교전 시작·기억 정리·재빙의 때 초기화한다 |
| 재시도 간격 | `MoveRetryInterval` > 0 | 무제한일 때도 간격 대기를 반드시 거친다 | 엔진 Delay Task의 파라미터라 코드에서 검증할 수 없다. manual 구성 절차로 안내한다 |
| 복귀 불가 처리 | 옵션 `Stay` / `Teleport` | Stay는 현재 위치를 새 Home으로 기록하고 기억을 정리한다. Teleport는 복귀 지점으로 순간이동한 뒤 기억을 정리한다 | 코드 필요. 아래 Task |
| 추격 한계 | `LeashDistance`, 0이면 무제한 | Pawn과 Home의 거리가 한계를 넘으면 Combat에서 Return으로 전이한다 | 에셋 구성. 엔진 `Distance Compare` 조건에 Pawn 위치와 Home Location을 바인딩한다 |
| 복귀 중 재감지 | — | 재교전한다 | 에셋 구성. 기존 Return의 TargetAcquired 전이를 유지한다 |
| 경계 왕복 방지 | — | 대상이 추격 한계 안에 있을 때만 Return에서 재교전한다 | 에셋 구성. 대상 위치와 Home Location의 `Distance Compare`를 재교전 전이 조건에 추가한다 |

복귀 불가 처리 Task `Resolve Kata AI Return Failure`의 동작 규칙은 다음과 같다.

- 입력: 처리 옵션, 복귀 지점(기본은 Home Location에 바인딩하며, 스포너 지점 등 다른 위치를 바인딩할 수 있다).
- Teleport는 이동 요청을 중지한 뒤 `TeleportTo`로 이동하고 도착 위치를 새 Home으로 기록한 뒤 기억을 정리한다. 충돌 등으로 순간이동이 실패하면 경고를 남기고 Stay로 처리해 영구 정지를 막는다.
- Stay는 Home을 현재 위치로 바꾸므로 이후 교전 종료 때 같은 복귀 실패를 반복하지 않는다. 원래 Home으로 되돌리는 정책은 이번 범위에 없다.
- 두 옵션 모두 즉시 완료하며 Returned 상태와 같은 방식으로 다음 트리 갱신 뒤 Root를 재선택한다.
- Home 재지정은 AI 타게팅 컴포넌트에 쓰기 API를 추가해 수행한다. 공유 에셋에는 쓰지 않는다.

## 행동 패턴 배치 (제안)

Root의 자식은 위에서부터 선택 우선순위다. StateTree는 진입 조건을 통과하는 첫 자식을 선택한다.

| 순서 | 부모 상태 | 진입 조건 | 자식·동작 |
|---|---|---|---|
| 1 | Disabled | 사망·기절 등 ASC 태그 | 이동 중지, 실행 Task 없음. 생명·GAS 규칙과 연결하는 별도 범위 |
| 2 | Combat | 보이는 대상 있음 | Engage(선택): 첫 발견 반응 한 번. Approach: 추적. Position: 대치·거리 유지(Pressure 대기 포함). Attack: Linked 아키타입 또는 Play KataActionGroup. Recover: 공격 간격 대기 |
| 3 | Search | 대상 없음, 마지막 위치 있음 | Move: 마지막 위치 스냅샷으로 이동. Wait: 둘러보기 |
| 4 | Investigate(후속) | 단서 있음 | 단서 위치 이동, 관찰. 확정 대상이 생기면 Combat |
| 5 | Routine | 기본값 | Linked 슬롯: Idle(제자리), Patrol(경로), Roam(영역 내 무작위) |
| — | Return | 명시 전이로만 진입 | Search 소진·추격 한계·Investigate 소진 후 Home으로 이동, 도착 시 기억 정리 |

사용자가 말한 "경계"는 두 상황으로 나눈다. 비전투 경계(의심스러운 소리를 듣고 살피는 것)는 Investigate, 전투 중 경계(대치·견제)는 Combat/Position이다. 모든 발견이 경계를 반드시 거치게 하지 않고, 확정 대상은 바로 Combat에 들어간다.

Return을 Root 선택 조건(Home에서 멀리 있음)으로 두지 않는 이유: Patrol·Roam도 Home에서 멀어지므로 루틴 중 복귀가 오발동한다. Return은 교전이 끝난 뒤의 전이로만 들어간다. 추격 한계를 넘으면 Combat 부모의 전이로 Return에 강제 진입한다. 복귀 중 재감지와 경계 왕복 방지는 「기본 루프 실패 처리」를 따른다.

공격 보호: Combat 부모에 TargetLost·TargetChanged 이탈 전이를 두지 않는다. 해당 전이는 Approach·Position·Recover에 둔다. Attack은 Task 완료 후 최신 상태로 재판단한다. 사망·기절 같은 강제 중단은 Disabled의 우선 선택으로 처리한다.

## Pressure와 실행 허가 (제안)

판정을 셋으로 나눈다. GAS 비용·쿨다운은 복제하지 않는다.

- Capability: 이 실행을 할 자격(무기·상태·전투 역할).
- GAS CanPlay: 비용·쿨다운 등 기존 Action 실행 조건.
- Pressure: 여러 AI가 같은 대상에게 동시에 가하는 공격 부담의 허용량.

흐름은 대상 결정 → 행동 판단 → 실행 후보 선택 → 실행 직전 Pressure 예약 → Action/Graph 실행 → 예약 해제다.

- Pressure는 대상 후보 필터가 아니다. 허가가 없어도 같은 대상을 추적하거나 Position에서 대치한다.
- 예약은 추적 때가 아니라 공격 시작 직전에 잡는다. 시작 실패·순간 완료·종료·중단·트리 이탈·빙의 해제·대상 무효화에서 여러 번 호출돼도 안전하게 해제한다.
- 첫 범위는 대상별 예산이다. 같은 대상을 공격하는 AI들이 비용 합계 상한을 공유한다. 예약 관리자는 KataAI의 월드 범위 소유자 후보이며 공유 에셋에 예약 상태를 저장하지 않는다.
- Graph는 전체 수명 동안 예약 하나를 유지한다. 실행 중 대상이 바뀌어도 예약은 시작 시 고정한 대상에 묶는다.
- Pressure 부족만으로 대상 교체나 Group 재추첨을 반복하지 않는다.

## 미확정 결정

기본 루프와 슬롯 연결의 세부는 2026-10-07 사용자가 추천안으로 확정했다. 슬롯은 `StateTree.Slot.Routine`과 Combat 내부 전체를 맡는 `StateTree.Slot.Combat`이다. 근접·원거리형은 접근·거리 유지부터 다르고 공격 종류 차이는 Action Group이 맡기 때문이다. 재시도 판정은 증가 Task와 순수 조건으로 나눈다.

후속 설계에 필요한 결정:

1. Pressure 비용 저장 위치: Action Group 항목 Payload, Action 에셋, AI Data 중 선택. [Action Group 계획](Action-Group-Plan.md)과 함께 정한다.
2. Position에서 Attack으로 돌아가는 계기: 예약 해제 이벤트 또는 주기 재확인. 대기자 공정성·기아 방지 방식.
3. 순환 Graph·무한 Action의 예약 점유: 전투 Group에서 금지, 최대 점유 시간 중 선택.
4. Capability 표현과 태그 루트, 어그로 증가·감쇠·교체 임계값, 대상별·그룹별 Pressure 범위.
5. 후속 인지 기록의 확정 대상·단서 구분과 Evaluator 출력 확장.

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| AI-4 | 높음 | 재시도 상한, 복귀 불가 처리 Task, Home 재지정 API. StarvedHound 샘플에 추격 한계·재교전 조건 구성 | — | 이동 불가·복귀 불가에서 무한 대기·프레임 재시도 없음. Stay·Teleport 모두 기억 정리 후 Routine 복귀 |
| AI-5 | 높음 | AI Data 슬롯 오버라이드와 Controller 적용, 마스터 트리 골격 | AI-4 확인 | 한 마스터 트리에서 Routine·Combat 하위 트리를 개체별로 교체 |
| AI-6 | 보통 | GameplayDebugger 카테고리 | AI-4 | 행동 상태·대상·기억 위치·재시도 횟수·Task 결과 표시. Pressure 예약은 AI-8 이후 추가 |
| AI-7 | 후속 | 청각·피해 감각, 단서·Investigate, 어그로 | 결정 4·5 | 단서와 확정 대상 구분, 대상별 어그로로 교체 |
| AI-8 | 후속 | Pressure·Capability 예약 | 결정 1~4 | 두 AI의 동시 허가 경쟁, 모든 종료에서 예약 해제 |

AI-7·AI-8은 #22 범위에 남아 있으며 기본 루프 확인 전에 자동으로 구현하지 않는다. 여러 AI의 동시 공격이 실제 문제로 확인되면 AI-8을 AI-7보다 앞당길 수 있다.

## 영향과 제한

- 모듈 경계: KataAI는 KataRuntime·KataGraph·KataTargeting과 엔진 AIModule·StateTreeModule·GameplayStateTreeModule·TargetingSystem을 사용한다. KataFramework·KataCamera에 의존하지 않는다.
- Linked 슬롯 오버라이드는 `UKataAIData`에 필드를 추가한다. 기존 AI Data는 빈 목록으로 기존 동작을 유지한다.
- Home 재지정 API는 `UKataAITargetingComponent`의 공개 표면을 늘린다. 기존 Home 기록 시점(정상 빙의)은 유지한다.
- 새 StateTree 파라미터(`MaxMoveRetries`, `LeashDistance`)는 기존 샘플 트리에 자동으로 추가되지 않는다. 사용자가 샘플 트리에 구성한다.
- 실행 상태(대상, 기억, 예약)는 컴포넌트·Task 인스턴스·월드 범위 소유자가 가지며 공유 에셋에 저장하지 않는다.
- 샘플 에셋은 `Content/KataSample`에 둔다.

## 사용자 확인 항목

빌드·PIE 확인은 사용자가 담당한다. 에이전트는 요청이 없으면 빌드·테스트·별도 검사를 실행하지 않는다. 현재 기본 루프의 확인 절차는 [AI 사용법의 실행 확인 가이드](../manual/AI.md)를 따른다. 후속 단계별 확인은 해당 단계 구현 시 이 절에 추가한다.

## 완료 시 갱신할 문서

- [#22](https://github.com/jaykop/Kata/issues/22): 단계별 구현·사용자 확인 결과. 게시·라벨 변경은 초안 승인 후.
- [AI 사용법](../manual/AI.md), [Gameplay Tags](../manual/Gameplay-Tags.md): 실패 정책, 슬롯 태그, 새 Evaluator 출력과 이벤트.
- [AI 변경 기록](../devlog/2026-10-05-AI-Lifecycle.md) 또는 주제별 신규 devlog: 트리 조립·Pressure 선택 이유.
- [문서 목록](../README.md): 신규 문서가 생길 때 갱신.
