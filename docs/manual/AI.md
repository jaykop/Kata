# KataAI 사용법

갱신: 2026-10-09  
대상: KataFramework AI 캐릭터·NPC 생성과 StateTree 실행 설정  
적용 기준: [#22](https://github.com/jaykop/Kata/issues/22) AI-1·AI-2·AI-3 실행 Task 소스  
확인 상태: 소스 구현. 사용자 빌드·배치·스폰·실행 미확인

## 목적과 준비

KataAI의 `UKataAIData`가 StateTree·파라미터·Sense·Targeting Preset 설정을 모은다. NPC 행은 AI Data를 참조하고 Character는 로드된 에셋을 보관한다. Controller는 StateTree와 Controller별 Perception 실행 상태를 소유한다.
현재 제공하는 범위는 AI Data 적용, StateTree·Perception 수명, 시각 인지 후보의 대상 선택과 Evaluator 출력이다. Action·Graph·Action Group 실행 Task와 수색·복귀·이동 실패 처리용 Task·조건을 제공한다. 추적·수색·복귀 상태는 아래 절차로 사용자가 StateTree에 구성하며 자동 전투 트리를 생성하지 않는다.

## 사용 순서

1. 빌드 후 `Kata AI Character`를 부모로 캐릭터 Blueprint를 만든다. 기존 NPC를 자동으로 변환하지 않는다.
2. Data Asset 생성에서 `Kata AI Data`를 선택한다. State Tree에 `StateTree AI Component` 스키마의 트리와 파라미터 오버라이드를 지정한다. 트리 스키마의 AIController Class는 `Kata AI Controller` 또는 해당 파생 클래스로 지정한다.
3. AI Data의 Senses 목록에 원하는 Sense Config를 추가하고 감각별 설정을 지정한다. Dominant Sense는 목록에 포함된 감각을 선택하며 비우면 첫 유효 감각을 사용한다. Targeting Preset에는 아래 시각 후보 Selection과 필터·정렬을 구성한다.
4. `FKataNPCCharacterRow`의 Character Class를 위 AI 캐릭터로, AI Data를 만든 설정 에셋으로 지정한다. AI Controller Class는 행에서 지정하거나 캐릭터 BP 기본값을 사용한다.
5. 테이블을 데이터 컬렉션의 NPC Tables에 등록하고, NPC 스포너·비동기 생성 경로에서 해당 캐릭터 ID로 생성한다. 여러 NPC 테이블을 등록할 수 있으며 전체 캐릭터 테이블의 행 이름은 중복되면 안 된다.
6. Pawn·ASC 준비 후 Controller가 설정을 적용한다. StateTree가 비어 있으면 행동 트리를 실행하지 않으며 Senses가 비어 있으면 Perception Listener를 만들지 않는다.

샘플 에셋은 이번 단계에서 생성하거나 변경하지 않았다. 확인용 기본 트리에 엔진 Delay Task를 두면 실제 공격 구성 없이 Controller·트리 연결을 확인할 수 있다.

## 주요 설정과 실행 계약

| 항목 | 의미 | 빈 값·실패·수명 |
|---|---|---|
| 캐릭터 AIData 참조 | NPC 행에서 받은 실행용 참조 | Transient이며 BP·배치 인스턴스에서 편집하거나 저장하지 않음 |
| NPC 행 AI Data | 생성 전에 로드할 소프트 참조 | 비우면 인지·행동 로직을 실행하지 않음 |
| AI Data State Tree | 트리·파라미터 오버라이드 | FStateTreeReference 사본을 Controller에 적용 |
| Senses | 에셋 내부의 감각 설정 원본 | Controller마다 복제. 빈·중복·구현 없는 항목은 경고 후 제외 |
| Dominant Sense | 감지 위치 판정에 우선할 감각 | 미지정은 첫 유효 감각. 미등록 감각은 검증 오류, 런타임 경고 후 첫 유효 감각 |
| Targeting Preset | 후보 필터·정렬 | 인지 변경과 Target Refresh Interval마다 실행. 빈 값이면 대상 없음 |
| NPC 행 AI Controller Class | AKataAIController 파생 클래스 | 비우면 캐릭터 Blueprint 기본값 유지 |
| IKataAIPawnInterface | Controller가 Pawn의 설정·준비 여부 조회 | Framework 역참조 없이 연결. 사용자 C++ Pawn도 구현 가능 |
| TryStartKataAI | 준비된 Pawn의 트리 시작 | Controller·Pawn 게임 준비와 ASC 필요. 한 빙의에서 한 번만 시도 |
| Controller 팀 번호 | Pawn 타게팅 컴포넌트의 팩션 | 컴포넌트가 없으면 NoTeam |

StateTree 자동 시작은 꺼져 있다. NPC 행의 AI Data가 설정 기준이며 Controller 컴포넌트에 직접 지정한 트리는 AI Data 값으로 대체된다. 파라미터 동기화는 사본에서 수행하며 공유 에셋을 수정하지 않는다.
Perception은 Controller마다 동적으로 생성한다. Controller BP에는 별도 Perception 컴포넌트를 추가하지 않는다. 이미 있으면 중복 등록 대신 경고하고 AI Data Sense 적용을 생략한다. 재빙의·종료 시 해당 Listener와 감지 기록을 제거한다.
현재 대상·마지막 감지 위치·타이머는 AI Data에 저장하지 않는다. 행동 거리·수색 시간·공격 간격은 StateTree 파라미터로 지정하고 같은 필드를 중복 추가하지 않는다.
캐릭터 BP를 직접 배치하면 NPC 행이 적용되지 않으므로 AI Data가 설정되지 않는다. NPC 행 기반 스포너를 배치한다.

### 기존 설정 이관

이전 BP 또는 NPC 행의 State Tree를 새 AI Data의 State Tree로 옮기고 NPC 행의 AI Data에 해당 에셋을 지정한다. 이전 NPC 행 StateTree는 직렬화 값을 보존하는 비편집 필드로 남으며 런타임에서 사용하지 않는다. 이전 트리만 지정된 행은 이관 안내 로그와 함께 생성에 실패한다.
StateTree와 AIData는 다른 에셋 타입이므로 Property Redirect로 변환하지 않는다. 기존 uasset은 자동 변경하지 않았다.
시작에 실패해도 매 프레임 다시 시도하지 않는다. 설정을 고친 뒤 재빙의하거나 PIE를 다시 시작한다. 실행 중 트리 교체 API는 이번 범위에 없다.
빙의 해제·Controller 종료에는 트리·이동·Gameplay Focus·트리 참조를 정리한다. AI 캐릭터 종료는 Controller의 빙의를 먼저 해제해 Pawn 컴포넌트가 남아 있을 때 트리 종료 처리를 수행한다.

## 제한과 문제 해결

| 증상 | 원인·확인 항목 |
|---|---|
| AI 설정을 지정한 NPC 생성 실패 | Character Class가 AKataAICharacter 계열인지, 지정 에셋이 로드됐는지 확인 |
| Controller가 있지만 트리가 시작되지 않음 | NPC 행 AI Data·설정 에셋의 State Tree, 행 기반 생성, AI Component 스키마, ASC Actor Info 확인 |
| 적이 감지·공격하지 않음 | Sight 설정·적대 팩션·Preset의 Selection 확인. 공격 상태의 실행 Task·대상 바인딩·완료 전이 확인 |

AI 필드를 비운 기존 NPC 행과 일반 AKataCharacter는 유지한다. 기존 PC의 생성·입력 설정은 이번 변경 대상이 아니다.

## 확인 상태와 근거

AI-1 최초 구현은 사용자가 Editor 빌드 성공을 보고했다. AI Data 추가 이후의 빌드·실행은 미확인이다. 에이전트 빌드·테스트·별도 검사는 실행하지 않았다. 사용자 확인은 AI Data 생성·행 기반 로드·트리 파라미터·같은 에셋을 쓰는 여러 Controller의 독립 Sense 설정·빙의 해제 정리가 필요하다.

- [Controller](../../Plugins/KataAI/Source/KataAI/Public/Controller/KataAIController.h).
- [AI Data](../../Plugins/KataAI/Source/KataAI/Public/Data/KataAIData.h).
- [AI 캐릭터](../../Plugins/KataFramework/Source/KataFramework/Public/Character/KataAICharacter.h).
- [변경 기록](../devlog/2026-10-05-AI-Lifecycle.md).

## 시각 타게팅과 StateTree 바인딩

AI 캐릭터의 기본 타게팅 서브오브젝트는 `UKataAITargetingComponent`다. 새 컴포넌트를 BP에 추가하지 않는다. 사용자 C++ Pawn은 인터페이스와 이 컴포넌트를 함께 제공해야 시각 대상 연결을 사용할 수 있다.

1. AI Data Senses에 Sight Config를 지정하고 Detection by Affiliation에서 대상의 관계가 감지되도록 설정한다. 공격 후보는 Kata Factions에서 적대 관계여야 한다.
2. Targeting Preset의 첫 태스크를 `Kata Select Perceived Actors`로 지정한다. 다음에 `Kata Filter Faction`과 원하는 정렬 태스크를 둔다. 거리 정렬을 사용하면 최초 선택 시 가장 가까운 후보를 선택한다. 다른 월드 검색 Selection을 덧붙이지 않는다.
3. AI Data의 Targeting Preset에 해당 에셋을 지정한다. Target Refresh Interval 기본값은 0.2초이며 감지 성공·실패 이벤트는 즉시 반영한다.
4. StateTree의 Evaluators에 `Kata AI Context`를 추가한다. AIController Context가 트리의 AIController에 연결되는지 확인한다. 상태 조건과 태스크 입력에서 아래 출력을 바인딩한다.

| 출력 | 계약 |
|---|---|
| Target Actor / Has Visible Target | 현재 보이는 적대 대상. 시야 상실·파괴 시 비움 |
| Has Last Known Location / Last Known Location | 마지막 선택 대상의 성공 감지 위치. 시야 상실 후 동결, 대상 파괴 시 무효 |
| Home Location | AI 시작 시 Pawn 위치. 새 빙의 때 다시 기록 |
| Time Since Last Seen | 마지막 관측부터 지난 초. 감지한 적이 없으면 0 |

보이는 현재 대상이 Preset을 통과하면 거리 순위가 바뀌어도 유지한다. 시야 상실이나 필터 제외 시 남은 후보 중 첫 대상을 선택한다. 다른 후보도 없으면 현재 대상을 비우고 마지막 위치를 보존한다. Perception의 감각 기억 시간과 수색 시간은 서로 다르며 수색 시간 정책은 이후 StateTree에서 구성한다.
시각 외 감각은 Listener에 설정할 수 있지만 이번 대상 선택에 사용하지 않는다. 인지·대상 참조는 약한 참조이며 빙의 해제·종료 시 타이머와 이벤트 구독을 정리한다. Evaluator는 대상 선택을 실행하지 않고 결과만 읽는다.

AI-2 빌드·실행·별도 검사는 미실시다. 사용자는 테이블의 에셋 설정 완료를 보고했다. 기존 에셋은 수정하지 않았으므로 빌드 후 Preset 첫 Selection과 Evaluator를 설정해야 한다.

### StateTree 할당·시작 진단 로그

Controller는 LogKataAI에 AIData·트리 경로·AIOwner·컴포넌트 등록 여부를 `AI StateTree assigned`로 기록하고, StartLogic 직후 상태와 IsRunning을 `AI StateTree start result`로 기록한다. `AI StateTree status changed`는 이후 완료·실패·중단도 기록한다. 참조가 비어 있으면 시작을 생략한 이유를 기록한다.
할당과 실행은 별개다. Running=false만으로 실패라고 판단하지 않고 Status의 Succeeded·Failed·Stopped를 구분한다. 에셋·스키마·컨텍스트 오류 상세는 엔진 LogStateTree를 함께 확인한다. Perception은 StateTree 실행 여부와 독립적으로 동작한다.


## Action·Graph 실행 Task

StateTree AI Component 스키마의 상태에 아래 Task 중 하나를 추가한다. Pawn Context는 스키마의 Actor(빙의된 Pawn)에 연결한다. Target Actor 입력은 Kata AI Context의 Target Actor 출력에 바인딩한다. 입력은 진입 시 고정하며 실행 중 재바인딩하지 않는다. 캐릭터에는 초기화된 ASC·KataActionComponent가 필요하고 Graph 실행에는 KataGraphComponent도 필요하다.

| Task | 입력 | 동작 |
|---|---|---|
| Play KataAction | Action | 지정 Action을 한 번 시작하고 종료까지 대기 |
| Play KataGraph | Graph, 선택 Entry Trigger | 콤보 전체 종료까지 대기. 진입 대기일 때만 Trigger를 한 번 보냄 |
| Play KataActionGroup | Group, 선택 Entry Trigger | 그룹에서 한 항목 선택 후 Action 또는 Graph 전체 종료까지 대기 |

Group Task는 유효한 에셋·Payload·양수 Weight를 가진 항목을 고려한다. Action은 CanPlayKataAction을 통과한 항목만 후보로 삼는다. Graph에는 같은 사전 판정 API가 없어 실제 시작 결과를 확인한다. 한 번 추첨하고 실행 실패 시 다른 항목을 연속 재시도하지 않는다. Selection 출력은 원본 EntryIndex와 항목·Payload 사본이다. 그룹 설정은 [Action Group 사용법](Action-Group.md)을 따른다.
Graph는 자동 Entry로 시작한 경우 Entry Trigger를 보내지 않는다. 자동 진입이 없으면 Trigger를 한 번 보내며, 이후에도 WaitingForEntry이거나 어떤 Action도 시작하지 못했으면 실패한다. 이후 콤보는 기존 자동 전이·예약 전이를 따른다. StateTree Task는 진입 이후의 Trigger를 보내지 않는다. 콤보를 이어갈 Trigger는 아래 `Kata Task: AI Send Trigger`를 액션 타임라인에 배치해 보낸다.

### 콤보 이어가기: AI Send Trigger

PC는 Transition Window 안에서 입력으로 Trigger를 보낸다. AI는 같은 자리에 `Kata Task: AI Send Trigger`(Add Task의 AI 카테고리)를 배치한다.
설계와 결정 이유는 [AI 그래프 트리거 발신 계획](../plan/AI-Graph-Trigger-Plan.md)을 따른다.

1. 이어갈 액션의 Transition Window 구간 안쪽 시각에 태스크를 추가한다. 창 시작과 같은 시각에 두어도 된다.
2. Trigger Tag에 다음 노드로 가는 엣지의 Trigger Event Tag를 지정한다.
3. 필요하면 Chance와 Condition을 정한다. 갈래가 여럿이면 갈래마다 태스크를 하나씩 둔다.

| 항목 | 의미 | 기본값·실패 시 동작 |
|---|---|---|
| Trigger Tag | 보낼 Trigger. `Trigger` 루트 아래 태그 | 비우면 설정 오류로 실행에서 제외한다 |
| Condition | 보낼 조건. 대상은 AI 타게팅의 현재 대상이며, 타게팅 컴포넌트가 없으면 액션 Context의 대상이다 | 비우면 통과. 거짓이면 보내지 않으므로 대상 상실·거리 이탈 시 콤보가 끊긴다 |
| Chance | 이 시점에 이어갈 확률(0~1) | 1. 태스크가 시작될 때 한 번만 굴린다 |
| Single Frame | 한 시점에 보낼지 여부 | 기본 켬. 시작한 프레임에 Chance·Condition을 확인해 보낸다. 끄고 구간을 주면 구간 동안 Condition이 참이 될 때까지 기다리며, 거절되면 다시 시도한다 |
| Order Hint | 같은 시각 태스크의 실행 순서 | 기본 1. 같은 시각의 Transition Window(0)가 먼저 열린 뒤 보낸다 |

- 소유 Pawn이 플레이어 조종이면 아무것도 하지 않는다. PC와 AI가 같은 액션을 써도 PC 콤보는 저절로 이어지지 않는다.
- 그래프가 실행 중이 아니면 보내지 않는다. 액션 편집기 프리뷰에서도 아무 일도 하지 않는다.
- 창 밖에서 보낸 Trigger는 그래프가 받지 않는다. 창과 겹치지 않는 배치를 경고하는 데이터 검증은 아직 없다.
- 보낸 기록은 `LogKataAI` Verbose로 남는다.

### 에셋 할당과 최소 실행 확인

1. 변경된 C++를 Editor 대상으로 빌드한 뒤 StateTree 에디터를 다시 연다.
2. 공격 상태에 `Play KataActionGroup` Task를 추가하고 **Group**의 에셋 선택기에서 생성한 Kata Action Group을 지정한다. Group·Action·Graph·Entry Trigger는 직접 설정과 바인딩이 가능한 Parameter다. Target Actor는 Input이므로 `Kata AI Context → Target Actor`에 바인딩한다. Pawn은 Context로 공급받는다.
3. 단일 Action 항목만 있는 그룹부터 사용한다. Graph 항목이 자동 진입하지 않으면 그룹 Task의 Entry Trigger를 그래프 진입 엣지와 같은 Trigger로 설정한다.
4. 정상 완료 전이를 `On State Succeeded`, 실패 전이를 `On State Failed`로 지정한다. 실행 중 공격 상태를 빠져나가는 다른 전이가 없는지 설정을 확인한다.
5. PIE에서 대상 감지 후 공격 상태에 진입시킨다. Action이 끝날 때까지 상태가 유지되고 종료 후 성공 전이로 이동하는지 확인한다. 이후 여러 항목과 Graph 항목을 추가해 선택과 콤보 종료를 확인한다.

기본 출력은 Result와 그룹 Task의 Selection이다. 시작 결과·종료 사유·실행 인스턴스는 Details를 펼쳐 확인한다. 기존 상세 출력 바인딩이 있다면 경로를 `Details.StartResult`, `Details.EndReason`, `Details.ActionInstance`, `Details.GraphInstance` 등으로 다시 연결한다. Result와 Selection의 경로는 유지한다.

Group에 에셋 선택기 없이 IN 표시만 보이면 변경 전 코드가 로드된 상태다. 빌드한 모듈로 에디터를 다시 시작한다. 설정값의 기존 바인딩이 있다면 해제한 뒤 직접 에셋을 지정한다. 실패 시 Result를 먼저 확인한다. NoEligibleEntry는 그룹 후보가 없거나 Action의 실행 조건·쿨다운에 걸린 경우, Busy는 기존 실행이 있는 경우다.
### 출력과 상태 수명

| 출력 | 의미 |
|---|---|
| Result | Running·Completed·Busy·InvalidSetup·NoEligibleEntry·StartRejected·Interrupted |
| Details → Has Start Result / Start Result | 실제 시작 요청 결과의 유효 여부와 값. Graph는 진입 처리 중 마지막 Action 시작 요청 결과 |
| Details → Has End Reason / End Reason | 실제 종료 사유의 유효 여부와 값. 유효 여부 없이 기본 Completed를 성공으로 해석하지 않음 |
| Details → Action Instance / Graph Instance | 이번 Task가 시작한 실행. 상태 이탈 시 참조 정리 |
| Selection | Group Task의 선택 인덱스·항목·Payload. 다음 진입 시 초기화 |

기존 Action이나 Graph가 실행 중이면 Busy로 실패하며 교체하지 않는다. 정상 완료는 Succeeded, 시작 거절·설정 오류·중단은 Failed를 반환한다. 길이 0 Action·순간 콤보는 EnterState에서 즉시 완료 결과를 반환할 수 있다. Graph 내부 Branched로 Action이 교체돼도 Task는 Running을 유지한다.
Task는 완료 판정에 포함된다. 실행 중 상태를 유지하려면 정상 전이를 OnStateCompleted·OnStateSucceeded·OnStateFailed로 구성하고 다른 병렬 Task의 완료 정책도 맞춘다. OnTick·OnEvent·부모 전이는 실행 중에도 이탈할 수 있다. Running은 모든 전이를 잠그지 않는다.
강제 이탈 시 자신의 인스턴스만 Cancelled로 종료한다. OwnerInvalid는 기존 컴포넌트 수명과 함께 처리한다. 다른 주체의 새 인스턴스는 중단하지 않는다. Tick은 상태 조회만 수행하며 실행 갱신은 기존 실행기가 담당한다.
순환 Graph나 무한 Action은 종료·강제 이탈까지 Running이다. 시야 상실 전이·Pressure 소비·예약·해제는 자동 추가하지 않았다.
이번 Task 소스의 빌드·실행은 미확인이다. 사용자는 그룹 에셋 생성 성공을 보고했으며 가중 선택·실행 전체의 확인과는 구분한다.


대기 Task의 실제 선택기 이름은 **Delay Task**다. Idle에서는 Run Forever를 켜고, 재공격 전 대기 상태에서는 Run Forever를 끄고 Duration을 지정한다. 공격 진입 Bool 조건은 Has Visible Target을 Left에 바인딩하고 Right를 true로 지정한다. Idle은 On Tick 조건 전이로 Attack을 선택하고, Attack은 성공·실패 완료 전이를 대기 상태로 연결한다.

## 대상 이벤트와 수색·복귀 설정

대상 이벤트는 모든 AI가 아래 고정 태그를 사용한다. AI Data에서 별도로 지정하지 않는다. 태그는 Config/Tags/Native/StateTree.ini에 정의되어 있다. 이전 AI 이벤트 태그에는 Redirect를 제공하지 않는다. StateTree 전이는 StateTree.Event.AI.* 태그로 지정한다.

| 항목 | 태그 |
|---|---|
| 대상 획득 | StateTree.Event.AI.TargetAcquired |
| 대상 상실 | StateTree.Event.AI.TargetLost |
| 대상 교체 | StateTree.Event.AI.TargetChanged |

이벤트는 Sight 후보가 아닌 최종 적대 대상의 획득·상실·교체 때 한 번 발송한다. 같은 대상의 위치 갱신은 이벤트를 보내지 않는다. 초기 감지 때 트리가 아직 실행되지 않았다면 초기 상태 선택으로 대응한다. 공격 중 상실 이벤트가 지나가도 완료 시 현재 상태를 재판단한다. Evaluator는 엔진 트리 갱신에서 전이 처리 전에 현재 값을 읽는다. TargetRefreshInterval은 기억 위치와 후보 무효화 갱신에 계속 사용한다.

### 상태 구성 순서

1. Root 아래 선택 순서를 Chase → SearchMove → Idle로 둔다. Chase 진입 조건은 Has Visible Target=true, SearchMove는 Has Visible Target=false AND Has Last Known Location=true, Idle은 조건 없는 마지막 기본 상태다. 선택 순서에 의해 처음부터 감지한 대상과 공격 완료 뒤의 대상 상태도 처리한다.
2. Chase에 엔진 MoveTo를 둔다. 대상 Actor는 Kata AI Context.Target Actor에 바인딩한다. 기존 공격 거리 설정을 보존한다. 성공은 Attack, 실패는 Retry로 연결한다. On Event TargetLost는 Root, TargetChanged는 Chase로 연결한다. 같은 상태 재진입은 Reactivate Target State를 Force Changed로 설정해 이동 요청 대상을 다시 반영한다.
3. Attack에 기존 Play KataActionGroup을 유지한다. 성공·실패는 AfterAttack으로 연결한다. Attack에는 상실·교체 이벤트로 나가는 전이를 두지 않는다. Root 등 공통 부모에도 공격을 강제 이탈시키는 이벤트 전이를 두지 않는다.
4. AfterAttack에 Delay Task를 둔다. Run Forever=false, Duration은 AttackInterval 파라미터에 바인딩한다. 완료하면 Root로 전이하며 Root가 재선택되도록 Force Changed를 사용한다.
5. Idle에 Delay Task, Run Forever=true를 설정한다. On Event TargetAcquired → Root로 연결한다. 감지용 On Tick 전이는 제거한다.
6. SearchMove에 엔진 MoveTo를 둔다. Actor 대상은 비우고 Location을 Kata AI Context.Last Known Location에 바인딩한다. 지속 대상 위치 추적은 끄고 진입 시 위치를 사용한다. 성공은 SearchWait, 실패는 Return으로 연결한다. TargetAcquired → Root 이벤트 전이를 둔다.
7. SearchWait에 Delay Task, Run Forever=false, Duration=SearchDuration을 설정한다. 완료는 Return, TargetAcquired 이벤트는 Root로 연결한다. SearchDuration은 마지막 위치 도착 이후의 대기 시간이다.
8. Return은 Root의 기본 선택 자식들 뒤에 배치하고 명시 전이로 진입시킨다. 엔진 MoveTo의 Actor는 비우고 Location=Kata AI Context.Home Location으로 설정한다. 성공은 Returned, 실패는 ReturnRetry, TargetAcquired 이벤트는 Root로 연결한다.
9. Returned에 Clear Kata AI Target Memory Task를 둔다. Pawn은 Context로 공급된다. 같은 상태에 Delay Task(Duration=0.01초)도 추가하고 Tasks 완료 정책을 All로 지정한 뒤 성공 전이를 Root로 연결한다. 이는 즉시 완료 Task 뒤에 이전 Evaluator 출력으로 다시 수색을 선택하지 않도록 다음 트리 갱신을 기다리기 위한 설정이다. 현재 대상이 있으면 기억을 지우지 않아 재감지를 보존한다.
10. Retry와 ReturnRetry에 Delay Task, Duration=MoveRetryInterval을 둔다. Retry 완료는 Root, ReturnRetry 완료는 Return으로 연결한다. 두 상태에도 TargetAcquired → Root 전이를 둔다. 오류 때 프레임마다 재진입하는 루프를 만들지 않는다. 재시도 상한은 아래 「이동 실패와 복귀 불가 처리」를 따른다.

공격 거리·이동 허용 오차는 기존 설정을 유지한다. 튜닝 시작값은 AttackInterval=0.5초, SearchDuration=3초, MoveRetryInterval=1초를 제안한다. StateTree 루트 파라미터로 만들고 AI Data.StateTree 파라미터 오버라이드에서 캐릭터별로 지정할 수 있다. 수치는 동작 확인 후 조절한다.

### 샘플 마스터 트리

`/Game/KataTest/AI/ST_KataAI_Master`는 위 상태 구성과 아래 실패 처리를 미리 배치한 샘플 마스터 트리다. 상태 순서는 Combat → Search(SearchMove·SearchWait·SearchRetry) → Idle → ReturnGroup(Return·Returned·ReturnFailed·ReturnRetry)이다. Combat은 `StateTree.Slot.Combat` 태그의 Linked Asset 상태로 기본 하위 트리 `ST_KataAI_Combat_Melee`(Chase·Attack·AfterAttack·Retry)를, Idle은 `StateTree.Slot.Routine` 태그로 `ST_KataAI_Routine_Idle`을 연결한다. Combat 하위 트리가 Succeeded로 끝나면 Root를 다시 선택하고, Failed로 끝나면 Return으로 간다. 구조는 [AI 계획의 몬스터별 사용 구조](../plan/AI-Plan.md)를 따른다.
마스터 트리 파라미터는 SearchDuration(3)이고, Combat 하위 트리 파라미터는 AttackGroup(Kata Action Group)과 AttackInterval(0.5)이다. 파라미터는 Root 상태가 아니라 트리 최상위 항목의 Parameters에 둔다. Root 상태 파라미터는 AI Data 오버라이드 목록에 나타나지 않는다. 재시도·추격 한계 값은 아래 Movement Failure 필드에서 온다.
몬스터별 값은 AI Data에서 지정한다. 마스터 파라미터는 **Kata|AI → State Tree** 아래 Parameters, Combat 하위 트리의 AttackGroup은 **Kata|AI → Linked StateTree Slots → 항목 → State Tree** 아래 Parameters에서 체크박스를 켜고 지정한다. 체크한 항목은 이후 기본값 변경을 따르지 않으므로 몬스터별로 다른 값만 체크한다. AttackGroup이 비어 있으면 Attack이 InvalidSetup으로 실패한다.
노드를 새로 추가하면 바인딩이 필요한 Input(Bool·Float Compare의 Left, Distance Compare의 Source)이 비어 있을 때 컴파일에 실패한다. Play KataAction·Play KataGraph·Play KataActionGroup과 재시도 상한 조건은 설명문에 바인딩된 원본 이름(예: `Parameters.Attack Group`)이나 상수 값을 표시하므로 연결 여부를 트리 화면에서 확인할 수 있다.

### 이동 실패와 복귀 불가 처리

재시도 상한·재시도 간격·추격 한계는 몬스터별로 AI Data의 **Kata|AI|Movement Failure** 필드에서 정한다. 마스터 트리와 슬롯 하위 트리가 같은 값을 쓰도록 StateTree 파라미터가 아니라 `Kata AI Context` 출력으로 제공한다. 트리에서는 Delay Duration, 재시도 상한 조건, Distance Compare를 이 출력에 바인딩한다.

| AI Data 필드 / Evaluator 출력 | 의미 |
|---|---|
| Max Move Retries | 연속 이동 실패 후 허용할 재시도 횟수. 기본 3, 0이면 무제한 |
| Move Retry Interval | 재시도 사이 대기. 기본 1초. 0 이하는 데이터 검증 오류이며 Evaluator는 1초로 대신한다 |
| Leash Distance | Home에서 허용할 추격 거리. 기본 0, 0이면 무제한 |

| 제공 요소 | 종류 | 동작 |
|---|---|---|
| Update Kata AI Move Retry | Task | Operation=Increment면 Pawn의 재시도 횟수를 하나 늘리고, Reset이면 0으로 만든다. Output RetryCount |
| Kata AI Move Retry Limit Reached | 조건 | 현재 횟수가 MaxMoveRetries 이상이면 true. 횟수를 바꾸지 않는다. MaxMoveRetries=0이면 항상 false |
| Resolve Kata AI Return Failure | Task | 이동을 멈춘 뒤 Policy=Stay면 현재 위치를, Teleport면 순간이동한 위치를 새 Home으로 기록하고 기억·재시도 횟수를 지운다. 즉시 성공 |

재시도 횟수는 Pawn의 `UKataAITargetingComponent`가 보관한다. 대상 획득, 기억 정리, 복귀 불가 처리, 재빙의 때 0으로 돌아간다.

1. Chase·SearchMove의 실패 전이를 두 개로 나눈다. 먼저 `Kata AI Move Retry Limit Reached`(MaxMoveRetries 바인딩) 조건이 있는 전이를 Return으로, 그다음 조건 없는 전이를 Retry로 둔다.
2. Return의 실패 전이도 같은 순서로 나눈다. 조건 전이는 ReturnFailed, 나머지는 ReturnRetry로 연결한다.
3. Retry와 ReturnRetry에 `Update Kata AI Move Retry`(Operation=Increment)를 Delay Task와 함께 두고 Tasks 완료 정책을 All로 지정한다.
4. ReturnFailed에 `Resolve Kata AI Return Failure`와 Delay Task(Duration=0.01초)를 두고 완료 정책을 All, 성공 전이를 Root로 연결한다. Returned 상태와 같은 이유로 다음 트리 갱신을 기다린다. Teleport 목적지는 기본이 Home이며 bUseReturnLocation을 켜면 ReturnLocation에 바인딩한 위치를 쓴다. 순간이동이 충돌로 실패하면 경고 로그를 남기고 Stay로 처리하며 Output AppliedPolicy에 실제 결과가 남는다.
5. 추격 한계는 코드 없이 구성한다. Chase에 엔진 `Distance Compare` 조건(Source=속성 함수 `Get Actor Location`으로 스키마 Actor의 위치, Target=Kata AI Context.Home Location, 비교값=LeashDistance)을 가진 On Tick 전이를 Return으로 둔다. LeashDistance=0을 무제한으로 쓰려면 같은 전이에 `LeashDistance > 0` Float Compare 조건을 AND로 추가한다.
6. 복귀 중 재감지는 기존 TargetAcquired → Root 전이를 유지한다. 추격 한계 경계에서 추적과 복귀가 왕복하지 않도록, LeashDistance를 쓰는 몬스터는 이 전이에 `Get Actor Location`(Kata AI Context.Target Actor)과 Home Location의 Distance Compare(한계 이내) 조건을 추가한다.
7. Attack 상태에는 추격 한계 전이를 두지 않는다. 진행 중 공격은 완료 후 Root 재선택으로 판단한다.

### Linked 슬롯 교체

마스터 트리의 Linked Asset 상태에 `StateTree.Slot.*` Tag를 지정하면, AI Data의 **Linked StateTree Slots**에 같은 태그 항목을 추가해 몬스터별 하위 트리로 교체할 수 있다. Controller는 트리 시작 전에 목록을 적용하고 재빙의 때 다시 적용한다. 항목이 없으면 Linked 상태에 지정된 기본 하위 트리를 쓴다.

- 엔진은 태그가 있는 Linked 상태의 Parameters에 바인딩이 하나라도 있으면 교체를 막는다. 슬롯 하위 트리에 필요한 값은 Linked 상태의 상수 파라미터(기본값)나 AI Data 슬롯 항목의 파라미터 오버라이드(몬스터별 값)로 넘긴다.
- 하위 트리는 마스터와 같은 StateTree AI Component 스키마여야 한다. 태그·트리·스키마가 맞지 않는 항목은 데이터 검증 오류이며 런타임에는 경고 후 제외한다.
- 하위 트리는 마스터 상태로 직접 전이할 수 없다. 자체 `Kata AI Context` Evaluator를 두고, 마스터에서 이어갈 행동은 하위 트리를 Succeeded·Failed로 끝내 Linked 상태의 전이로 구분한다.

### 실행 확인 가이드

- PIE 시작 전에 이미 시야 안에 있어도 추적하는지 확인한다. 계속 보이는 동안 공격 완료 후 다시 공격해야 한다.
- Chase 중 벽 뒤로 숨으면 마지막 감지 위치로 이동해야 한다. 숨은 플레이어의 새 위치를 따라가면 안 된다.
- 마지막 위치 도착 후 SearchDuration을 기다리고 HomeLocation으로 복귀해야 한다. 복귀 후 다시 수색하지 않고 Idle을 유지해야 한다.
- SearchMove·SearchWait·Return 중 다시 보이면 추적을 재개해야 한다.
- Attack 중 숨거나 대상이 교체되어도 현재 공격은 완료하고 이후 상태를 판단해야 한다.
- 대상 파괴·이동 불가·재빙의에서 이벤트 누락·중복과 즉시 재시도 루프가 없는지 확인한다.
- MaxMoveRetries=2에서 도달할 수 없는 위치로 이동시키면 두 번 재시도한 뒤 포기해야 한다. 0이면 MoveRetryInterval 간격으로 계속 재시도해야 한다.
- Home에 도달할 수 없을 때 Stay는 그 자리를 새 Home으로 삼고 다시 수색하지 않아야 한다. Teleport는 Home으로 순간이동해야 하며, 막힌 위치면 경고 후 Stay로 처리돼야 한다.
- LeashDistance를 넘으면 복귀해야 하고, 경계에 대상이 서 있어도 추적과 복귀를 반복하지 않아야 한다.

2026-10-08 사용자가 Editor 빌드 후 `ST_KataAI_Master`와 `DA_AI_StarvedHound`로 추적·공격·공격 보호·수색·재감지·복귀·재시도 상한·추격 한계를 PIE에서 확인했다. 복귀 불가 처리(Stay·Teleport)와 순간이동 실패 경로는 이번 확인 항목에 포함되지 않았다.
같은 날 Linked 슬롯 구성(`ST_KataAI_Combat_Melee`·`ST_KataAI_Routine_Idle`, DA의 Combat 슬롯 항목)으로 바꾼 뒤 사용자가 PIE를 다시 확인했으며 슬롯 무시·오버라이드 오류 로그는 없었다. 복귀 불가 상황에서 제자리에 머문 동작이 Stay 처리인지, Return Move To의 Allow Partial Path로 부분 경로가 성공 처리된 것인지는 구분하지 않았다.