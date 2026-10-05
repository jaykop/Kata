# KataAI 구현 계획

작성: 2026-10-05  
갱신: 2026-10-05  
연결 이슈: [#22 KataAI](https://github.com/jaykop/Kata/issues/22)  
현재 상태 근거: 연결 이슈, [타게팅 사용법](../manual/Targeting.md), 기존 KataFramework 캐릭터·생성 경로  
대체 관계: 없음

## 목적과 현재 구조

몬스터가 적대 대상을 시각으로 감지하고 추적해 KataAction을 실행하도록 한다. 기존 구조는 공용 타게팅 기반과 팩션, 액션 실행 컴포넌트, Framework 캐릭터·NPC 행·비동기 생성 경로를 제공한다. AI 역할별 조합은 이 계획으로 추가한다.

## 범위

- 첫 구현: KataAI Runtime 플러그인, AIController, Perception 시각 연결, AI 타게팅 파생 컴포넌트와 Preset 후보 수집, StateTree에서 단일·가중 랜덤 KataAction과 콤보 KataGraph 실행, Framework AI 캐릭터·NPC 행 연결, GameplayDebugger, 샘플 설정 안내.
- 첫 행동 흐름: 대기 → 추적 → 공격 → 재판단. 시야 상실 시 마지막 감지 위치 이동 → 일정 시간 수색 → 시작 위치 복귀.
- 후속: 청각·피해 감각, 대상별 어그로, 동료 인지 전파, 액션 소음, Pressure·Capability.
- 별도 작업: 피격·사망·스탯·GA 지급, 네트워크, BT 어댑터. 콤보는 기존 KataGraph를 사용하고 별도 콤보 실행기를 만들지 않는다.

## 확정 사항과 제안

| 항목 | 구분 | 내용과 근거 |
|---|---|---|
| StateTree 기반 | 확정 | #22와 AGENTS. BT 의존·어댑터 없음 |
| 첫 구현 범위 | 확정 | 2026-10-05 사용자: 시각 감지·추적·공격·복귀부터 |
| 시야 상실 | 확정 | 2026-10-05 사용자: 마지막 감지 위치로 이동하고 일정 시간 뒤 복귀 |
| 타게팅 계층 | 확정 | UKataTargetingComponent의 AI 파생은 KataAI, 조합 캐릭터는 KataFramework |
| 팩션·어그로 | 확정 | PC만 대상으로 가정하지 않음. 어그로는 후속 단계에서 몬스터·대상 쌍별 관리 |
| Pressure 수명 | 확정 | 후속 단계의 순수 StateTree 조건이 가능 여부 확인, 실행 Task가 예약하고 액션 종료 시 자동 해제 |
| 트리 실행 기반 | 제안 | 엔진 UStateTreeAIComponent와 UStateTreeAIComponentSchema 재사용. Controller 클래스 컨텍스트를 AKataAIController로 설정. 별도 스키마·카메라 실행 코드 복사 없음 |
| 상태 정책 소유자 | 제안 | StateTree가 이동·수색·공격·복귀를 결정. Controller는 인지 전달과 실행 수명만 관리 |
| 첫 타겟 선택 | 제안 | 보이는 현재 적대 대상 유지, 잃거나 무효하면 Preset의 가장 우선하는 보이는 후보로 교체. 기본 샘플은 거리순. 어그로는 미포함 |
| 공격 중 타겟 변경 | 제안 | 액션 시작 시 대상을 스냅샷으로 고정. 시야 상실만으로 진행 중 공격을 끊지 않고 끝난 뒤 재판단. 대상 파괴·트리 이탈은 취소 |
| NPC 호환 | 제안 | 기존 FKataNPCCharacterRow·AKataCharacter 샘플 유지. AI 설정을 지정한 행만 AKataAICharacter 계열 요구 |
| 샘플 | 제안 | StarvedHound를 첫 대상으로 사용. 애니메이션·공격 에셋·NavMesh 준비 여부는 구현 후 사용자가 확인 |

## 모듈 경계

KataAI는 KataRuntime·KataTargeting과 엔진 AIModule·StateTreeModule·GameplayStateTreeModule·TargetingSystem을 사용한다. KataFramework·KataCamera에 의존하지 않는다. KataFramework가 KataAI를 참조해 AI 캐릭터와 데이터 행을 조합한다.

[플러그인 계획](Plugin-Modularization-Plan.md)은 KataAI → KataTargeting을 명시한다. AGENTS의 포괄적인 위성 → 코어 문구는 이 관계가 분명하도록 구현 승인 시 함께 정리하는 안이다. 코어 역참조·순환 의존은 허용하지 않는다.
FKataNPCCharacterRow의 기존 주석인 “AI 항목은 KataAI가 더한다”는 Framework 기반 타입의 역참조를 유발하므로, Framework에서 AI 설정을 조합한다는 계약으로 고친다.

공개 상속·구조체 필드에 필요한 모듈만 Public에 둔다. Perception 구현과 Debugger 등 구현 전용 의존은 Private에 둔다. 새 Editor 모듈은 첫 범위에서 만들지 않는다.

## 첫 구현의 파일별 계획

| 신규 경로 | 역할 |
|---|---|
| Plugins/KataAI/KataAI.uplugin | Runtime 플러그인, 필요한 엔진·Kata 플러그인 활성화 |
| Source/KataAI/KataAI.Build.cs | 모듈 의존과 GameplayDebugger 지원 |
| Public/KataAIModule.h, Private/KataAIModule.cpp, Private/KataAILog.h | 모듈·진단·Debugger 등록 |
| Public/KataAIPawnInterface.h | Framework 역참조 없이 Pawn의 StateTree·게임 준비 여부를 읽는 C++ 계약 |
| Public·Private/Data/KataAIData.h/.cpp | 공유 StateTree 참조·파라미터·Sense 원본·주 감각·Targeting Preset과 에디터 데이터 검증 |
| Public·Private/Controller/KataAIController.h/.cpp | AAIController 파생, Perception·StateTree 소유, 빙의·해제, Pawn의 팩션 팀 조회 |
| Public·Private/Targeting/KataAITargetingComponent.h/.cpp | 인지 기록·현재 대상·마지막 위치·귀환 위치, 기존 타게팅 가상 함수 구현 |
| Public·Private/Targeting/Tasks/KataTargetingTask_SelectPerceivedActors.h/.cpp | 소유 Pawn의 AI 타게팅 기록에서 보이는 후보를 Preset 결과에 채움 |
| Public·Private/StateTree/KataStateTreeEvaluator_AI.h/.cpp | TargetActor, bHasVisibleTarget, LastKnownLocation, HomeLocation을 트리 바인딩에 제공 |
| Public·Private/StateTree/KataStateTreeTask_PlayKataAction.h/.cpp | 단일 액션 시작·대기·종료 결과·이탈 정리 |
| Public·Private/StateTree/KataStateTreeTask_PlayWeightedKataAction.h/.cpp | 실행 가능한 후보의 가중 랜덤 선택과 단일 액션 수명 재사용 |
| Public·Private/StateTree/KataStateTreeTask_PlayKataGraph.h/.cpp | 콤보 그래프 시작·진입 트리거·전체 실행 대기·인스턴스 출력 |
| Private/Debug/GameplayDebuggerCategory_KataAI.h/.cpp | 대상·시각 감지·마지막 위치·현재 액션·시작 거절 표시 |
| Plugins/KataFramework/Source/KataFramework/Public·Private/Character/KataAICharacter.h/.cpp | 타게팅 서브오브젝트 교체, 기본 Controller, NPC 행 적용 |

위 Source/KataAI 경로는 Plugins/KataAI 아래다. 헤더와 cpp는 각 모듈 Public·Private에 나누며, 공유 실행 상태는 에셋이 아닌 컴포넌트·태스크 인스턴스가 소유한다.

| 수정 경로 | 변경 |
|---|---|
| Framework Character/KataCharacterRow.h/.cpp | NPC 행에 선택 AIControllerClass·AIData 소프트 참조, 로드 경로 수집, 이전 StateTree 이관 진단 |
| Framework Character/KataCharacterSpawnSubsystem.cpp | AI 설정을 지정한 NPC 행의 캐릭터 타입 계약, 기존 NPC 호환 보존 |
| KataFramework.uplugin·KataFramework.Build.cs | KataAI·StateTree 사용에 필요한 의존 |
| ProjectKata.uproject | KataAI 활성화 |
| AGENTS.md·Plugin-Modularization-Plan.md | 실제 추가 모듈과 KataAI → KataTargeting 관계 명시 |
| docs/manual/AI.md·Character-Data.md·Targeting.md | 생성·인지·Preset·StateTree·액션 Task 계약 |
| 주제별 devlog·docs/README.md | 선택 이유·현재 사용법 진입점 |

2026-10-05 사용자 후속 승인으로 NPC 행에서 UKataAIData를 참조한다. AI Data는 StateTree와 파라미터 오버라이드·Sense 원본 목록·주 감각·Targeting Preset을 모은다. Character는 AI Data의 Transient 참조를 보관하며 Controller가 트리 사본과 복제 Sense를 실행한다. AI Data가 빈 행은 인지·행동을 시작하지 않는다. AIControllerClass의 빈 참조는 Blueprint 기본값을 유지한다.
NPC Tables에 여러 테이블을 등록하며 NPC·Creature의 별도 행 구조는 만들지 않는다. 이전 NPC 행 StateTree는 수동 이관용 직렬화 필드로 보존하고 런타임에서 사용하지 않는다. 추적·수색·공격 수치는 StateTree 파라미터로 구성하며 AI Data 필드로 중복 추가하지 않는다. 어그로·Pressure 상세 설정은 후속 설계다.

AI Data의 Target Refresh Interval은 인지 후보와 Preset의 재평가 주기를 정한다. 대상 선택과 Evaluator의 현재 API 계약은 [AI manual](../manual/AI.md)을 따른다.

## 인지·타게팅 계약

1. AIController는 UAIPerceptionComponent와 Sight 설정을 소유한다. 팀 ID는 빙의한 Pawn의 UKataTargetingComponent에서 전달한다.
2. 성공 감지·시야 상실 이벤트를 Pawn의 AI 타게팅 컴포넌트로 전달한다. 시야 상실은 즉시 보이는 후보에서 제거하지만 마지막 성공 감지 위치는 유지한다. 보이는 대상의 기억 위치는 설정된 갱신 주기에서 갱신하며, 시야 상실 뒤에는 동결한다. 첫 발견 이벤트의 위치만 남아 수색 위치가 오래되는 문제를 피한다.
3. 후보 수집은 인지 기록, 필터는 기존 Kata Filter Faction, 정렬은 Preset의 거리 기준을 사용한다. 월드 전체를 별도로 검색하지 않는다.
4. 보이는 현재 대상은 유지한다. 현재 대상의 시야 상실·무효화·Preset 필터 제외 시 다른 후보를 선택한다. 재선택은 인지 변경과 설정된 갱신 주기에서 수행하며 매 프레임 Preset을 실행하지 않는다.
5. 수색 중 대상의 실제 위치를 읽어 갱신하지 않는다. 새 시각 감지가 있으면 다시 추적한다. 무효·파괴된 액터는 약한 참조에서 제거한다.
6. GetCurrentTarget은 현재 공격 가능한 보이는 액터를 반환한다. 수색은 별도 LastKnownLocation을 사용한다. ResolveActionTarget·CanKeepActionTarget으로 기존 Resolve Target Command와 연결한다.
7. 공격은 시작 시 고정한 대상과 Context를 유지한다. 다음 공격의 대상은 다시 선택한다. 런타임 팩션 변경·사망 태그의 자동 감지는 후속 계약이며 첫 범위에서 가정하지 않는다.

## StateTree 행동 구성

엔진 AI Component 스키마의 Controller·Actor 컨텍스트와 Kata AI Evaluator 출력에 바인딩한다. 전용 태그 이벤트를 새로 정의하지 않고 처음에는 StateTree의 Tick 전이·기본 조건·Wait·Move To를 사용한다.

| 상태 | 진입·동작 | 종료·전이 |
|---|---|---|
| Idle | 대상 없음, 대기 | 보이는 적대 대상 → Chase |
| Chase | Move To TargetActor, 움직이는 목표 추적 | 공격 거리 안 → Attack, 시야 상실 → Search, 이동 실패 → 재시도·Return |
| Attack | 이동을 중지하고 Play KataAction 실행. Task가 실행 인스턴스를 소유 | 완료 → 간격 대기 후 재판단, 시작 거절 → 대기 후 재판단, 파괴·이탈 → 정리 |
| Search | 마지막 감지 위치로 Move To, 도착 후 Wait | 재감지 → Chase, 수색 제한 시간 만료 → Return. 이동 실패도 무한 대기하지 않음 |
| Return | 최초 정상 빙의 시 기록한 HomeLocation으로 Move To | 도착 → Idle, 재감지 → Chase. 실패는 대기 후 제한적으로 재시도 |

Search 시간 제한은 마지막 시야 상실부터 계산하는 안이다. 이동 도중에도 제한이 적용돼 도달 불가능한 위치에 계속 머무르지 않는다. 도착 후 별도 대기 시간과 함께 최종 정책을 구현 전에 정한다.
Sight 범위·시야각·공격 거리·수색 시간·공격 간격·이동 실패 재시도는 에셋 설정으로 노출한다. 구체적인 수치와 공격 에셋은 샘플 구성 때 정하며 확정값으로 기록하지 않는다.

## 액션 실행 Task의 수명

- EnterState에서 Controller의 Pawn과 ASC·UKataActionComponent, 대상·액션을 확인하고 FKataContext를 만들어 PlayKataAction을 한 번 요청한다. 공용 실행 코드는 Framework 타입을 참조하지 않는다.
- 결과 EKataStartResult와 실행 인스턴스, 최종 EKataEndReason을 Task 인스턴스 데이터에 보관한다. 트리의 Task 정의·UKataAction 에셋에는 저장하지 않는다.
- 성공 시 Running, 정상 Completed는 Succeeded, 시작 거절·비정상 종료는 Failed로 전달한다. 시작 결과와 종료 사유를 Output으로 노출해 에셋에서 거절과 오류의 다음 행동을 구분할 수 있게 한다.
- 0초 액션은 시작 함수 안에서 끝날 수 있으므로 반환 직후 IsRunning·GetEndReason을 확인한다. 이후 Tick에서 해당 인스턴스의 종료를 조회하는 기본 구현을 제안한다. 원시 인스턴스 데이터 주소를 비동기 콜백에 캡처하지 않는다.
- ExitState·트리 정지·UnPossess·EndPlay에는 자신이 시작한 인스턴스만 Cancelled 또는 OwnerInvalid로 정리한다. StopKata로 다른 실행 주체의 새 액션을 중단하지 않는다.
- 이동 중지·Focus 정리는 해당 상태·Controller 수명에서 처리한다. 새 타게팅 수집 요청 핸들은 즉시 실행 후 반환 전에 해제한다.
- 후속 Pressure는 액션 종료 델리게이트를 받는 안전한 UObject 예약 소유자를 검토한다. 상태 종료까지 예약을 지연시키지 않고 시작 실패·완료·중단·타겟 파괴에 멱등적으로 해제한다.

## 초기화·GC·호환성

- NPC 행은 기존 deferred spawn의 OnConstruction에서 적용한다. AI는 Pawn 컴포넌트·ASC Actor Info 준비와 정상 게임 월드를 확인한 후 시작한다. 에디터 프리뷰·Construction에서는 트리를 실행하지 않는다.
- 엔진 트리의 자동 시작을 끄고 시작 조건이 갖춰진 한 경로에서만 StartLogic한다. BeginPlay·OnPossess의 호출 순서 차이를 처리해 한 번만 시작한다.
- UnPossess 때 이전 Pawn 구독·이동·트리·기억을 정리한 뒤 새 Pawn과 연결한다. HomeLocation은 새 Pawn 정상 빙의 때 다시 기록한다.
- 현재 대상·인지 기록의 액터는 약한 참조, Perception·StateTree·실행 인스턴스 등 소유 객체는 UPROPERTY로 추적한다. 종료된 실행 인스턴스 참조는 상태 종료에서 비운다.
- 기존 NPC 클래스·행은 강제 변환하지 않는다. StarvedHound Blueprint 부모 변경은 샘플 적용 시 따로 수행하고 기존 타겟 지점·애니메이션·히트박스 설정을 보존한다.
- 기존 타입명·프로퍼티명을 바꾸지 않아 Redirect는 계획하지 않는다. 새 StateTree·Preset 등 샘플 에셋은 Content/KataTest에 둔다.

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 | 완료 조건 |
|---|---|---|---|---|
| AI-1 | 높음 | 플러그인·Controller·AI 캐릭터·NPC 행 수명 연결 | 첫 설계 승인 | 배치·스폰 모두 지정 Controller를 받고 준비 후 트리 시작 |
| AI-2 | 높음 | 시각 인지·AI 타게팅·Preset·Evaluator | AI-1 | 적대 대상 선택, 현재 대상 유지, 마지막 감지 위치 보존 |
| AI-3 | 높음 | Play KataAction Task와 기본 StateTree | AI-2 | 추적·공격·거절 후 대기·수색·복귀, 이탈 시 자원 정리 |
| AI-4 | 보통 | Debugger·문서·StarvedHound 샘플 설정 | AI-3 | 사용자가 행동과 시작 거절 사유를 관찰할 수 있음 |
| AI-5 | 후속 | 청각·피해 감각·어그로·전파 | 기본 루프 확인, 상세 설계 | 대상별 어그로와 팩션을 반영한 대상 교체 |
| AI-6 | 후속 | Pressure·Capability 조건과 예약 | 대상 선택 계약, 상세 설계 | 여러 AI의 동시 공격 제한, 모든 종료에서 예약 해제 |

AI-5·AI-6은 #22 전체 범위에 남으며 이번 첫 구현 승인으로 자동 구현하지 않는다. Pressure 용량의 대상별·그룹별 구분, Capability 표현·태그 루트, 어그로 증가·감쇠·교체 임계값은 후속 설계 항목이다.

## 사용자 확인 항목

에이전트는 빌드·테스트·별도 검사를 실행하지 않는다. 구현 완료와 아래 사용자 확인을 구분한다.

- Editor 빌드 후 KataAI 로드, NPC 배치·스폰 Controller·트리 시작, 기존 PC·NPC 유지.
- 적대만 감지·선택하고 다수 후보 중 현재 대상 유지. 감지 상실 후 마지막 위치만 수색하고 복귀, 재감지 시 추적.
- NavMesh 추적·공격 거리·루트 모션 공격, 쿨다운 거절 후 과도한 반복 없이 대기.
- 정상 완료·시작 중 즉시 종료·외부 중단·대상 파괴·Pawn 빙의 해제·소유자 파괴 시 Task 정리.
- 이동 실패와 수색 시간 만료 시 무한 대기하지 않음. Game 빌드는 사용자가 요청한 확인 범위에 따라 별도 수행.

## 완료 시 갱신할 문서

- [#22](https://github.com/jaykop/Kata/issues/22): 단계별 구현·사용자 확인 결과. 게시·라벨 변경은 초안 승인 후.
- AI manual 신규, [Character Data](../manual/Character-Data.md)·[Targeting](../manual/Targeting.md): 설정·수명·행 계약.
- devlog: 엔진 StateTree 재사용·단일 액션 실행·인지 기억과 첫 범위 선택 이유.
- [플러그인 계획](Plugin-Modularization-Plan.md)·AGENTS: 모듈 구성·의존 문구 정리.
- [문서 목록](../README.md): plan·manual·devlog 링크.


## AI-3 실행 Task 확장 계획

2026-10-05 사용자 요청: Play KataAction 또는 PlayAction 명칭, 단일 Action·Weight 기반 랜덤 Action·콤보 KataGraph 세 방식, 실행 중 State 유지. 이번 요청은 코드 진단과 구현 계획이며 소스 구현은 포함하지 않는다. 기존 단일 액션 계획은 아래 범위로 확장한다.

### 코드 진단

| 기존 계약 | 확인한 동작 | 계획에 미치는 영향 |
|---|---|---|
| UKataActionComponent.PlayKataAction | EKataStartResult와 실행 인스턴스를 반환. 시작 호출 안에서 순간 액션이 종료될 수 있음 | 반환 직후 IsRunning·GetEndReason 확인, 이미 완료된 실행도 놓치지 않음 |
| CanPlayKataAction | 시작 태그·쿨다운·차단·조건을 부작용 없이 확인. 실제 초기화까지 보장하지 않음 | 가중 후보 사전 필터에 사용하되 실제 Play 결과를 최종 판정 |
| UKataActionInstance | 실행 상태·종료 사유 조회, 해당 인스턴스 RequestEnd 제공 | 현재 컴포넌트의 액션 전체를 StopKata하지 않고 자신의 실행만 정리 |
| UKataGraphComponent.StartGraph | bool은 그래프 초기화 성공. 자동 진입 Action이 거절돼 WaitingForEntry여도 true. 기존 Graph는 먼저 중단 | 시작 성공과 첫 Action 실행 성공 분리, Task에서 Busy 사전 거절 제안 |
| UKataGraphInstance | Action 종료 후 자동 전이 또는 예약 전이 실행. Action이 Branched되어도 Graph 수명은 이어짐 | 개별 Action 종료가 아니라 Graph 전체 종료를 대기 |
| Graph 종료 조회 | OnGraphEnded는 있지만 저장된 종료 사유 getter는 없음. 시작 도중 Graph 종료 가능 | EndReason 저장·getter를 추가해 Tick 조회와 순간 완료를 지원 |
| StateTree Task | Running 반환은 미완료 유지. 별도 전이·다른 완료 정책까지 차단하지는 않음 | 공격 상태의 정상 전이는 OnStateCompleted 계열로 구성. 강제 이탈 시 실행 취소 |

근거: KataRuntime의 Runtime/KataActionComponent.h/.cpp·KataActionInstance.h, KataGraph의 KataGraphComponent.h/.cpp·KataGraphInstance.h/.cpp, UE 5.8 StateTreeTaskBase.h. 소스 읽기 진단이며 빌드·실행 검증 결과는 아니다.

### Task 구성 제안

Mode 하나 대신 세 Task를 제안한다. Graph는 입력·출력·대기 단위가 Action과 다르며, 단일 Task에 모으면 사용하지 않는 Action 목록·Graph 설정이 섞인다. 단일과 랜덤은 내부 실행·완료·정리를 공용화한다.

| 표시 이름 | 제안 타입 | 입력 | 주요 출력 |
|---|---|---|---|
| Play KataAction | FKataStateTreeTask_PlayKataAction | Pawn Context, Action, TargetActor | ActionInstance, StartResult, EndReason, Task 결과 |
| Play Weighted KataAction | FKataStateTreeTask_PlayWeightedKataAction | Pawn Context, Action·Weight 목록, TargetActor | SelectedAction, SelectedIndex, ActionInstance, StartResult, EndReason, Task 결과 |
| Play KataGraph | FKataStateTreeTask_PlayKataGraph | Pawn Context, Graph, TargetActor, 선택 EntryTrigger | GraphInstance, 실제 첫 Action 시작 결과의 유효 여부·값, EndReason, Task 결과 |

공통 Task 결과는 성공·시작 거절·후보 없음·Busy·설정 오류·실행 중단을 구분한다. 실행하지 않은 경우 기본 EKataEndReason의 Completed를 실제 성공으로 해석하지 않도록 종료 사유의 유효 여부도 출력한다. Graph의 bool 초기화 결과를 EKataStartResult::Started로 대체하지 않는다.

### 실행과 상태 유지

1. EnterState에서 Pawn·필수 ASC·해당 컴포넌트와 입력을 확인한다. 공유 에셋을 변경하지 않는다.
2. 대상·설정은 진입 시 사본으로 고정한다. 바인딩 입력의 Tick·Exit 자동 복사를 끄거나 별도 실행 기록을 사용해, 실행 중 대상 변경과 정리할 인스턴스 변경을 막는다. 실제 액션의 PreCommands 및 Graph 엣지 bKeepTarget 계약은 기존 실행기가 유지한다.
3. 실행은 EnterState에서 한 번 요청한다. 성공 후 실행 중이면 Running, 호출 안에서 종료됐으면 즉시 실제 종료 결과를 반환한다.
4. Tick은 자신이 시작한 인스턴스의 상태만 조회한다. 완료는 Succeeded, 시작 실패·Cancelled·Interrupted·OwnerInvalid·ContractError는 Failed와 상세 Output으로 구분한다. Graph 내부 Branched는 Task를 완료시키지 않는다.
5. ExitState는 실행 중인 자신의 인스턴스만 RequestEnd(Cancelled)하고 내부 참조를 정리한다. Pawn·컴포넌트 파괴는 기존 EndPlay 경로의 OwnerInvalid와 함께 처리한다. 다른 주체의 새 액션이나 Graph를 StopKata·StopGraph로 중단하지 않는다.
6. Task는 완료 판정에 포함한다. 정상 공격 상태에는 실행 중에도 발동하는 Chase/Search 전이를 두지 않는다. 강제 중단 전이는 허용하며 이탈 정리를 수행한다. Running 반환만으로 모든 StateTree 전이를 잠그는 기능은 만들지 않는다.

기본 구현은 Tick 조회로 제안한다. StateTree 인스턴스 데이터의 원시 주소를 delegate에 캡처하지 않고 구독 누락·순간 종료·수명 문제를 피한다. 인스턴스는 UPROPERTY로 GC 추적하며 StateTree 정의에는 실행 상태를 보관하지 않는다.

### 가중 랜덤 정책 제안

- 목록 원소는 Action과 Weight다. Weight가 유한한 양수이고 에셋이 유효한 항목만 고려한다. 비어 있거나 유효 후보가 없으면 NoEligibleAction으로 실패한다.
- 동일한 진입 Context로 CanPlayKataAction을 평가해 실행 가능한 후보만 남긴다. 비용 지불·GE 적용·입력 소비는 실제 실행에서만 수행한다.
- 남은 후보의 Weight 합을 기준으로 한 번 선택한다. 반복 Tick에서 재추첨하지 않는다. 동일 Action이 여러 행이면 각 행의 Weight가 확률에 기여하고 SelectedIndex는 원래 목록 인덱스다.
- 선택한 Action의 실제 시작이 실패하면 그 결과로 종료한다. 같은 프레임에서 다른 후보를 계속 시도하지 않는다. 재시도 시간·횟수는 StateTree의 Wait·전이가 담당한다.
- 이 정책은 사용자 확정 전 제안이다. 실행 불가능 후보까지 포함해 먼저 뽑는 방식보다 쿨다운 중인 후보 때문에 불필요하게 공격 기회를 잃는 것을 줄인다.

### Graph 진입과 콤보 정책 제안

- 기존 StartGraphOnSelf 또는 같은 Context의 StartGraph를 사용한다. GraphInstance를 Output으로 제공한다.
- 자동 Entry 엣지로 이미 Action이 시작됐으면 EntryTrigger를 보내지 않는다. WaitingForEntry이고 EntryTrigger가 지정돼 있으면 반환된 인스턴스에 한 번 보낸다. Categories=Trigger 메타로 선택기를 제한한다.
- 이후에도 WaitingForEntry이면 Task는 시작 거절 또는 진입 설정 오류로 실패하고 자신이 만든 Graph를 정리한다. 단순 bool 성공 때문에 무한 Running에 머물지 않는다.
- 콤보는 기본적으로 Graph의 자동 전이로 이어간다. Trigger 기반 콤보는 출력된 GraphInstance에 기존 SendTrigger를 보내는 별도 발신자를 사용한다. Task가 매 Tick 임의 Trigger를 반복 전송하지 않는다.
- Graph에 저장된 EndReason getter와 Action 시작 시도의 결과·유효 여부 조회를 추가한다. 초기 거절·순간 완료와 실행 중 종료를 조회할 수 있도록 필요한 관측 API만 늘린다. 기존 #28의 거절 처리·전이·비용 정책은 변경하지 않는다.
- Task 진입 시 기존 Action 또는 Graph가 실행 중이면 Busy로 실패하는 정책을 제안한다. 기존 StartGraph의 강제 교체나 Action의 중단 권한으로 다른 실행을 뜻하지 않게 중단하는 일을 피한다.
- 순환 Graph와 무한 반복 Action은 실제로 끝날 때까지 Running을 유지한다. 별도 제한 시간은 이번 요청에서 추가하지 않으며 StateTree 강제 이탈로 취소할 수 있다.

### 파일·모듈과 구현 순서

| 대상 | 계획 |
|---|---|
| KataAI Public·Private/StateTree/KataStateTreeTask_PlayKataAction.h/.cpp | 단일 Task와 공통 출력·실행 수명 |
| KataAI Public·Private/StateTree/KataStateTreeTask_PlayWeightedKataAction.h/.cpp | 가중 목록 타입·선택 및 공통 Action 실행 재사용 |
| KataAI Public·Private/StateTree/KataStateTreeTask_PlayKataGraph.h/.cpp | Graph 수명·진입 Trigger·인스턴스 Output |
| KataAI Private/StateTree 공용 구현 헤더 또는 함수 | Action 시작·조회·정리 중복 제거. 불필요한 공개 기반 UObject 없음 |
| KataGraph Public·Private/KataGraphInstance.h/.cpp | 저장된 종료 사유와 Action 시작 결과 조회 |
| KataAI.Build.cs·KataAI.uplugin | 공개 타입에 필요한 KataRuntime·KataGraph 모듈, 코어 Kata 플러그인 직접 의존 명시 |
| AI manual·Runtime-Usage·주제 devlog | Task 사용·Output·Graph 관측 API 계약과 선택 이유 |

순서: Graph 관측 API → 단일 Task와 공통 실행 수명 → 가중 Task → Graph Task → 문서 반영. Framework·AIController·Perception 동작 변경은 이번 Task 범위에 필요하지 않다. 코드 변경은 구현 요청 후 수행한다.

### 사용자 확인 항목과 남은 결정

실행 확인은 사용자가 담당한다. 정상 Action 동안 State 유지, 순간 완료, 쿨다운·조건 거절, Weight 0·빈 목록, 자동 Graph 진입·Trigger 진입·진입 거절, 콤보 중 Action 전이, Graph 종료, 강제 State 이탈·외부 중단·Pawn 파괴를 확인한다. 계획 작성 중 빌드·테스트·lint·별도 리뷰는 실행하지 않았다.

남은 제안은 세 Task 분리, 실행 가능한 후보 안에서 가중 선택, Graph 자동 콤보+선택 EntryTrigger, 기존 실행 Busy 거절이다. 사용자 요청의 세 방식·실행 중 State 유지 자체는 확정 요구다. #22 공개 게시·라벨 변경은 사용자 확인 후 수행한다.

### 후속 제안: 가중 목록을 그룹 에셋으로 분리

사용자가 Task 구현 전에 UKataActionGroup 데이터 에셋 계획을 요청했다. Action·Graph·Weight·Payload의 데이터와 가중 선택은 [그룹 계획](Action-Group-Plan.md)에 분리한다. 앞 절의 Play Weighted KataAction 내부 목록 제안은 Play KataActionGroup 소비 Task로 대체하는 안이다. Task 세 가지는 Play KataAction·Play KataGraph·Play KataActionGroup이며, 실행 수명·완료·정리는 앞 계약을 유지한다. 그룹 에셋 범위와 실행 Task 구현 범위를 구분하고 Payload의 Pressure 정책은 후속 설계로 남긴다.

## 후속 설계: 대상 이벤트·수색·복귀

이 절은 앞선 OnTick 감지 전이 안내를 대체한다. 사용자 요청은 Perception 인식에 따른 이벤트 전이이며, 추적·공격 실행 성공은 사용자가 보고했다. 아래 추가 API·태그·실패 정책은 구현 제안이다.

### 이벤트와 상태 갱신 계약

- UKataAITargetingComponent의 SelectCurrentTarget에서 선택 전후 대상을 비교한다. 후보 인지 자체가 아니라 팩션·Preset을 통과한 최종 대상의 변화만 알린다. 같은 대상의 위치 갱신에는 이벤트를 보내지 않는다.
- None → 대상은 TargetAcquired, 대상 → None은 TargetLost, A → B는 TargetChanged 한 번으로 구분한다. 교체를 Lost와 Acquired 두 개로 보내 불필요하게 수색 상태를 거치지 않는다.
- Controller가 대상 변경 델리게이트를 구독하고 실행 중인 StateTreeComponent에 SendStateTreeEvent로 전달한다. StateTree 재진입을 유발하는 동기 전이는 사용하지 않는다.
- 태그는 AI Data에 선택 설정으로 추가한다. 제안 루트는 AI.Event, 프로젝트 태그는 AI.Event.TargetAcquired / TargetLost / TargetChanged다. 플러그인은 태그를 정의하지 않고 Categories="AI.Event"로 선택기를 제한한다. 빈 태그는 해당 이벤트를 보내지 않는다. 기존 AI Data는 기존 전이를 유지할 수 있다.
- 상태·마지막 감지 위치를 먼저 확정하고 알림을 보낸다. 구현 시 엔진의 이벤트 처리와 Evaluator 갱신 순서를 확인하여 전이 조건과 Task가 이전 프레임 대상을 읽지 않도록 한다. 필요하면 현재 타게팅 컴포넌트를 읽는 순수 조건을 제공하며, 이벤트 수신 시점의 Payload를 장기 대상 상태로 사용하지 않는다.
- 초기 인지와 TreeStart 순서에 의존하지 않는다. 처음부터 보이는 대상은 트리 초기 선택으로 처리한다. 공격·대기 완료 후에도 현재 대상 상태로 재선택한다. 이벤트는 상태 변화 통지이며 현재 상태 저장소를 대체하지 않는다.
- 기존 TargetRefreshInterval 타이머는 이동 중 기억 위치 갱신, 팩션·Preset 재평가, 파괴된 대상 정리를 위해 유지한다. 매 Tick 감지 전이 조건을 제거하는 것이며 이동·회전·Action 실행 Tick 자체를 제거하지 않는다.
- 파괴된 weak 대상은 Get()==nullptr 비교만으로 상실이 누락될 수 있다. 직전 선택 유효 상태를 별도로 추적해 파괴·무효화에 따른 상실을 한 번만 알린다.
- UnPossess / EndPlay / 재초기화에서 구독을 해제한다. 정리 과정에는 전투 이벤트를 보내지 않는다. 초기 인지 전에 구독하되 트리 시작 전 알림은 초기 상태 선택으로 보완한다.

### StateTree 구성안

| 상태 | Task와 입력 | 전이 |
|---|---|---|
| Decide | 현재 상태에 따라 자식 선택 | 보이는 대상 → Chase, 기억 위치 → SearchMove, 그 외 → Return 또는 Idle |
| Idle | Delay Task, Run Forever=true | TargetAcquired → Decide |
| Chase | 엔진 MoveTo, Actor=현재 TargetActor | 성공 → Attack, TargetLost → Decide, TargetChanged → Chase 재진입, 실패 → Retry |
| Attack | 기존 Play KataActionGroup | 성공·실패 → AfterAttack. 시야 상실·교체만으로 실행을 취소하지 않음 |
| AfterAttack | Delay Task, Duration=AttackInterval | 완료 → Decide |
| SearchMove | 엔진 MoveTo, Location=마지막 감지 위치 스냅샷 | 성공 → SearchWait, 실패 → Return, TargetAcquired → Decide |
| SearchWait | Delay Task, Duration=SearchDuration | 완료 → Return, TargetAcquired → Decide |
| Return | 엔진 MoveTo, Location=HomeLocation | 성공 → Idle, 실패 → ReturnRetry, TargetAcquired → Decide |
| Retry / ReturnRetry | Delay Task, Duration=MoveRetryInterval | 지연 후 현재 상태로 재판단하거나 복귀 재시도. 프레임마다 실패·재진입하는 루프 금지 |

Decide는 실제 트리 선택용 부모 구조로 표현하며 별도의 빈 Task가 즉시 완료한다는 전제에 의존하지 않는다. Return 성공 뒤에는 검색 기억을 정리하고 Idle로 진입한다. Return 실패 때는 기억을 유지하되 복귀를 다시 시도한다. 재시도 횟수 제한과 복귀 불가 시 최종 정책은 후속 결정 대상이다.

공격 사거리 조건·전이 정책은 현재 샘플 설정을 보존한다. AttackInterval·SearchDuration·MoveRetryInterval·이동 허용 오차는 StateTree 파라미터로 노출하고 AI Data의 기존 StateTree 파라미터 오버라이드를 사용한다. 새 이동 Task·수색 타이머 시스템을 중복 구현하지 않는다. 검색 대기는 마지막 위치 도착 뒤 시작하며 이동 실패 시 곧바로 복귀한다.

### 파일별 구현 순서

| 단계 | 변경 대상 | 완료 조건 |
|---|---|---|
| AI-4A | KataAITargetingComponent.h/.cpp | 최종 대상 변화 델리게이트, 획득·상실·교체 및 파괴 처리, 위치 갱신 중 중복 알림 없음 |
| AI-4B | KataAIData.h/.cpp, KataAIController.h/.cpp | 이벤트 태그 설정·카테고리, 구독·해제·StateTree 전달, 초기 인지·재빙의 순서 처리 |
| AI-4C | 필요 시 AI 순수 조건, 타게팅 기억 정리 API와 StateTree Task | 이벤트 전이 시 최신 상태 조회, 복귀 성공 시 기억 정리. 실행 중 공유 에셋 수정 없음 |
| AI-4D | Config/Tags의 프로젝트 이벤트 태그, ST_StarvedHound와 AI Data 설정 가이드 | 기존 Chase·Attack 보존, Idle·수색·복귀 이벤트 전이와 실패 지연 구성 |
| AI-4E | manual/AI.md, manual/Gameplay-Tags.md, AI devlog | 실제 UI 이름·바인딩·태그·전이 순서와 테스트 안내 반영 |

우선 AI-4A와 AI-4B로 이벤트 연결을 구현한 뒤 수색·복귀를 구성한다. 이번 계획 작성은 코드 구현과 샘플 에셋 변경을 수행하지 않는다. 공개 #22 갱신은 사용자 확인 후 수행한다.

### 사용자 실행 확인 기준

1. 무인지 상태에서 Idle 유지. 감지 한 번에 Chase 시작. 같은 대상을 계속 보는 동안 획득 이벤트 반복 없음.
2. 이미 보이는 대상이 있는 상태에서 트리를 시작해도 Chase 시작. AfterAttack 뒤 새로운 획득 이벤트 없이 재추적·재공격 가능.
3. Chase 중 벽 뒤로 숨으면 마지막 성공 감지 위치로 이동. 숨은 플레이어의 실제 위치를 따라가지 않음.
4. SearchMove / SearchWait / Return 중 재감지하면 Chase 재개. SearchDuration 뒤 HomeLocation 복귀.
5. 공격 중 시야 상실·대상 교체는 진행 중 Action을 유지하고 완료 후 최신 상태로 판단. 대상 파괴 시 기존 Action 수명 정책 적용.
6. A → B 교체에서 수색 상태를 거치지 않고 새 대상으로 이동. 현재 대상 파괴 시 상실 처리 누락 없음.
7. 이동 불가 시 프레임 단위 재시도 폭주 없음. 복귀 성공 뒤 과거 기억 때문에 다시 수색하지 않음.
8. UnPossess·재빙의·레벨 종료 후 중복 이벤트나 이전 Pawn 참조 없음. 이동 Yaw 회전과 기존 공격 동작 회귀 확인.

빌드·PIE 확인은 사용자가 담당한다. 수색 시간·재공격 간격·재시도 간격의 구체 값은 샘플 튜닝 제안으로 제시하고 실제 사용자 확인과 구분한다.

## 이벤트 태그 확정 변경

사용자 후속 결정으로 AI Data별 이벤트 태그 설정을 제거한다. 별도 AI.ini를 만들지 않고 기존 Event.ini를 Config/Tags/Native로 이동해 Event.AI.TargetAcquired / TargetLost / TargetChanged를 통합한다. 앞선 AI.Event 선택 설정 제안을 대체한다. 생성 코드는 ProjectKata 모듈 소유이므로 KataAI는 등록된 고정 태그 이름을 조회하여 모듈 경계를 유지한다. 이전 태그 이름은 Redirect로 이관한다.


## StateTree 이벤트 루트 후속 결정

사용자 요청으로 앞선 Event.AI 통합안을 대체한다. Native/StateTree.ini에 StateTree.Event.AI.*와 StateTree.Event.Camera.Reselect를 분리하며, 사용하지 않는 AI.Event Redirect는 제거한다.
