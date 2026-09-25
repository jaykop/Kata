# 플러그인 분리 모듈화 계획

작성: 2026-09-24  
갱신: 2026-09-24  
연결 이슈: [#1 플러그인 분리 모듈화](https://github.com/jaykop/Kata/issues/1)  
현재 상태 근거: [현재 구현 상태](../devlog/Implementation-Status.md) · [모듈 구조 진단](../devlog/2026-09-24-Module-Structure-Diagnosis.md)  
대체 관계: 없음

## 목적과 현재 상태

타게팅, AI, 카메라 같은 기반 시스템([액션 게임 기반 시스템 계획](Action-Game-Systems-Plan.md))을 추가하면
Beta·Experimental 엔진 플러그인 의존이 함께 생긴다. 현재 `Kata` 플러그인 하나에 모듈만 추가하면
`Kata.uplugin`에 그 의존을 선언해야 하므로, 해당 기능을 쓰지 않는 사용자도 그 플러그인을 켜야 한다.
또한 코어 `KataRuntime`에 있는 `AKataCharacter`는 의존 방향 때문에 `UKataGraphComponent`나 이후 위성 플러그인의
컴포넌트를 가질 수 없다. 진단 내용은 [모듈 구조 진단](../devlog/2026-09-24-Module-Structure-Diagnosis.md)에 있다.

현재 구조는 `Kata` 플러그인 하나(`KataConditions`, `KataRuntime`, `KataGraph`, `KataEditor`, `KataGraphEditor`)와
프로젝트 모듈 `ProjectKata`, `ProjectKataTesting`이다. 각 단계의 진행 상태는 [#1](https://github.com/jaykop/Kata/issues/1)을 따른다.

## 범위

- 포함: 플러그인 구성과 의존 방향, 코어가 제공할 확장 지점, `AKataCharacter` 이동, 새 플러그인 뼈대, 지침 문서 수정.
- 제외: 각 위성 플러그인의 기능 상세 설계. 타게팅은 별도 `Targeting-Plan.md`에서 다룬다.
  BT(Behavior Tree) 어댑터는 추가하지 않는다.

## 목표 구조

```
Plugins/
  Kata/             코어              의존: GameplayAbilities
    KataConditions  조건, Context
    KataRuntime     액션, 실행, GAS 연결, 외부 의존 없는 태스크, `UKataCommand`
    KataGraph       그래프, 콤보 전이
    KataEditor / KataGraphEditor
  KataTargeting/    타게팅 컴포넌트, Preset 커스텀 태스크, 팩션
                    의존: Kata, TargetingSystem(Beta), AIModule(엔진 모듈)
  KataAI/           Perception 연결, 대상별 어그로, Pressure·Capability, KataAIController, StateTree Task
                    의존: KataTargeting, AIModule, StateTree, GameplayStateTree
  KataCamera/       카메라 모드, 락온 화면 구성, 카메라 태스크 (후속)
                    의존: Kata, 카메라 기반 결정에 따라 GameplayCameras(Experimental)
  KataFramework/    통합: AKataCharacter(모든 컴포넌트 조합), PlayerController(입력 → 트리거 태그)
                    의존: 위 플러그인 전부, EnhancedInput
Source/
  ProjectKata         샘플 전용. 재사용 코드는 두지 않는다
  ProjectKataTesting  개발 검증용 DeveloperTool 모듈(유지)
```

각 플러그인은 `Plugins/` 아래 형제 폴더로 둔다. 엔진 플러그인 상태는 2026-09-24에 UE 5.8 설치 폴더의
`.uplugin`으로 확인했다. `TargetingSystem`은 Beta이며 `Plugins/Experimental` 아래에 있고,
`StateTree`·`GameplayStateTree`는 Beta가 아니며, `GameplayCameras`는 Experimental 0.1이다.

## 분리 원칙

1. 코어는 엔진과 GAS에만 의존한다. Character·Controller 같은 게임 프레임워크의 구체 클래스를 두지 않는다.
   코어가 요구하는 것은 "ASC와 `UKataActionComponent`를 가진 액터"라는 규격뿐이다.
2. 추가 엔진 플러그인 의존이 생기거나, 끄고 켤 수 있어야 하는 기능 묶음이면 별도 플러그인으로 만든다.
3. 의존은 위성 → 코어 한 방향이다. 코어는 위성을 알지 못하며 확장 지점(`UKataCommand`, 인터페이스, 델리게이트)만 제공한다.
4. 외부 의존 없는 태스크는 코어에 둔다. 외부 의존이 있는 태스크(Motion Warping, 카메라, Niagara 등)는 해당 시스템 플러그인에 둔다.
5. Editor 모듈은 편집기 UI가 실제로 필요할 때만 추가한다.
6. 여러 플러그인을 조합하는 클래스는 `KataFramework`에만 둔다. 프로젝트는 샘플 전용이다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 모듈이 아닌 플러그인 단위 분리 | 확정 | 2026-09-24 사용자 결정. 위성 기능의 엔진 플러그인 의존을 코어에서 떼어 낸다 |
| `AKataCharacter` 위치 | 확정 | 2026-09-24 사용자 결정. 통합 플러그인 `KataFramework`로 이동 |
| 프로젝트의 역할 | 확정 | 2026-09-24 사용자 결정. `ProjectKata`는 샘플 전용 |
| KataAI 범위 | 확정 | 2026-09-24 사용자 확인. StateTree 기반으로 연다. BT 어댑터는 추가하지 않는다 |
| 통합 플러그인 이름 | 확정 | `KataFramework` |
| 입력 계층 위치 | 확정 | `KataFramework`의 PlayerController와 입력 매핑 |
| 태그 정의 위치 | 확정 | 2026-09-24 사용자 결정. 태그 정의와 생성 코드(`KataTag`)는 샘플 프로젝트에 남긴다. 게임별 데이터이므로 재사용 코드로 보지 않는다. 플러그인은 `KataTag`를 참조하지 않는다. [게임플레이 태그 구현 기록](../devlog/2026-09-24-Gameplay-Tag-Generation.md) |
| 타게팅 상태 소유 | 확정 | `UKataActionComponent`는 액션·그래프만 처리한다. 타게팅 상태는 `KataTargeting`의 컴포넌트가 소유한다 |
| PM-2 코어 확장 지점 | 확정 | 2026-09-24 사용자 결정. 아래 "PM-2 설계"의 결정 1~7. 앞서 제안한 수명 주기 객체형 "액션 훅"은 채택하지 않고 Pre·Post Command로 대체했다 |
| 카메라 기반 | 결정 필요 | GameplayCameras(Experimental) 또는 SpringArm과 자체 모드 스택 |
| 각 플러그인의 `CanContainContent`·버전 표기 | 제안 | 콘텐츠가 생길 때까지 `false`, 버전은 코어와 같은 0.1.0·Beta로 시작한다. `KataFramework`에 적용했다 |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| PM-0 | 높음 | 결정 기록과 AGENTS.md 수정(플러그인 구성, KataAI 범위, 프로젝트 샘플 전용) | 없음 | AGENTS.md와 이 문서가 결정과 일치한다 |
| PM-1 | 높음 | `KataFramework` 뼈대 생성, `AKataCharacter` 이동, `ClassRedirects` 추가 | PM-0 | 코어에 Character가 없고 기존 에셋이 유지된다 |
| PM-2 | 높음 | 코어 확장 지점: Pre·Post Command, PreCommands의 대상 변경, 그래프 Context 다시 기록, 엣지 대상 유지 토글 | PM-0 | 위성 없이 코어만으로 빌드되고 Command가 없는 기존 액션·그래프 동작이 유지된다 |
| PM-3 | 높음 | `KataTargeting` 플러그인 뼈대 | PM-2 | 코어에 의존하지 않고 로드된다. `Kata`·`TargetingSystem` 의존은 쓰는 코드가 생길 때 추가한다 |
| PM-4 | 높음 | 타게팅 기능 구현 | PM-3, [#13](https://github.com/jaykop/Kata/issues/13) 설계 | #13의 완료 조건 |
| PM-5 | 보통 | `KataAI` 플러그인: Perception, 어그로, `KataAIController`, StateTree Task | PM-4 | AI가 인지한 대상으로 Kata 액션을 실행한다 |
| PM-6 | 낮음 | `KataCamera` 플러그인 | 카메라 기반 결정 | 락온 화면 구성과 카메라 태스크가 동작한다 |

PM-1과 PM-2는 서로 독립적이다. `KataFramework`는 위성 플러그인이 생길 때마다 조합 대상을 늘린다.
PM-1 구현 내용은 [모듈 구조 진단](../devlog/2026-09-24-Module-Structure-Diagnosis.md)의 후속 기록에 있다.

## PM-2 설계

코어 `Kata`만 수정하며 타게팅·Pressure 같은 위성 기능의 개념을 코어에 넣지 않는다.

### A. Pre·Post Command

`UKataCommand`는 액션의 시작 또는 종료 시점에 한 번 실행하고 같은 프레임 안에 끝나는 로직의 기반 클래스다.
`UKataTask`를 상속하지 않으며 시간 필드와 인스턴스 객체가 없다. 지속 시간이나 유지되는 효과가 필요한 로직은 타임라인 태스크로 만든다.
Pre·Post 구분은 타입이 아니라 `UKataAction`의 목록이 정한다. 같은 Command 클래스를 양쪽 목록에 넣을 수 있다.

- `PreCommands`: 시작 조건을 통과한 뒤, GAS 활성 태그를 적용하고 타임라인보다 먼저 실행한다.
- `PostCommands`: 종료 시 타임라인 태스크를 정리한 뒤, GAS 활성 태그를 거두기 전에 실행한다. 항목마다 종료 사유 필터를 둔다.
- C++·Blueprint 클래스 상속으로 로직을 만든다(`Blueprintable`, BlueprintNativeEvent `Execute`).
- 액션 에셋 상속은 기존 고유 설정 규칙을 따라 목록 전체를 한 값으로 다룬다.

### B. 대상 변경과 그래프 Context 다시 기록

현재 `UKataActionInstance`의 Context는 읽기만 가능하고, `UKataGraphInstance`는 시작 때 받은 Context를 모든 노드에 넘긴다.
PreCommands가 실행되는 동안에만 `UKataActionInstance::SetTargetActor`로 대상을 바꿀 수 있게 하고,
그래프는 액션 시작 직후의 `TargetActor`를 그래프 Context에 다시 기록해 다음 노드가 이어받게 한다.

### C. 엣지 대상 유지 토글

`UKataEdge`에 대상 유지 토글을 추가한다. 기본값은 true다.

- true: 다음 노드에 그래프 Context의 `TargetActor`를 넘긴다. 대상이 파괴되어 무효가 되었으면 비워서 넘긴다.
- false: 이 전이로 들어가는 노드에는 `TargetActor`를 비워서 넘긴다.
- 진입점에서 나가는 엣지는 그래프 시작 때 받은 Context를 그대로 쓴다.
- 경유 노드를 거치는 전이는 경로의 첫 엣지 토글을 따른다.

### 결정

| # | 항목 | 결정 |
|---|---|---|
| 1 | 실행 길이 | Command는 한 프레임 안에 실행하고 끝난다. 지속·유지가 필요하면 타임라인 태스크를 쓴다 |
| 2 | Post 실행 조건 | 모든 종료 사유에서 실행하되 항목별 종료 사유 필터를 둔다. 기본값은 모든 사유다 |
| 3 | 편집 UI | Kata Action Details의 목록으로 제공한다. 타임라인 트랙 표시는 필요할 때 검토한다 |
| 4 | 대상 변경 권한 | PreCommands 실행 중에만 허용한다. 타임라인 도중 대상이 바뀌면 실행 중인 태스크와 어긋나기 때문이다 |
| 5 | 다시 기록하는 시점 | 액션 시작 직후, 즉 PreCommands 실행 뒤. 액션 도중 전이가 일어나도 반영된다 |
| 6 | 다시 기록하는 범위 | `TargetActor`만. Owner·Avatar·ASC는 그래프 실행 동안 바뀌지 않는다 |
| 7 | 대상 유지 토글 위치 | `UKataEdge`. 같은 노드라도 들어온 전이에 따라 유지 여부가 달라야 하기 때문이다. 노드 단위 요구는 들어오는 엣지를 모두 끄는 것으로 대신한다 |
| 8 | 이름과 타입 | 클래스 `UKataCommand`, 목록 `PreCommands`·`PostCommands`. Pre·Post를 타입으로 나누지 않는다. 앞서 논의한 태스크 기반 PreTask·PostTask는 채택하지 않았다 |

## 영향과 제한

- 직렬화된 에셋: `DefaultEngine.ini`의 `/Script/KataRuntime.KataCharacter` Redirect는 옛 경로를 참조하는 에셋이 남아 있을 때를 위해 유지한다.
- 병행 작업: PM-2는 `KataAction`, `KataResolvedAction`, `KataActionInstance`, `KataRuntimeTypes`, `KataPropertyOverride`,
  `KataEdge`, `KataGraphInstance`를 수정하고 `KataCommand`를 추가한다. [#2](https://github.com/jaykop/Kata/issues/2) T05(프리뷰 무기 부착)도 `KataAction.h`를 수정하므로 PM-2의 해당 변경이 커밋된 뒤 진행한다.
- 공개 API: 위성 플러그인이 쓰는 코어 타입(`FKataContext`, 액션 인스턴스 이벤트, `UKataCommand`)은 모듈 API로 공개해야 한다.
- 에셋 호환: Pre·Post Command 목록과 엣지 토글은 새 프로퍼티다. 기존 에셋은 빈 목록과 토글 기본값 true로 읽혀 지금과 같게 동작해야 한다.
- 싱글플레이 전용 원칙은 모든 플러그인에 그대로 적용한다.

## 사용자 확인 항목

- PM-2 이후 기존 액션·그래프 에셋이 열리고 전과 같게 재생되는지, Pre·Post Commands와 엣지 Keep Target이 Details에 보이는지 확인한다.
- 각 단계의 빌드는 사용자가 수행한다. 이 문서만으로 에이전트가 빌드·테스트를 수행하지 않는다.

## 완료 시 갱신할 문서

- [현재 구현 상태](../devlog/Implementation-Status.md): 플러그인 구성과 모듈 경계.
- [런타임 사용법](../manual/Runtime-Usage.md): 캐릭터 요구 조건과 `KataFramework` 사용법.
- [액션 게임 기반 시스템 계획](Action-Game-Systems-Plan.md): 타게팅·AI·카메라의 위치 결정 반영.
- [문서 목록](../README.md): 이 문서와 진단 기록 링크.
