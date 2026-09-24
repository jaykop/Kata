# 플러그인 분리 모듈화 계획

작성: 2026-09-24  
갱신: 2026-09-24  
문서 상태: 확정  
현재 상태 근거: [현재 구현 상태](../devlog/Implementation-Status.md) · [모듈 구조 진단](../devlog/2026-09-24-Module-Structure-Diagnosis.md)  
대체 관계: 없음

## 목적과 현재 상태

타게팅, AI, 카메라 같은 기반 시스템([액션 게임 기반 시스템 계획](Action-Game-Systems-Plan.md))을 추가하면
Beta·Experimental 엔진 플러그인 의존이 함께 생긴다. 현재 `Kata` 플러그인 하나에 모듈만 추가하면
`Kata.uplugin`에 그 의존을 선언해야 하므로, 해당 기능을 쓰지 않는 사용자도 그 플러그인을 켜야 한다.
또한 코어 `KataRuntime`에 있는 `AKataCharacter`는 의존 방향 때문에 `UKataGraphComponent`나 이후 위성 플러그인의
컴포넌트를 가질 수 없다. 진단 내용은 [모듈 구조 진단](../devlog/2026-09-24-Module-Structure-Diagnosis.md)에 있다.

현재 구조는 `Kata` 플러그인 하나(`KataConditions`, `KataRuntime`, `KataGraph`, `KataEditor`, `KataGraphEditor`)와
프로젝트 모듈 `ProjectKata`, `ProjectKataTesting`이다. 이 계획의 항목은 아직 구현하지 않았다.

## 범위

- 포함: 플러그인 구성과 의존 방향, 코어가 제공할 확장 지점, `AKataCharacter` 이동, 새 플러그인 뼈대, 지침 문서 수정.
- 제외: 각 위성 플러그인의 기능 상세 설계. 타게팅은 별도 `Targeting-Plan.md`에서 다룬다.
  BT(Behavior Tree) 어댑터는 추가하지 않는다.

## 목표 구조

```
Plugins/
  Kata/             코어              의존: GameplayAbilities
    KataConditions  조건, Context
    KataRuntime     액션, 실행, GAS 연결, 외부 의존 없는 태스크, 액션 훅 기반 클래스
    KataGraph       그래프, 콤보 전이
    KataEditor / KataGraphEditor
  KataTargeting/    타게팅 컴포넌트, Preset 커스텀 태스크, 팩션, Capability·Pressure
                    의존: Kata, TargetingSystem(Beta), AIModule(엔진 모듈)
  KataAI/           Perception 연결, 대상별 어그로, KataAIController, StateTree Task
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
   코어가 요구하는 것은 "ASC와 `UKataComponent`를 가진 액터"라는 계약뿐이다.
2. 추가 엔진 플러그인 의존이 생기거나, 끄고 켤 수 있어야 하는 기능 묶음이면 별도 플러그인으로 만든다.
3. 의존은 위성 → 코어 한 방향이다. 코어는 위성을 알지 못하며 확장 지점(액션 훅 기반 클래스, 인터페이스, 델리게이트)만 제공한다.
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
| 타게팅 상태 소유 | 확정 | `UKataComponent`는 액션·그래프만 처리한다. 타게팅 상태는 `KataTargeting`의 컴포넌트가 소유한다 |
| 액션 훅 | 제안 | 시간 개념이 없는 수명 주기 객체. `UKataTask`를 상속하지 않으며 한 객체가 ResolveContext → CanStart → OnStarted → OnEnded를 가진다 |
| 그래프 대상 유지 옵션 | 확정 | `UKataGraph` 옵션, 기본값 true. 액션 훅이 구한 대상을 그래프 Context에 다시 기록한다 |
| 카메라 기반 | 결정 필요 | GameplayCameras(Experimental) 또는 SpringArm과 자체 모드 스택 |
| 각 플러그인의 `CanContainContent`·버전 표기 | 결정 필요 | 뼈대를 만들 때 정한다 |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 상태 | 선행 조건 | 완료 조건 | 결과 기록 |
|---|---|---|---|---|---|---|
| PM-0 | 높음 | 결정 기록과 AGENTS.md 수정(플러그인 구성, KataAI 범위, 프로젝트 샘플 전용) | 구현 완료·검증 미실시 | 없음 | AGENTS.md와 이 문서가 결정과 일치한다 | [모듈 구조 진단](../devlog/2026-09-24-Module-Structure-Diagnosis.md) |
| PM-1 | 높음 | `KataFramework` 뼈대 생성, `AKataCharacter` 이동, `ClassRedirects` 추가 | 미착수 | PM-0 | 코어에 Character가 없고 `BP_KataCharacter`·`KA_Test`가 Redirect로 유지된다 | 미완료 |
| PM-2 | 높음 | 코어 확장 지점: 액션 훅 기반 클래스, 훅 거부 시작 결과, 그래프 Context 다시 기록과 대상 유지 옵션 | 미착수 | PM-0 | 위성 없이 코어만으로 빌드되고 기존 액션·그래프 동작이 유지된다 | 미완료 |
| PM-3 | 높음 | `KataTargeting` 플러그인 뼈대 | 미착수 | PM-2 | 코어와 `TargetingSystem` 의존만으로 로드된다 | 미완료 |
| PM-4 | 높음 | 타게팅 기능 구현 | 미착수 | PM-3, `Targeting-Plan.md` | `Targeting-Plan.md`의 완료 조건 | 미완료 |
| PM-5 | 보통 | `KataAI` 플러그인: Perception, 어그로, `KataAIController`, StateTree Task | 미착수 | PM-4 | AI가 인지한 대상으로 Kata 액션을 실행한다 | 미완료 |
| PM-6 | 낮음 | `KataCamera` 플러그인 | 미착수 | 카메라 기반 결정 | 락온 화면 구성과 카메라 태스크가 동작한다 | 미완료 |

PM-1과 PM-2는 서로 독립적이다. `KataFramework`는 위성 플러그인이 생길 때마다 조합 대상을 늘린다.

## 영향과 제한

- 직렬화된 에셋: `AKataCharacter`를 옮기면 스크립트 경로가 `/Script/KataRuntime.KataCharacter`에서
  `KataFramework` 모듈 경로로 바뀐다. `Content/KataTest`의 `BP_KataCharacter`, `ABP_KataCharacter`, `KA_Test`가 이 클래스를 참조한다.
  `DefaultEngine.ini`에 `ClassRedirects`를 추가하고 에디터에서 에셋을 다시 저장해야 한다.
- 다른 작업과의 충돌: 2026-09-24 사용자 통보로 KataRuntime 태스크 작업 세션은 이 분리 작업 동안 일시 중지했다.
  작업 트리에 남은 그 세션의 미커밋 변경은 보존한다. PM-2는 `UKataComponent`, `UKataActionInstance`, `UKataGraphInstance`를 수정한다.
- 공개 API: 위성 플러그인이 쓰는 코어 타입(`FKataContext`, 액션 인스턴스 이벤트, 훅 기반 클래스)은 모듈 API로 공개해야 한다.
- 싱글플레이 전용 원칙은 모든 플러그인에 그대로 적용한다.

## 사용자 확인 항목

- PM-1 이후 에디터에서 테스트 에셋이 정상으로 열리는지 확인하고 다시 저장한다.
- 각 단계의 빌드는 사용자가 수행한다. 이 문서만으로 에이전트가 빌드·테스트를 수행하지 않는다.

## 완료 시 갱신할 문서

- [현재 구현 상태](../devlog/Implementation-Status.md): 플러그인 구성과 모듈 경계.
- [런타임 사용법](../manual/Runtime-Usage.md): 캐릭터 요구 조건과 `KataFramework` 사용법.
- [액션 게임 기반 시스템 계획](Action-Game-Systems-Plan.md): 타게팅·AI·카메라의 위치 결정 반영.
- [문서 목록](../README.md): 이 문서와 진단 기록 링크.
