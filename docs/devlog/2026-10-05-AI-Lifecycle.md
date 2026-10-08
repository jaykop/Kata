# KataAI 기반과 StateTree 실행 수명

작성: 2026-10-05  
갱신: 2026-10-05  
유형: 구현 기록  
대상: KataAI·KataFramework AI-1  
기준: #22 미커밋 구현

## 배경과 결론

사용자가 #22 계획 중 AI-1부터 구현하도록 승인했다. 감지·공격보다 먼저 플러그인·Controller·AI 캐릭터와 NPC 데이터 설정, 트리 시작·정지 수명을 연결했다.

## 변경 내용과 이유

- KataAI Runtime 모듈과 AKataAIController를 추가했다. 엔진 UStateTreeAIComponent를 재사용하고 별도 StateTree 스키마를 만들지 않았다.
- Framework의 AKataAICharacter가 기본 Controller와 StateTree를 지정한다. Controller는 IKataAIPawnInterface를 통해 설정·준비 여부를 읽어 Framework에 역의존하지 않는다.
- FKataNPCCharacterRow에 선택 AI Controller Class·State Tree 소프트 참조와 로드 수집을 추가했다. AI 항목을 지정한 행만 AI 캐릭터를 요구해 기존 NPC를 강제로 변경하지 않는다.
- 자동 트리 시작을 끄고 Controller·Pawn의 BeginPlay 준비와 ASC Avatar 초기화 후 한 번 시작한다. BeginPlay 콜백 중 HasActorBegunPlay가 아직 false인 엔진 수명 때문에 명시적인 준비 플래그를 사용한다. 배치·런타임 스폰의 호출 순서 차이를 Controller BeginPlay·OnPossess와 Pawn BeginPlay에서 같은 시작 함수로 모은다.
- UnPossess 전에 트리·이동·Gameplay Focus를 정리하고 이전 트리 참조를 비운다. 캐릭터 종료에서도 컴포넌트 파괴 전 빙의를 해제한다.
- 팀 번호는 Pawn의 기존 타게팅 팩션에서 조회한다. AI 전용 타게팅 파생과 감지 연결은 AI-2에서 추가하므로 현재 AI 캐릭터는 기존 기반 타게팅을 사용한다.

## 근거

- [AI Controller](../../Plugins/KataAI/Source/KataAI/Private/Controller/KataAIController.cpp): 시작·정지 조건.
- [AI 캐릭터](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataAICharacter.cpp): Pawn 준비·행 적용.
- [#22](https://github.com/jaykop/Kata/issues/22), [계획](../plan/AI-Plan.md): 첫 단계와 후속 범위.

## 확인 범위와 결과

소스 구현과 문서 반영만 수행했다. 빌드·테스트·별도 검사·사용자 실행 확인은 미실시다. 샘플 uasset·umap은 변경하지 않았다. 기존 반영 타입·에셋 경로를 변경하지 않아 Redirect를 추가하지 않았다.

## 남은 제한과 연관 문서 반영

### 후속 결정: 공유 AI Data

사용자가 AI 설정 에셋 추가를 승인했다. UKataAIData에 FStateTreeReference(트리·파라미터 오버라이드), Instanced Sense Config 목록, 주 감각, Targeting Preset을 모았다. NPC 행은 AI Data를 소프트 참조하고 에셋 내부의 트리·Preset은 하드 참조로 함께 로드한다. Character는 로드된 AI Data 참조만 보관한다.
Controller는 트리 참조를 복사해 파라미터를 동기화하고, Sense Config를 새 Perception 컴포넌트 아래 복제한 뒤 등록한다. 재빙의·종료에는 해당 Listener와 기억을 제거한다. 공유 에셋에 실행 상태를 저장하지 않는다. 주 감각 미등록·중복 감각은 데이터 검증에서 진단하며, 런타임은 경고와 대체·제외로 처리한다.
이전 NPC 행 StateTree는 비편집 직렬화 필드로 보존하고 AI Data가 없는 기존 트리 설정은 생성 실패 로그로 이관을 안내한다. 서로 다른 에셋 타입을 Property Redirect로 바꾸지 않는다. 사용자가 새 AI Data에 이전 트리를 지정해야 한다.
Perception 설정 적용까지 이번 요청에 포함했으며 감지 후보 선택·어그로·Pressure는 추가하지 않았다. 이번 변경의 빌드·실행·별도 검사는 미실시다.

### 후속 결정: StateTree는 NPC 행에서 지정

사용자와 NPC·Creature 구분을 논의한 뒤, 기존 NPC Tables에 중립·대화·적 등 여러 테이블을 등록하는 구조를 유지하고 StateTree의 설정 진입점을 NPC 행으로 모았다. 테이블 분류는 콘텐츠 정리이며 별도의 전투 가능 타입을 뜻하지 않는다. 공격·적대화 정책은 이 작업에서 추가하지 않았다.

AKataAICharacter의 StateTree를 EditAnywhere에서 Transient 보관 참조로 변경했다. ApplyCharacterRow는 빈 StateTree도 반영해 이전 설정이 남지 않게 한다. Controller가 트리 실행 상태를 소유하는 수명은 유지한다.
이전에 캐릭터 BP에 지정한 트리는 NPC 행으로 옮겨야 한다. 직접 배치한 캐릭터에는 NPC 행이 적용되지 않으므로 행 기반 스포너·생성 경로를 사용한다. 기존 StateTree 프로퍼티 이름과 타입은 유지했으며 Redirect는 추가하지 않았다.

사용자가 AI-1 최초 소스의 Editor 빌드 성공을 보고했다. 테이블 전용 설정으로 바꾼 후의 빌드·실행은 미확인이다. 에이전트 빌드·테스트·별도 검사는 실행하지 않았다.

Perception·AI 타게팅·공격 Task·Debugger·추적·수색·복귀는 다음 단계다. 어그로·Pressure는 별도 후속 설계다.
[AI manual](../manual/AI.md)·Character Data·README·AGENTS와 플러그인 계획에 현재 추가 구성과 사용 계약을 반영했다. #22 결과 댓글·라벨 변경은 초안 승인 후 게시한다.

### 후속 구현: 시각 타게팅과 Evaluator

사용자의 테이블 에셋 설정 완료 보고와 작업 진행 요청에 따라 AI-2를 구현했다. Framework AI 캐릭터의 기본 타게팅을 UKataAITargetingComponent로 교체하고 Controller의 Sight 이벤트를 전달한다. 보이는 액터만 Preset 후보로 수집하는 Kata Select Perceived Actors를 추가했다. 현재 적대 대상이 Preset을 통과하면 유지하며, 상실·제외 시 정렬된 후보 중 첫 대상을 선택한다.

Target Refresh Interval 기본값은 0.2초다. 성공 감지와 보이는 대상의 주기적 위치 관측으로 마지막 위치를 갱신하고 시야 상실 후에는 실제 위치를 읽지 않는다. 대상 파괴 시 약한 참조가 무효화된다. 새 빙의 때 귀환 위치를 기록하고 해제·종료 시 타이머와 이벤트 구독을 정리한다.

Kata AI Context는 엔진 공용 Evaluator 기반을 사용해 기존 AI Component 스키마에서 대상·가시성·마지막 위치 유효 여부·위치·귀환 위치·관측 경과 시간을 제공한다. Evaluator가 매 프레임 Preset을 실행하지 않도록 인지 갱신과 출력 조회를 분리했다. 공개 상속 타입에 필요한 KataTargeting·TargetingSystem 의존은 Public으로 옮겼다.

[AI manual](../manual/AI.md)에 Preset 구성·출력 바인딩·수명 계약을 반영했고 plan의 파일명을 실제 식별자에 맞췄다. 기존 uasset은 수정하지 않았다. AI-2 빌드·테스트·별도 검사·실행 확인은 미실시다. 이동·공격·수색·복귀 행동은 AI-3 이후이며 #22 게시·커밋은 수행하지 않았다.

### 후속 진단: StateTree 참조와 실행 여부 구분

사용자의 코드 진단 요청에 따라 할당 경로와 UE 5.8 SetStateTreeReference·StartLogic 구현을 읽고, 기존 PIE의 Controller 컴포넌트를 MCP로 조회했다. 해당 컴포넌트의 StateTreeRef에 ST_StarvedHound가 지정되어 있어 실제 할당을 확인했다. 트리의 루트·직접 자식 상태 Task와 자식 전이, Global Task 목록은 비어 있었으며 Kata AI Context Evaluator는 존재했다. 이 조회로 현재 실행 상태까지 확인한 것은 아니다.
기존 Controller는 AI Data·트리 참조가 비거나 트리가 즉시 완료된 경우를 자체 로그에서 구분하지 못했다. 할당·시작 직후 상태·상태 변경·빈 설정에 대한 진단을 추가하고 AI manual에 로그 계약을 반영했다. 에셋과 행동 정책은 변경하지 않았다. 새 진단 코드의 빌드·실행은 미실시다.

### 후속 설정: 팩션 태그와 선택기 제한

Perception 미감지 진단 뒤 사용자가 없는 Faction 태그 추가와 태그 선택기 제한을 요청했다. 프로젝트 Config/Tags/Faction.ini에 Faction 루트와 Player·Monster·Neutral 샘플 태그를 추가했다. 플러그인은 태그를 정의하지 않고 Targeting 컴포넌트·프로젝트 팩션 목록·관계표의 태그 필드와 Blueprint 팩션 함수 입력에 Categories=Faction 메타만 제공한다.
Gameplay-Tags와 Factions manual에 루트·샘플 태그·선택기 계약을 반영했다. 캐릭터별 팩션 지정과 적대 관계는 자동 변경하지 않았다. 빌드·테스트·UI 확인은 미실시다.

사용자 후속 요청으로 샘플 Faction.Monster를 Faction.Enemy로 변경했다. 현재 manual의 예시도 변경했으며 이전 태그를 저장한 에셋을 위해 GameplayTagRedirects를 추가했다. 빌드·UI 확인은 미실시다.

### 후속 구현: Action·Graph·Group 실행 Task

사용자가 그룹 에셋 생성 성공을 보고한 뒤 Task 구현을 요청했다. Play KataAction·Play KataGraph·Play KataActionGroup을 KataAI의 공용 StateTree Task 기반으로 추가했다. Pawn Context와 진입 대상은 실행 중 재복사하지 않고 진입에서 한 번 실행한다. 실행 중 Running, 정상 완료 Succeeded, 거절·설정 오류·중단 Failed와 상세 출력을 반환한다. 완료 판정에서 제외할 수 없도록 Task 설정을 고정했으며 별도의 StateTree 전이까지 차단하지는 않는다.

기존 Action·Graph가 실행 중이면 Busy로 거절한다. Tick은 자신이 시작한 실행의 상태만 조회한다. ExitState는 해당 인스턴스에만 취소를 요청하고 다른 실행 주체의 현재 인스턴스를 StopKata·StopGraph로 중단하지 않는다. 상태 기록의 원시 주소를 종료 델리게이트에 캡처하지 않아 추가 구독이 필요 없다.

Group Task는 활성 에셋·Payload·Weight를 확인하고 Action은 CanPlayKataAction 결과로 후보를 제한한다. Graph는 같은 사전 API가 없어 실제 시작 결과를 확인한다. 한 번 추첨한 Selection과 Payload 사본을 출력하며 실패 후 다른 후보를 연속 재시도하지 않는다. Pressure 의미는 해석하지 않는다.

Graph에 저장된 종료 사유와 마지막 Action 시작 결과·시작 경험 여부 조회를 추가했다. StartGraph의 초기화 성공과 진입 Action 실행을 구분하고, 선택 EntryTrigger를 진입 대기일 때 한 번 전달한다. 계속 진입 대기이거나 Action을 하나도 시작하지 못했으면 실패해 무한 대기를 피한다. 콤보 내부 Action 전이에서는 Task를 종료하지 않는다. 기존 Graph 전이·거절 정책은 변경하지 않았다.

KataAI 공개 실행 타입에 맞춰 KataRuntime·KataGraph·GameplayTags 의존과 코어 Kata 플러그인 직접 의존을 명시했다. AI·Runtime-Usage·Action-Group manual 및 AGENTS에 계약을 반영했다. 소스 구현만 완료했으며 빌드·테스트·별도 검사·UI/실행 확인은 미실시다. 샘플 StateTree·Action·Graph·그룹 에셋은 수정하지 않았다. 추적·수색·복귀 상태와 샘플 연결은 별도 구성해야 한다. #22 공개 게시·커밋은 수행하지 않았다.

## StateTree Task 설정 UI 수정 (2026-10-05)

Group을 Input으로 선언해 상수 에셋을 직접 할당할 수 없었던 문제를 수정했다. StateTree Input은 바인딩 입력으로 취급되므로 Action·Graph·Group·EntryTrigger를 Parameter로 변경했다. TargetActor는 Input, Pawn은 Context를 유지한다.

Result와 Selection 외의 여섯 출력은 FKataStateTreeExecutionDetails로 묶었다. 상세 출력은 Details 아래로 이동하며 기존 상세 바인딩은 다시 연결해야 한다. 일반 AdvancedDisplay 메타에만 의존하지 않고 중첩 구조로 표시를 정리했다. 설정과 실행 확인 절차는 manual/AI.md에 반영했다. 이 변경의 빌드와 실행 UI는 사용자 확인 전이다.

## 대상 변화 StateTree 이벤트와 기억 정리 (2026-10-05)

최종 대상 선택 변경을 native 델리게이트로 알리고 Controller가 AI Data의 선택 이벤트 태그를 StateTree에 전달한다. 위치 갱신에는 반복 발송하지 않으며 파괴된 weak 대상의 상실도 직전 선택 여부로 판별한다. 초기 대상은 TreeStart 선택으로 처리한다. 해제 때 구독을 먼저 제거하여 정리 중 전투 이벤트를 억제한다. Clear Kata AI Target Memory Task는 복귀 완료 후 기억을 정리하며 현재 대상은 보존한다. 이동·대기는 엔진 MoveTo와 Delay Task를 재사용한다. 프로젝트 이벤트 태그와 설정·실행 가이드를 추가했다. 샘플 에셋 변경·빌드·PIE는 미실시다.


## AI 이벤트 태그 통합

사용자 결정에 따라 AI Data의 이벤트 필드 세 개를 제거하고 Event.AI.* 고정 태그를 사용한다. Event.ini를 Native 경로로 이동하며 기존 Hit·Camera 태그를 보존했다. 별도 AI.ini는 제거하고 이전 AI.Event.* 이름에 Redirect를 추가했다. 생성된 KataTag는 ProjectKata 모듈 소유이므로 KataAI에서 직접 참조하지 않고 등록된 고정 이름을 조회한다. 누락 태그는 시작 경고로 진단한다. 태그 코드 생성은 수행했으며 빌드·PIE는 미실시다.


## StateTree 이벤트 분리와 Test 태그 제거

사용자 요청으로 StateTree.Event.AI.* 및 StateTree.Event.Camera.Reselect를 Native/StateTree.ini에 분리했다. Event.ini에는 GAS용 Event.Hit를 보존했다. 사용처 없는 AI.Event Redirect를 제거하고 Event.AI Redirect도 추가하지 않는다. 조건 테스트의 Kata.Tests.Condition.* 네이티브 등록 세 개를 제거하고 기존 Status.LockOn 태그 조회로 치환했다. 테스트 추가·실행은 하지 않았다. 기존 카메라 Watcher와 StateTree 전이의 태그는 새 값으로 직접 설정해야 한다. 태그 생성은 수행하되 빌드·PIE는 사용자 확인 전이다.

## AI 계획 정리 (2026-10-07)

AI-Plan에 단계별 제안이 누적되면서 구현 완료 규칙과 대체된 제안(가중 전용 Task, AI Data 이벤트 태그, Tick 감지 전이)이 섞여 있었다. 사용자 요청으로 구현된 사용 규칙은 AI manual과 이 기록을 기준으로 두고, plan에는 역할 경계·트리 조립·행동 배치·실패 정책·Pressure의 미확정 설계만 남겼다. 2026-10-06의 코드·행동 흐름 제안은 plan의 해당 절로 통합했고, 이전 판 원문은 Git 기록으로 추적한다. 소스·에셋 변경과 빌드는 없다.

## 이동 실패·복귀 불가 처리 (2026-10-07)

사용자 결정에 따라 실패 처리는 시스템이 설정 수단과 안전 보장을 제공하고 값은 몬스터별 StateTree 파라미터로 정한다. 재시도 횟수는 Home·기억과 초기화 시점을 맞추기 위해 `UKataAITargetingComponent`에 두었다. 조건 평가에 부작용을 두지 않는 원칙에 따라 횟수 증가(`Update Kata AI Move Retry`)와 상한 판정(`Kata AI Move Retry Limit Reached`)을 나눴다. 0은 무제한이다.
복귀 불가 시 `Resolve Kata AI Return Failure`가 이동을 멈추고 Stay(현재 위치를 새 Home) 또는 Teleport(복귀 지점 순간이동, 충돌 실패 시 Stay)로 처리한다. 두 경우 모두 실제 위치를 Home으로 기록해 다음 교전 뒤 같은 실패를 반복하지 않게 했다. 추격 한계와 경계 왕복 방지는 엔진 Distance Compare 조건으로 구성할 수 있어 코드를 추가하지 않았다. MoveRetryInterval은 엔진 Delay Task 값이라 코드 검증 대신 manual에 0보다 커야 한다고 명시했다. 소스 구현만 했으며 빌드·PIE와 샘플 에셋 구성은 미실시다.

## StateTree 노드 설명문의 바인딩 표시 (2026-10-08)

MCP와 에디터에서 Play KataAction·Play KataGraph·Play KataActionGroup의 에셋 입력과 재시도 상한 조건의 MaxMoveRetries가 파라미터에 연결됐는지 확인할 수 없었다. 엔진 Delay Task처럼 `GetDescription`을 구현해 바인딩된 입력은 원본 이름(예: `Parameters.AttackGroup`), 아니면 상수 값을 설명문에 표시한다. 공용 처리는 Private `KataStateTreeExecution::DescribeInput`·`DescribeAsset`에 두고 `WITH_EDITOR`로 제한해 실행 동작에는 영향이 없다. 빌드 전이다.

## Linked 슬롯과 실패 처리 값의 AI Data 이동 (2026-10-08)

UE 5.8 StateTree 컴파일러는 태그가 있는 Linked Asset 상태의 Parameters에 바인딩이 있으면 런타임 교체를 막는다(`bCanOverrideLinkedAssetAtRuntime`). 따라서 몬스터별로 교체하는 슬롯 하위 트리는 마스터 트리 파라미터를 받을 수 없다. 사용자가 안 2를 선택해, 마스터와 슬롯이 함께 쓰는 실패 처리 값(MaxMoveRetries, MoveRetryInterval, LeashDistance)을 `UKataAIData` 필드로 옮기고 `Kata AI Context` 출력으로 제공했다. 2026-10-05의 "행동 수치를 AI Data에 중복하지 않는다" 결정은 이 세 값에 한해 바뀐다. 대신 값의 중복이 없어지고 MoveRetryInterval > 0을 데이터 검증할 수 있다.
`UKataAIData`에 `StateTree.Slot` 태그와 하위 트리를 짝짓는 `LinkedStateTreeSlots`를 추가했다. Controller는 트리 시작 전에 `SetLinkedStateTreeOverrides`로 적용하며, 엔진이 무효 항목 하나로 목록 전체를 무시하므로 태그·트리·스키마가 맞지 않는 항목은 미리 제외한다. 소스 구현만 했으며 빌드·PIE와 샘플 에셋 분리는 이후 단계다.

## GameplayDebugger 카테고리 (2026-10-09)

AI-6으로 KataAI 모듈에 GameplayDebugger "KataAI" 카테고리를 추가했다. KataCamera와 같이 `SetupGameplayDebuggerSupport`와 모듈 시작·종료 시 등록·해제를 사용하고, 구현 전체를 `WITH_GAMEPLAY_DEBUGGER`로 묶었다. 디버그 대상 Pawn의 StateTree 활성 상태(슬롯 하위 트리 포함), 슬롯, 대상·기억·Home·추격 한계, 재시도 횟수, 실행 중 Action·Graph를 기존 조회 함수로 읽어 표시하며 새 공개 API는 추가하지 않았다. Play Task의 Result는 StateTree 인스턴스 데이터라 표시 범위에서 제외했다. 소스 구현만 했으며 빌드·PIE 확인 전이다.

## 슬롯 하위 트리 재진입 시 오버라이드 파라미터 누락 (2026-10-09)

AI-6 Debugger로 몬스터가 첫 공격 뒤 멈추는 현상을 진단했다. Combat 하위 트리가 공격마다 Succeeded로 끝나고 마스터 Root를 거쳐 같은 Combat 슬롯에 즉시 재진입하는 구조였다. UE 5.8은 같은 선택 과정에서 Linked 상태를 재진입할 때 이전 파라미터 인스턴스가 활성이면 슬롯 오버라이드 파라미터를 적용하지 않는다(`StateTreeExecutionContext.cpp` 상태 선택의 파라미터 생성부). 그래서 두 번째 진입부터 AttackGroup이 비어 `Play KataActionGroup`이 즉시 실패했다. 같은 재진입 경로에서 Scheduled Tick이 켜져 있으면 트리가 깨어나지 않고 멈추는 현상도 함께 관찰됐다.
샘플 `ST_KataAI_Combat_Melee`의 AfterAttack·Retry에 "대상이 보이면 Chase" 전이를 먼저 두어 공격 사이에는 하위 트리 안에서 반복하고, 대상이 없을 때만 Succeeded로 끝나게 바꿨다. 진단을 위해 그룹 Task의 실패 사유와 항목별 실행 가능 판정, Action 시작 거절을 `LogKataAI` Verbose 로그로 남기도록 했다. 2026-10-09 사용자 PIE(Scheduled Tick 기본값)에서 연속 공격, 대상 상실 후 수색·복귀, 재교전과 Routine 슬롯 진입을 로그로 확인했으며 실패 로그는 없었다.
