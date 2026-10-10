# 해석 결과 캐시 계획

작성: 2026-10-10  
갱신: 2026-10-10  
연결 이슈: [#47 KataAction 해석 결과 캐시 (측정 후 결정)](https://github.com/jaykop/Kata/issues/47)  
현재 상태 근거: [런타임 사용법](../manual/Runtime-Usage.md), [태스크 제작](../manual/Task-Authoring.md#실행-수명과-제한), 소스 대조(2026-10-10)  
대체 관계: 없음

## 목적과 현재 상태

`UKataAction::Resolve`는 호출할 때마다 부모 체인을 새로 병합한다. 그 과정에서 UObject를 여러 개 생성하고 Instanced 객체를 복제한다.
`UKataActionComponent`의 `PlayKataAction`·`PlayKataActionTransition`·`CanPlayKataAction`이 매번 이 해석을 호출한다.
이 계획은 에셋별로 읽기 전용 해석 결과(템플릿)를 캐시해 시작 판정과 재생이 템플릿을 공유하게 하는 구조를 검토한다.
실행 상태는 지금처럼 `UKataActionInstance`와 `UKataTaskInstance`만 새로 만든다.

2026-10-10 [#50](https://github.com/jaykop/Kata/issues/50)에서 상속을 [Template, Action] 1단계로 바꿨다. 해석 함수는 `UKataActionAssetBase`로 옮겼고,
아래의 `UKataAction::Resolve`·`MakeEffectiveSettings`·`ResolveChain` 설명은 같은 로직을 가리킨다. 순환 검사(`TSet`)는 제거됐다.
세대 값은 `UKataActionAssetBase`에 두고, 체인 비교는 부모 Template 하나의 세대 비교로 줄어든다.

참고한 원칙은 CAPCOM REDox의 접근이다. 공유 데이터는 읽기 전용으로 한 번만 만들고, 필요할 때만 해석하며, 바뀐 부분만 다시 만든다.
비용은 아직 측정하지 않았다. 측정 결과에 따라 캐시 구현 여부와 범위를 정한다([측정 방법](#측정-방법)).

### 현재 해석 한 번의 비용 구성

소스 기준([KataAction.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Action/KataAction.cpp))으로 `Resolve` 한 번은 다음을 수행한다.

1. `CollectActionChain`: 부모 체인 수집. `TSet` 하나와 배열 하나를 사용한다.
2. `MakeEffectiveSettings`(79행): Transient 패키지에 `UKataAction` 사본을 `NewObject`로 만든다. 체인 단계마다 고유 설정 경로 문자열을 분해하고 `CopyOverriddenProperty`로 복사한다.
   이때 `StartCondition`·`PreCommands`·`PostCommands`의 Instanced 객체를 `StaticDuplicateObjectEx`로 복제한다.
   루트 단계에서 한 번 복제하고, 자식이 같은 설정을 오버라이드하면 그 단계에서 다시 복제한다.
3. `ResolveChain`(159행): `UKataResolvedAction`을 `NewObject`로 만든다. 2단계에서 복제한 조건·명령을 `DuplicateObject`로 **다시** 복제한다(174·186·197행).
   체인의 모든 타임라인 태스크도 `DuplicateObject`로 복제한다(232행). 태스크가 가진 Instanced 하위 객체(예: Hit Trace의 `HitHandlers`)도 함께 복제한다.
   Modify 오버라이드가 Instanced 프로퍼티를 바꾸면 그 값도 다시 복제한다. 이후 의존성 검사와 정렬에 `TMap`·배열을 사용한다.
4. 2단계의 Effective 사본과 그 아래 복제본은 반환 직후 참조가 사라져 다음 GC의 회수 대상이 된다.

조건·명령은 `Resolve`마다 최소 두 번 복제된다. 시작 판정만 하는 호출도 태스크 전체를 복제한다.

## 범위

- 포함: 런타임 해석 결과의 에셋별 캐시, `CanPlayKataAction`의 무복제 판정, 캐시 무효화(에디터 편집·PIE·cooked), 측정 절차, 관련 manual 규칙 변경.
- 제외: `UKataActionInstance`·`UKataTaskInstance` 풀링, 해석 결과를 저장 시점에 에셋으로 굽는 방식(아래 대안 B), KataGraph 실행 사본 구조 변경,
  에디터 편집용 해석(`bForEditing`) 경로의 캐시.

## 객체별 공유 가능성

2026-10-10 소스 대조 결과다. "실행 중 자기 필드 기록"은 해석 결과에 들어간 설정 객체가 재생 중 자신의 필드를 바꾸는지를 뜻한다.

| 대상 | 실행 중 자기 필드 기록 | 판정 | 근거 |
|---|---|---|---|
| `UKataTask` 기반 | 없음 | 공유 가능 | 실행 상태는 `UKataTaskInstance`가 보관한다는 규칙([KataTask.h](../../Plugins/Kata/Source/KataRuntime/Public/Action/KataTask.h), [태스크 제작](../manual/Task-Authoring.md#실행-수명과-제한)) |
| Apply Gameplay Effect, Apply Loose Tag, Cancel Window, Play Montage, Send Gameplay Event, Transition Window | 없음 | 공유 가능 | 핸들·약한 참조·Transient 필드는 모두 짝 `UKataTaskInstance_*`에 있다. 실행 클래스는 `const` 정의로 Cast해 읽는다 |
| AI Send Trigger (KataAI), Rotate To Facing (KataTargeting) | 없음 | 공유 가능 | 상태 필드는 실행 클래스에 있다. 정의는 `const`로 읽는다 |
| Hit Trace (KataFramework) | 없음 | 공유 가능(주의) | 실행 클래스가 정의를 non-const 포인터로 받아 `FKataHitBoxRegistration::Task`(원시 포인터)로 넘긴다. 서브시스템은 `HitBoxPreset`·`bCheckOnStart`·`FilterPreset`·`HitHandlers`를 읽기만 하며, 태스크 포인터를 키로 쓰는 맵·집합도 없다 |
| `UKataHitHandler` 파생 (Hit Trace 하위 객체) | 없음 | 공유 가능 | `HandleHit`이 `const`다 |
| `UKataCondition` 기반과 Angle·Attribute·Distance·Group·Tag·Move Direction | 없음 | 공유 가능 | `EvaluateCondition`이 `const`이고 부작용이 없다는 규칙 |
| `UKataCommand` 기반 | `RunningInstance`(Transient)를 `Run` 동안만 기록 | 조건부 공유 가능 | `TGuardValue`로 설정·복원하므로 동기 실행과 중첩 호출에는 안전하다. 명령은 같은 호출 안에서 끝나야 한다는 규칙이 전제다 |
| Resolve Target, Resolve Facing, Face Move Direction (KataTargeting) | 없음 | 조건부 공유 가능 | 기반 클래스의 `RunningInstance` 외 상태 필드가 없다 |
| Blueprint 파생 Task·Command | 작성자에 따라 다름 | 복제 필요 가능 | BP 설정 변수는 실행 BP가 `GetTaskDefinition`(non-const 반환)으로 바꿀 수 있다. 지금은 재생마다 사본이라 다음 재생에 새지 않지만 공유하면 샌다. 저장소에는 현재 BP 파생이 없다(소스 기준, 콘텐츠 미확인) |
| 에디터 편집용 해석(`Resolve(..., true)`) | 편집기가 태스크 사본을 직접 수정 | 복제 필요 | `FKataActionEditor::Refresh`가 사본에 `RF_Transactional`을 붙이고 Task Details에서 편집한다 |
| `UKataResolvedAction` 자체 | 없음 | 공유 가능 | "읽기 전용 실행 데이터" 규칙. 실행 경로에서 필드에 쓰는 코드를 찾지 못했다 |

정리하면 현재 소스의 네이티브 설정 객체는 모두 템플릿으로 공유할 수 있다.
복제가 꼭 필요한 대상은 에디터 편집용 해석과, 규칙을 어기고 설정 변수를 바꾸는 BP 파생 객체다.

## 호출 지점과 빈도

모든 런타임 진입점은 `UKataActionComponent`를 거친다([#46](https://github.com/jaykop/Kata/issues/46) 결정으로 이 구조를 유지한다). 따라서 캐시는 컴포넌트 아래 한 곳에서 처리할 수 있다.

| 호출 지점 | 경로 | 해석 횟수 | 빈도 특성 |
|---|---|---|---|
| [KataActionComponent.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionComponent.cpp) `PlayKataAction` | `Resolve(this)` → `StartResolved` | 1 | 판정에서 거절돼도 전체 해석을 이미 마친 상태다 |
| 같은 파일 `PlayKataActionTransition` | `UKataGraphInstance::StartNode`([KataGraphInstance.cpp](../../Plugins/Kata/Source/KataGraph/Private/KataGraphInstance.cpp)) | 전이 시도마다 1 | 입력 트리거 전이, 예약 전이, 자동 전이에서 호출한다. 콤보 연타 시 입력 빈도를 따른다. 쿨다운·태그로 거절돼도 해석한다 |
| 같은 파일 `CanPlayKataAction` | `Resolve(GetTransientPackage())` → `CanStartResolved` | 1, 결과 폐기 | 현재 유일한 호출자는 KataAI Group 태스크다 |
| [KataStateTreeTask_PlayKataActionGroup.cpp](../../Plugins/KataAI/Source/KataAI/Private/StateTree/KataStateTreeTask_PlayKataActionGroup.cpp) `EnterState` | Action 항목마다 `CanPlayKataAction`, 선택 후 `PlayAction` → `PlayKataActionOnSelf` → `PlayKataAction` | Action 항목 수 + 1 | AI 수 × 상태 진입 빈도 × 항목 수에 비례한다. 선택된 항목은 같은 프레임에 두 번 해석한다 |
| [KataStateTreeExecutionUtils.cpp](../../Plugins/KataAI/Source/KataAI/Private/StateTree/KataStateTreeExecutionUtils.cpp) `PlayAction` | `PlayKataActionOnSelf` | 1 | 단일 Action 태스크의 상태 진입마다 |
| [AbilityTask_PlayKataAction.cpp](../../Plugins/Kata/Source/KataRuntime/Private/GAS/AbilityTask_PlayKataAction.cpp) | `PlayKataAction` | 활성화마다 1 | 현재 사용처는 없다. #23의 반응 GA가 사용할 예정이므로 피격 빈도를 따르게 된다 |
| [KataActionEditor.cpp](../../Plugins/Kata/Source/KataEditor/Private/KataActionEditor.cpp) `Refresh` | `MakeEffectiveSettings` 1회 + `Resolve(편집)` + `Resolve(실행)` | Effective 3, ResolveChain 2 | 편집·Undo·부모 에셋 변경마다. Tick에서 한 프레임에 한 번으로 합친다 |
| [SKataPreviewViewport.cpp](../../Plugins/Kata/Source/KataEditor/Private/SKataPreviewViewport.cpp) `Start` | `PlayKataAction` | 1 | 프리뷰 재생, 편집 후 장면 재생성, 뒤로 탐색마다 |
| `UKataAction::IsDataValid` | `Resolve` | 1 | 저장·데이터 검증 때 |

## 설계안

### 템플릿 캐시

- 에셋마다 실행용 `UKataResolvedAction` 템플릿을 하나 만들고 `const`로 공유한다. 첫 요청 때 기존 `Resolve`로 만든다(지연 생성).
- `CanPlayKataAction`은 템플릿으로 `CanStartResolved`만 수행한다. 객체를 만들지 않는다.
- `PlayKataAction`·`PlayKataActionTransition`도 같은 템플릿을 `UKataActionInstance`에 넘긴다. 재생마다 생기는 객체는 실행 인스턴스와 태스크 인스턴스뿐이다.
- 실행 중인 인스턴스는 `ResolvedDefinition`으로 템플릿을 강하게 참조한다. 캐시가 무효화돼도 진행 중인 재생은 이전 템플릿으로 끝까지 진행한다.
  지금 재생 중인 인스턴스가 시작 시점의 사본을 유지하는 동작과 같다.
- 기존 `Resolve(Outer, bForEditing)`는 새 객체를 반환하는 현재 의미를 유지한다. 에디터 편집, 데이터 검증, 진단 표시는 계속 이 함수를 사용한다.
  캐시 조회는 별도 API로 추가한다(이름은 미확정).

### 캐시 위치 후보

| 후보 | 장점 | 단점 |
|---|---|---|
| A1. `UKataAction`의 Transient 멤버 | 조회가 포인터 하나다. 에셋 언로드 시 함께 해제된다 | `const` 함수에서 캐시를 채우려면 `mutable`이 필요하다. UHT 지원 여부를 구현 시 확인한다 |
| A2. 엔진·게임 인스턴스 서브시스템의 `TMap<TWeakObjectPtr<const UKataAction>, ...>` | 에셋 타입이 바뀌지 않는다. PIE 종료 시 일괄 해제할 수 있다 | 맵 조회 비용과 약한 참조 정리가 필요하다. 코어에 서브시스템이 하나 늘어난다 |
| A3. `UKataActionComponent`별 캐시 | 수명이 단순하다 | 같은 에셋을 쓰는 캐릭터 수만큼 템플릿이 생긴다. AI 다수 상황의 이득이 줄어든다 |

제안은 A1이다. A2는 A1의 `mutable` 문제가 있을 때 쓴다.

### 무복제 최소 개선안(R0)

측정 결과 캐시까지 필요 없다고 판단하면, 실행용 해석에서 `MakeEffectiveSettings`의 Instanced 복제만 건너뛰는 방안이 있다.
Effective 사본은 `ResolveChain`이 다시 복제하기 전에 잠깐만 쓰이므로 원본 참조로 충분하다. 조건·명령의 이중 복제와 Effective 하위 객체 생성이 사라진다.
태스크 복제와 시작 판정의 낭비는 남는다.

## 캐시 무효화

### KataGraph의 세대 패턴과 비교

KataGraph는 저장 시점에 실행 사본을 에셋에 굽는다. `UKataGraph::CompiledGeneration`은 저장용 재구성에 성공할 때만 새로 발급한다.
`CompiledDependencies`는 마지막 재구성에 쓴 외장 서브그래프와 그 세대를 기록한다. 에디터는 이를 현재 세대와 비교해 "의존 그래프를 먼저 저장하라"고 안내한다.
두 필드는 `WITH_EDITORONLY_DATA`이며 런타임은 구워진 데이터만 읽는다. PIE 중 저장하면 실행 중인 인스턴스는 이전 데이터를 유지한다는 알림을 띄운다
([KataGraphBuildContext.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/KataGraphBuildContext.cpp), [KataGraphEditorModule.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/KataGraphEditorModule.cpp)).

| 관점 | KataGraph(현재) | 액션 캐시 대안 A: 메모리 지연 캐시(제안) | 액션 캐시 대안 B: 저장 시 굽기 |
|---|---|---|---|
| 생성 시점 | 저장(PreSave) | 첫 재생·판정 요청 | 저장(PreSave) |
| 세대 | 영속 `FGuid`, 저장 시 발급 | Transient 정수, 편집 때 증가 | 영속 `FGuid` |
| 의존 기록 | 외장 원본별 세대 목록 | 캐시 항목에 부모 체인 각 단계의 (약한 참조, 세대) 목록 | 부모 액션별 세대 목록 |
| 부모 편집 반영 | 자식 재저장 필요 | 다음 요청에서 체인 세대 비교 후 재생성 | 자식 재저장 필요 |
| PIE 중 편집 | 재시작해야 반영 | 다음 재생부터 반영(현재 동작과 같다) | 저장 후에도 실행 중 인스턴스는 이전 데이터 |
| cooked | 구운 데이터 사용 | 세대가 바뀌지 않으므로 비교를 생략할 수 있다. 첫 사용 시 한 번 해석한다 | 로드 비용만 든다 |
| 에셋 형식 변경 | 있음(실행 노드 직렬화) | 없음 | 있음 |

대안 A는 KataGraph의 의존 세대 비교 방식을 빌리되 영속 저장 대신 메모리 세대를 사용한다.
현재 액션은 매번 해석하므로 저장 없이도 편집이 즉시 반영된다. 이 동작을 유지하려면 메모리 방식이 맞다.
대안 B는 에셋 형식 변경과 자식 재저장 부담이 커서 제외 범위로 둔다.

### 대안 A의 무효화 규칙

- 각 `UKataAction`에 Transient 세대 값을 둔다. 캐시 항목은 생성 당시 체인 각 단계의 세대를 기록한다.
  조회 때 현재 `ParentAction` 체인을 따라가며 세대를 비교하고, 하나라도 다르거나 체인 구성이 바뀌었으면 다시 만든다.
- 세대 증가 시점(모두 `WITH_EDITOR`):
  - `UKataAction::PostEditChangeChainProperty`, `PostEditUndo`.
  - Instanced 하위 객체(Task·Condition·Command·Hit Handler) 편집. 하위 객체의 PostEditChange는 액션에 전달되지 않는다.
    `FCoreUObjectDelegates::OnObjectPropertyChanged`에서 대상의 `GetTypedOuter<UKataAction>()` 세대를 올린다. 액션 에디터의 `OnObjectChanged`와 같은 판정이다.
  - Undo·Redo: `FCoreUObjectDelegates::OnObjectTransacted`.
  - BP 재컴파일 재인스턴싱, 패키지 다시 로드, Live Coding: `FCoreUObjectDelegates::OnObjectsReplaced` 등에서 전역 세대(epoch)를 올려 전부 버린다.
- 부모 편집은 부모 세대만 올린다. 자식 캐시는 다음 조회에서 체인 비교로 무효화된다. 열린 자식 에디터 갱신은 기존 `OnObjectChanged` 경로가 맡는다.
- cooked(`!WITH_EDITOR`): 세대가 변하지 않으므로 체인 비교 코드를 빼고 포인터 확인만 한다.
- 진단 로그: 지금은 오류가 있는 액션을 시작할 때마다 `LogDiagnostics`가 출력한다. 캐시 후에도 같은 빈도로 출력할지, 세대마다 한 번만 출력할지 정한다.

### PIE와 에디터

- 에셋 객체는 에디터와 PIE가 공유한다. PIE에서 만든 템플릿은 PIE 종료 후에도 남지만 읽기 전용이므로 문제가 없다. 편집이 생기면 세대 비교로 버린다.
- 에디터 `Refresh`의 실행용 `Resolve`(진단 표시)는 캐시 조회로 바꿀 수 있다. 편집용 해석은 복제 사본을 유지한다.
- 프리뷰 재생은 컴포넌트를 거치므로 자동으로 캐시를 사용한다. 편집 직후 재생은 세대가 올라간 상태라 다시 해석한다.

## 측정 방법

구현 여부를 정하기 전에 현재 비용을 측정한다. 측정 실행은 사용자가 맡는다.

1. 측정 스코프 준비(코드 변경, 사용자 승인 후): `UKataAction::Resolve`, `MakeEffectiveSettings`, `ResolveChain`,
   `UKataActionComponent::CanPlayKataAction`·`StartResolved`에 `TRACE_CPUPROFILER_EVENT_SCOPE`를 추가한다.
   `StaticDuplicateObjectEx`·GC의 엔진 스코프가 이미 보이면 그대로 쓴다. Insights에서 보이지 않을 때만 해당 호출부 주변에 Kata 스코프를 추가한다.
2. 빌드: Development Editor로 PIE를 측정하고, 대표 값은 Development Game(`-game` 또는 패키지)으로 다시 측정한다. Shipping은 트레이스가 제한되므로 쓰지 않는다.
3. 실행: `-trace=default,memory` 인자로 Unreal Insights 트레이스를 기록한다. 채널 이름은 UE 5.8 문서로 확인한다.
4. 시나리오:
   - S1: PC가 3~4단 콤보를 30초 연타한다(그래프 전이 경로).
   - S2: Action 항목 4개 이상의 KataActionGroup을 쓰는 AI 10체와 30체가 30초 교전한다(CanPlay 경로).
   - S3: 쿨다운 중인 액션을 반복 입력한다(거절 경로).
5. 읽을 값:
   - Timing Insights: 스코프별 호출 수, 평균·최대 시간, 프레임당 합계. 스파이크 프레임에서 `Resolve`의 비중.
   - GC: GC 스코프(예: `CollectGarbage`)의 발생 간격과 시간, 증분 정리(purge) 시간.
   - Memory Insights: 측정 구간의 할당 횟수와 크기 추이.
   - 객체 수: 콘솔 `obj list class=KataResolvedAction`, `obj list class=KataAction`을 시나리오 전후와 GC 직후(`obj gc`)에 기록한다.
     Transient Effective 사본이 쌓이는 양을 볼 수 있다.
6. 판단 기준(제안): 프레임당 해석 비용이 게임 스레드 예산의 1%를 넘거나, 해석 생성 객체 때문에 GC 빈도가 눈에 띄게 늘면 캐시를 구현한다.
   그보다 작으면 R0만 적용하거나 보류한다. 기준값은 사용자가 정한다.
7. 구현 후에는 같은 시나리오를 캐시 켬·끔으로 비교한다. 비교를 위해 개발 빌드용 CVar(예: `kata.Action.ResolveCache`)를 둘지 결정한다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 측정 후 판단 | 확정 | 비용은 측정하지 않았다. 측정 결과로 구현 여부를 정한다(요청 대화) |
| 런타임 진입점 단일화 | 확정 | 모든 런타임 경로가 `UKataActionComponent`를 거친다. #46 결정 |
| 네이티브 설정 객체 공유 가능 | 제안 | [객체별 공유 가능성](#객체별-공유-가능성)의 소스 대조 결과. 콘텐츠의 BP 파생은 확인하지 않았다 |
| 메모리 지연 캐시(대안 A) | 제안 | 저장 시 굽기(대안 B)는 제외한다 |
| 캐시 위치 | 결정 필요 | A1 `UKataAction` Transient 멤버(제안), A2 서브시스템, A3 컴포넌트별 |
| 캐시 조회 API 이름과 공개 범위 | 결정 필요 | 예: `GetRuntimeTemplate() const`. C++ 전용으로 할지 BP에 노출할지 |
| BP 파생 Task·Command 처리 | 결정 필요 | (1) 설정 불변 규칙만 문서화, (2) 개발 빌드에서 변경 감지 경고, (3) 클래스 단위로 재생마다 복제하는 선택 플래그 |
| 에디터에서 캐시 사용 | 결정 필요 | 무효화 규칙과 함께 에디터·PIE에서도 사용(제안), 또는 에디터에서는 끄고 cooked·`-game`에서만 사용 |
| 진단 로그 빈도 | 결정 필요 | 재생마다 출력 유지 또는 세대마다 한 번 |
| 첫 사용 지연 | 결정 필요 | 지연 생성만 할지, 컴포넌트 BeginPlay나 그래프 시작 때 미리 만들지 |
| 측정용 CVar와 트레이스 스코프 | 결정 필요 | 측정 단계에서만 추가할지, 남겨 둘지 |
| R0 단독 적용 | 결정 필요 | 측정 결과가 작을 때의 대안 |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| M1 | 높음 | 측정 스코프 추가와 현재 비용 측정 | 스코프 추가 승인 | S1~S3의 스코프 시간, GC 간격, 객체 수가 이슈에 기록된다 |
| D1 | 높음 | 미확정 항목 결정 | M1 | 캐시 위치·API·BP 처리·에디터 사용 여부가 이 문서에 확정으로 표시된다 |
| R0 | 보통 | 실행용 해석의 Effective Instanced 복제 제거 | M1, D1에서 채택 | 실행 결과와 진단은 같고, `Resolve`당 조건·명령 복제가 한 번으로 줄어든다 |
| C1 | 보통 | 템플릿 캐시와 체인 세대 비교 | D1 | 같은 에셋을 반복 재생해도 재생당 `UKataResolvedAction`이 새로 생기지 않는다 |
| C2 | 보통 | `CanPlayKataAction`·Play 경로를 템플릿으로 전환 | C1 | Group 태스크의 판정에서 해석 객체가 생기지 않는다 |
| C3 | 보통 | 에디터 무효화 훅 | C1 | 부모·자식·하위 객체 편집, Undo·Redo, BP 재컴파일 직후의 재생이 최신 설정을 사용한다 |
| C4 | 낮음 | 측정 재실행과 비교 | C2, C3 | 캐시 켬·끔 수치가 이슈에 기록된다 |
| DOC | 보통 | manual 규칙 갱신 | C2 | 아래 문서가 공유 템플릿 기준으로 바뀐다 |

## 영향과 제한

- 모듈 경계: 캐시와 무효화 훅은 모두 `KataRuntime`에 둔다. 에디터 훅은 CoreUObject 델리게이트만 쓰고 `WITH_EDITOR`로 감싼다. UnrealEd 의존은 추가하지 않는다.
- 공개 규칙 변경: [태스크 제작](../manual/Task-Authoring.md#command-제작)의 "실행 객체는 매 액션 해석에서 복제한다"가 바뀐다.
  Command와 Task 설정은 여러 재생과 여러 캐릭터가 공유하게 된다. `GetResolvedDefinition()`이 반환하는 객체도 재생 간 공유된다([런타임 사용법](../manual/Runtime-Usage.md)).
- 직렬화: 대안 A는 에셋 형식을 바꾸지 않는다. Transient 데이터만 추가한다.
- 자원 수명: 템플릿은 에셋이 로드된 동안 유지된다. 지금은 재생마다 생성 후 버리는 객체가 에셋당 한 벌로 바뀐다.
  `FKataHitBoxRegistration::Task`는 원시 포인터다. 실행 인스턴스가 템플릿을 강하게 참조하므로 수명은 지금과 같다.
- 스레드: 해석과 조회는 게임 스레드에서만 수행한다는 전제를 유지한다.
- 이 문서는 빌드·테스트·측정 실행 권한을 부여하지 않는다.

## 사용자 확인 항목

- 구현 완료 조건: 위 작업표의 완료 조건을 소스로 충족한다.
- 실행 확인 조건(사용자): Editor·Game 빌드. 콤보·AI Group·프리뷰 재생이 이전과 같게 동작하는지 확인한다.
  PIE 중 부모 액션을 편집한 뒤 다음 재생에 반영되는지, 측정 수치가 개선됐는지도 확인한다.
- 아직 확인하지 않은 범위: 현재 비용 수치, 콘텐츠의 BP 파생 Task·Command 존재 여부.

## 완료 시 갱신할 문서

- [연결 이슈](https://github.com/jaykop/Kata/issues/47): 측정 결과, 결정, 구현·확인 상태.
- [태스크 제작](../manual/Task-Authoring.md): Command·Task 설정 공유 규칙과 BP 설정 변수 금지 규칙.
- [런타임 사용법](../manual/Runtime-Usage.md): `UKataResolvedAction` 공유, 캐시 조회 API, 무효화 시점.
- devlog(새 문서): 캐시 구조와 측정 결과, 대안 B를 택하지 않은 이유.
- [문서 목록](../README.md): 계획을 삭제할 때 링크를 제거한다.
