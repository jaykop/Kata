# 모듈 구조와 AKataCharacter 진단

작성: 2026-09-24  
갱신: 2026-09-24  
유형: 진단, 결정 기록  
대상: Kata 플러그인 모듈 구성, `AKataCharacter`, 프로젝트 모듈 역할  
기준: `06ede58` 이후 작업 트리. 다른 세션의 미커밋 문서 변경을 포함하며 소스 변경은 확인 대상에서 제외했다

## 배경과 결론

타게팅·AI·카메라 같은 기반 시스템을 추가하기 전에 현재 모듈 구조가 적합한지 진단했다.
현재 의존 방향과 에디터·런타임 분리는 문제가 없다. 그러나 새 시스템이 요구하는 엔진 플러그인 의존을
코어 플러그인이 떠안게 되는 문제가 있고, `AKataCharacter`는 코어에 있는 한 다른 모듈의 컴포넌트를 가질 수 없다.
사용자는 모듈 단위가 아닌 플러그인 단위 분리, `AKataCharacter`의 통합 플러그인 이동, 프로젝트의 샘플 전용화를 결정했다.
실행 계획은 [플러그인 분리 모듈화 계획](../plan/Plugin-Modularization-Plan.md)에 있다. 이 기록 시점에 소스는 변경하지 않았다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 의존 방향 | `KataConditions` ← `KataRuntime` ← `KataGraph`, 에디터 모듈은 각 런타임 모듈을 참조한다. 순환 없음 | 유지 |
| `KataConditions` 분리 | 1.5k줄로 작지만 KataRuntime에 의존하지 않는다 | 유지. 타게팅 필터와 AI 판정이 액션 실행 없이 조건을 재사용할 수 있다 |
| `KataRuntime`의 관심사 | 액션 데이터, 실행, GAS 연결, 구체 태스크가 한 모듈에 있다. 이후 태스크는 Niagara·MotionWarping·GameplayCameras 등 외부 의존을 가져온다 | 외부 의존이 있는 태스크는 해당 시스템 플러그인에 둔다 |
| 플러그인 의존 | `Kata.uplugin`은 `GameplayAbilities`만 요구한다. 같은 플러그인에 타게팅 모듈을 추가하면 `TargetingSystem`(Beta)을 선언해야 한다 | 추가 엔진 플러그인 의존이 생기는 시스템은 별도 플러그인으로 분리 |
| `AKataCharacter` 참조 | C++ 참조 없음. 프리뷰는 `PreviewActorClass`로 임의 클래스를 받는다. `Content/KataTest`의 `BP_KataCharacter`, `ABP_KataCharacter`, `KA_Test`가 참조한다 | `KataFramework`로 이동하고 `ClassRedirects` 추가(미구현) |
| `AKataCharacter` 구성 | ASC와 `UKataComponent`만 가진다. `KataRuntime`이 `KataGraph`를 참조할 수 없어 `UKataGraphComponent`가 없다 | 통합 플러그인에서 모든 컴포넌트를 조합한다 |
| `KataTask_TransitionWindow` 위치 | 코어에 있는 그래프용 태스크처럼 보이지만, 열린 창 상태는 `UKataActionInstance`가 보관한다 | 유지. 그래프 없이도 쓰는 범용 개념이다 |
| AI 범위 | AGENTS.md가 KataAI와 BT/StateTree 어댑터를 제외했다 | StateTree 기반으로 범위를 열고 BT 어댑터는 계속 제외 |

## 주요 결정과 이유

- 플러그인 단위 분리(사용자 결정): 모듈만 나누면 `.uplugin` 의존은 그대로 남는다.
  기능을 쓰지 않는 사용자가 Beta·Experimental 엔진 플러그인을 켜야 하는 문제를 막으려면 플러그인을 나눠야 한다.
- `AKataCharacter`를 통합 플러그인 `KataFramework`로 이동(사용자 결정): 코어 캐릭터는 의존 방향 때문에 위성 컴포넌트를 가질 수 없다.
  코어에 캐릭터가 있으면 상속을 강제하는 인상을 주고 ASC 위치·이동·입력 같은 게임별 결정이 섞인다.
  검토한 대안은 코어에 최소 캐릭터로 남기는 방식과 프로젝트 모듈로 옮기는 방식이다. 사용자는 프로젝트를 샘플 전용으로 두기로 해서 통합 플러그인을 택했다.
- 프로젝트 샘플 전용(사용자 결정): 재사용할 코드는 모두 플러그인에 두고 `ProjectKata`는 사용 예제만 담는다.
- KataAI 범위 개방(사용자 확인): KataAction을 실행하는 StateTree Task가 필요하다. AI 프레임워크를 StateTree로 정하고 BT 어댑터는 추가하지 않는다.
- 타게팅 상태 소유(사용자 결정): `UKataComponent`는 액션과 그래프 처리만 담당한다. 타게팅은 `KataTargeting`의 전용 컴포넌트가 소유한다.
- 액션 훅(제안): 그래프 전이가 `UKataGraphInstance`에서 `PlayKataAction`을 직접 호출하므로, 호출자가 미리 대상을 구하는 방식으로는
  콤보 전이 시점을 처리할 수 없다. 액션 시작 경로 안에서 불리는 확장 지점이 필요하다. 세부 설계는 타게팅 계획에서 확정한다.

## 근거

- [Kata.uplugin](../../Plugins/Kata/Kata.uplugin): 모듈 다섯 개와 `GameplayAbilities` 의존.
- 각 모듈의 `Build.cs`: 의존 목록. `KataConditions`는 GAS만, `KataGraph`는 `KataRuntime`·`KataConditions`에 의존한다.
- `KataRuntime/Public/Character/KataCharacter.h`(진단 당시 위치, 현재 [KataFramework](../../Plugins/KataFramework/Source/KataFramework/Public/Character/KataCharacter.h)): ASC와 `UKataComponent`만 소유한다.
- [KataGraphInstance.h](../../Plugins/Kata/Source/KataGraph/Public/KataGraphInstance.h): 그래프 실행 동안 `Context`를 한 번 받아 모든 노드에 사용한다.
- [DefaultEngine.ini](../../Config/DefaultEngine.ini): 테스트 클래스 이동 때 사용한 `ClassRedirects` 선례.
- UE 5.8 설치 폴더의 `.uplugin`: `TargetingSystem` Beta(`Plugins/Experimental`), `StateTree`·`GameplayStateTree` Beta 아님,
  `GameplayCameras` Experimental 0.1. `GenericTeamAgentInterface.h`는 `AIModule`에 있다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 모듈 의존과 규모 | 소스·`Build.cs` 읽기, 에이전트 | 위 표와 같다 | Public·Private 의존 구분의 세부 적정성 |
| `AKataCharacter` 참조 | 소스 검색과 `Content`·`Config` 바이너리 문자열 검색, 에이전트 | 테스트 에셋 3개만 참조한다 | 에디터에서 실제 참조 관계 확인 |
| 엔진 플러그인 상태 | UE 5.8 `.uplugin` 읽기, 에이전트 | 위 근거와 같다 | 각 플러그인 API의 실제 사용성 |

빌드와 실행 확인은 수행하지 않았다.

## 남은 제한과 후속 작업

- 분리 작업은 [플러그인 분리 모듈화 계획](../plan/Plugin-Modularization-Plan.md)의 PM-1부터 진행한다.
- 액션 훅과 타게팅 세부 설계는 `Targeting-Plan.md`로 정리해야 한다.
  → 2026-09-24 후속: 액션 훅은 채택하지 않고 PreTask·PostTask로 대체했다([계획의 PM-2 설계](../plan/Plugin-Modularization-Plan.md#pm-2-설계)).
  타게팅은 [#13](https://github.com/jaykop/Kata/issues/13)에서 다룬다.
- 카메라 기반과 각 플러그인의 `CanContainContent` 설정은 결정이 필요하다.

## 후속 기록: PM-1 구현 (2026-09-24)

- `Plugins/KataFramework`를 만들었다. `KataFramework.uplugin`은 `Kata`·`GameplayAbilities`에 의존하고 `CanContainContent`는 `false`다.
  Runtime 모듈 `KataFramework`는 Core·CoreUObject·Engine·GameplayAbilities·KataRuntime을 Public으로 의존한다.
- `AKataCharacter`를 `KataRuntime`에서 `KataFramework`로 옮기고 API 매크로만 `KATAFRAMEWORK_API`로 바꿨다(커밋 `9c620a6`).
- `ProjectKata.uproject`에서 `KataFramework`를 활성화하고 `DefaultEngine.ini`에
  `/Script/KataRuntime.KataCharacter` → `/Script/KataFramework.KataCharacter` Redirect를 추가했다.
- 2026-09-24 사용자가 빌드 성공을 확인했다. 사용자는 기존 테스트 캐릭터 Blueprint를 지우고 샘플 캐릭터 Blueprint를 새로 만들었다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [AGENTS.md](../../AGENTS.md) | 플러그인 분리 결정, KataAI 범위, 프로젝트 샘플 전용 규칙 반영 |
| [현재 구현 상태](Implementation-Status.md) | PM-1 이후 `AKataCharacter` 위치와 사용자 빌드 확인을 반영했다 |
| [플러그인 분리 모듈화 계획](../plan/Plugin-Modularization-Plan.md) | 신규 작성 |
| [액션 게임 기반 시스템 계획](../plan/Action-Game-Systems-Plan.md) | 타게팅·퍼셉션 위치 결정 반영 |
| [문서 목록](../README.md) | 두 문서 링크 추가 |
