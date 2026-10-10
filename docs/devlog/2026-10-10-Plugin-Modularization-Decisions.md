# 플러그인 분리 모듈화 결정 기록

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 결정 기록  
대상: 플러그인 구성(코어 `Kata`, 위성 `KataTargeting`·`KataAI`·`KataCamera`, 통합 `KataFramework`), 코어 확장 지점 `UKataCommand`·Keep Target  
기준: [#1](https://github.com/jaykop/Kata/issues/1) 종료 시점(b95b6b8). 삭제한 `docs/plan/Plugin-Modularization-Plan.md`(2026-09-24 작성, 2026-10-10 갱신)의 결정을 옮긴 기록. 작업 트리의 다른 미커밋 소스 변경은 확인 대상에서 제외했다

## 배경과 결론

타게팅·AI·카메라 같은 기반 시스템은 Beta·Experimental 엔진 플러그인 의존을 가져온다. 이 기능들을 `Kata` 플러그인 하나에 모듈로만 추가하면
`Kata.uplugin`에 그 의존을 선언해야 하므로 기능을 쓰지 않는 사용자도 해당 엔진 플러그인을 켜야 한다.
또한 코어에 있던 `AKataCharacter`는 의존 방향 때문에 그래프나 위성 플러그인의 컴포넌트를 가질 수 없었다. 진단은 [모듈 구조 진단](2026-09-24-Module-Structure-Diagnosis.md)에 있다.

#1에서 PM-0~PM-3을 구현했다. PM-4(타게팅 기능)는 [#13](https://github.com/jaykop/Kata/issues/13)이 맡아 완료했고,
PM-5(KataAI)는 [#22](https://github.com/jaykop/Kata/issues/22), PM-6(KataCamera)은 [#20](https://github.com/jaykop/Kata/issues/20)으로 넘겼다.
사용자가 PM-2의 빌드·Details 표시와 런타임 동작을 확인해 이슈를 닫았다. 계획 문서의 결정과 이유를 이 기록으로 옮기고 plan은 삭제했다.
현재 모듈 경계 규칙은 [AGENTS.md](../../AGENTS.md)의 "프로젝트와 현재 범위"·"모듈 경계", Command·Keep Target 사용법은 [런타임 사용법](../manual/Runtime-Usage.md)을 따른다.

## 종료 시점의 구성

| 플러그인 | 모듈 | `.uplugin` 플러그인 의존 | 역할 |
|---|---|---|---|
| `Kata`(코어) | `KataConditions`, `KataRuntime`, `KataGraph`, `KataEditor`, `KataGraphEditor` | `GameplayAbilities` | 조건, 액션·실행·GAS 연결, 외부 의존 없는 태스크, `UKataCommand`, 그래프 |
| `KataTargeting` | `KataTargeting` | `Kata`, `TargetingSystem`(Beta), `GameplayAbilities` | 팩션, 타게팅 컴포넌트, Preset 확장 태스크, 대상·방향 Command. `AIModule`은 엔진 모듈로 참조한다 |
| `KataAI` | `KataAI` | `Kata`, `KataTargeting`, `TargetingSystem`, `StateTree`, `GameplayStateTree`, `GameplayAbilities` | Perception, AI 타게팅, StateTree Task·Evaluator |
| `KataCamera` | `KataCamera` | `StateTree`, `GameplayAbilities` | 카메라 매니저와 파이프라인. 코어 `Kata`를 참조하지 않는다 |
| `KataFramework`(통합) | `KataFramework`, `KataFrameworkEditor` | 위 플러그인 전부와 `EnhancedInput`, `StateTree`, `TargetingSystem`, `GameplayAbilities` | `AKataCharacter`·`AKataPlayerCharacter`·`AKataPlayerController` 등 조합 클래스 |

게임플레이 플러그인과 별개로 개발 도구 `KataGASInspector`(Editor 모듈 하나, `GameplayAbilities`만 의존)를 두며, 게임플레이 플러그인은 이 도구를 참조하지 않는다.
`ProjectKata`·`ProjectKataEditor`는 샘플 전용이다. 새 위성 플러그인의 `CanContainContent`는 `false`, 버전은 0.1.0·Beta로 시작했다. 이 값은 계획에서 제안으로 적용했으며 별도 확정 기록은 없다.

## 주요 결정과 이유

| 결정 | 일자 | 이유·근거 |
|---|---|---|
| 모듈이 아닌 플러그인 단위로 분리한다 | 2026-09-24 | 모듈만 나누면 `.uplugin` 의존이 그대로 남는다. 사용자 결정 |
| 플러그인 간 의존은 위성·통합 → 코어 한 방향이다. 코어는 엔진과 GAS에만 의존하고 확장 지점(기반 클래스, 인터페이스, 델리게이트)만 제공한다 | 2026-09-24 | 코어를 위성 기능 없이 단독으로 쓸 수 있게 한다 |
| 위성 간 의존은 `KataAI` → `KataTargeting`만 허용한다. 여러 플러그인의 조합은 `KataFramework`가 맡는다 | 2026-09-24 | 몬스터 타게팅 컴포넌트가 `KataTargeting`의 공용 기반을 상속한다. 그 밖의 연결은 통합 플러그인에서 해 순환을 막는다 |
| `AKataCharacter`를 통합 플러그인 `KataFramework`로 옮긴다 | 2026-09-24 | 코어 캐릭터는 위성 컴포넌트를 가질 수 없고, 코어에 두면 상속을 강제하는 인상을 준다. 사용자 결정 |
| `ProjectKata`는 샘플 전용이며 재사용 코드를 두지 않는다 | 2026-09-24 | 재사용 코드는 모두 플러그인에 둔다. 사용자 결정 |
| `KataAI`는 StateTree 기반이며 BT 어댑터를 추가하지 않는다 | 2026-09-24 | KataAction을 실행하는 StateTree Task가 필요하다. 사용자 확인 |
| 태그 정의와 생성 코드(`KataTag`)는 샘플 프로젝트에 남기고 플러그인은 참조하지 않는다 | 2026-09-24 | 게임별 데이터다. [게임플레이 태그 구현 기록](2026-09-24-Gameplay-Tag-Generation.md) |
| `UKataActionComponent`는 액션·그래프만 처리하고 타게팅 상태는 `KataTargeting` 컴포넌트가 소유한다 | 2026-09-24 | 사용자 결정. 세부는 [타게팅 결정 기록](2026-10-10-Targeting-Decisions.md) |
| 외부 의존이 없는 태스크는 코어에, 외부 의존이 있는 태스크(Motion Warping, 카메라, Niagara 등)는 해당 시스템 플러그인에 둔다. Editor 모듈은 편집 UI가 실제로 필요할 때만 추가한다 | 2026-09-24 | 계획 작성 시 정한 분리 원칙 |
| 입력 계층은 `KataFramework`에 둔다. 바인딩은 `AKataPlayerCharacter`, 플레이어 단위 기능은 `AKataPlayerController`다 | 2026-09-26 | [입력 계층 결정](2026-10-10-Input-Layer.md) |
| 카메라는 자체 구현하며 `GameplayCameras`에 의존하지 않는다 | 2026-09-27 | 5.8에서 Experimental 0.1이라 장기 유지보수를 보장할 수 없다. 사용자 결정, [카메라 시스템 계획](../plan/Camera-Plan.md) |
| `KataGASInspector`는 게임플레이 플러그인의 의존 대상이 아닌 별도 Editor 도구로 둔다 | 2026-10-05 | [GAS Inspector 기록](2026-10-05-GAS-Inspector.md) |
| 전용 테스트 모듈을 제거한다 | 2026-10-05 | 입력 실행 경로로 확인할 수 있게 됐다. [제거 기록](2026-10-05-Testing-Module-Removal.md) |

## PM-2 코어 확장 지점

코어 `Kata`만 수정했고 타게팅·Pressure 같은 위성 개념은 코어에 넣지 않았다. 2026-09-24 사용자가 아래 결정을 확정했다(설계 05c48a8, 구현 e52c2c0).

| # | 항목 | 결정과 이유 |
|---|---|---|
| 1 | 실행 길이 | `UKataCommand`는 액션 시작·종료 시점에 한 번 실행하고 같은 프레임 안에 끝난다. 지속·유지가 필요하면 타임라인 태스크를 쓴다 |
| 2 | Post 실행 조건 | 모든 종료 사유에서 실행하되 항목별 종료 사유 필터를 둔다. 기본값은 모든 사유다 |
| 3 | 편집 UI | Kata Action Details의 목록으로 제공한다. 타임라인 트랙 표시는 필요할 때 검토한다 |
| 4 | 대상 변경 권한 | PreCommands 실행 중에만 허용한다. 타임라인 도중 대상이 바뀌면 실행 중인 태스크와 어긋난다 |
| 5 | 그래프 Context 다시 기록 시점 | 액션 시작 직후, 즉 PreCommands 실행 뒤다. 액션 도중 전이가 일어나도 반영된다 |
| 6 | 다시 기록하는 범위 | `TargetActor`만. Owner·Avatar·ASC는 그래프 실행 동안 바뀌지 않는다 |
| 7 | 대상 유지 토글 위치 | `UKataEdge`(기본값 true). 같은 노드라도 들어온 전이에 따라 유지 여부가 달라야 한다. 노드 단위 요구는 들어오는 엣지를 모두 끄는 것으로 대신한다. 진입점에서 나가는 엣지는 시작 Context를 그대로 쓰고, 경유 노드를 거치는 전이는 경로의 첫 엣지 토글을 따른다 |
| 8 | 이름과 타입 | 클래스 `UKataCommand`, 목록 `PreCommands`·`PostCommands`. Pre·Post는 타입이 아니라 `UKataAction`의 목록이 정하며 같은 클래스를 양쪽에 넣을 수 있다 |

채택하지 않은 안:

- 수명 주기 객체형 "액션 훅": 진단 단계의 제안이었다. 사용자가 의도한 것은 그 시점에 한 번 실행하고 끝나는 로직이므로 Pre·Post Command로 대체했다.
- 태스크 기반 PreTask·PostTask: `UKataTask`를 상속하면 시간 필드와 인스턴스 객체를 갖게 된다. `UKataCommand`는 `UKataTask`를 상속하지 않는다.

## 계획과 달라진 점

| 항목 | 계획 | 결과 |
|---|---|---|
| `KataCamera` 의존 | 코어 `Kata`에 의존 | 코어를 참조하지 않는다. 락온 연결은 `KataFramework`가 `OnLockTargetChanged`를 구독해 카메라 초점 API에 넘긴다([카메라 시스템 계획](../plan/Camera-Plan.md)) |
| PM-3 완료 조건 | 뼈대는 코어에 의존하지 않고 로드되며, `Kata`·`TargetingSystem` 의존은 쓰는 코드가 생길 때 추가 | #13 구현에서 두 의존을 추가했다 |
| PM-4~PM-6 | #1의 단계 | PM-4는 #13, PM-5는 #22, PM-6은 #20이 추적한다 |
| 구체 Command | 제공하는 Command 없음 | `KataTargeting`이 `UKataCommand_ResolveTarget`·`UKataCommand_ResolveFacing`·`UKataCommand_FaceMoveDirection`을 제공한다 |

## 근거

- 각 플러그인의 `.uplugin`과 `Build.cs`: 위 구성 표의 의존 목록.
- [KataCommand.h](../../Plugins/Kata/Source/KataRuntime/Public/Action/KataCommand.h), [KataEdge.h](../../Plugins/Kata/Source/KataGraph/Public/KataEdge.h): PM-2 확장 지점.
- [DefaultEngine.ini](../../Config/DefaultEngine.ini): `/Script/KataRuntime.KataCharacter` → `/Script/KataFramework.KataCharacter` ClassRedirect.
- UE 5.8 엔진 플러그인 상태(2026-09-24 확인): `TargetingSystem` Beta, `StateTree`·`GameplayStateTree` Beta 아님, `GameplayCameras` Experimental 0.1.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| PM-1 `KataFramework` 신설·`AKataCharacter` 이동 | 사용자 빌드, 2026-09-24 | 빌드 통과. 사용자가 샘플 캐릭터 Blueprint를 새로 만들었다 | 없음 |
| PM-2 빌드와 Details 표시 | 사용자 빌드·에디터 확인. 일자 미상(2026-09-26 이전) | Pre/Post Commands와 Keep Target이 Details에 보인다 | 없음 |
| PM-2 런타임 동작 | 사용자 실행 확인. 2026-10-10 사용자가 이전에 확인했다고 알렸으며 확인 일자는 미상 | PreCommands·PostCommands 실행, 그래프 대상 전달, Keep Target 해제 동작 | 없음 |
| 플러그인 간 참조 방향 | 각 플러그인 소스의 `#include` 대상 소유 플러그인 대조, 에이전트, 2026-10-10 | 허용 방향 밖의 참조 0건, 코어의 Character·Controller 클래스와 `KataTag` 참조 없음 | 빌드는 실행하지 않았다 |

## 남은 제한과 후속 작업

- `KataAI`(PM-5)는 #22, `KataCamera`(PM-6)는 #20에서 진행한다. 진행 순서는 [#24 로드맵](https://github.com/jaykop/Kata/issues/24)을 따른다.
- 옛 경로를 참조하는 에셋이 남아 있을 수 있으므로 `KataCharacter` ClassRedirect는 유지한다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#1](https://github.com/jaykop/Kata/issues/1) | PM-2 런타임 확인 체크와 범위 갱신 후 종료. 게시는 사용자 확인 후 |
| [AGENTS.md](../../AGENTS.md) | 삭제한 plan 대신 이 기록을 가리킨다 |
| [런타임 사용법](../manual/Runtime-Usage.md) | "제공하는 구체 Command는 아직 없다"를 KataTargeting 제공 Command로 갱신 |
| [모듈 구조 진단](2026-09-24-Module-Structure-Diagnosis.md) 등 과거 devlog | plan 링크를 이 기록으로 바꾸고 당시 문장은 보존 |
| [액션 게임 기반 시스템 계획](../plan/Action-Game-Systems-Plan.md), [카메라 시스템 계획](../plan/Camera-Plan.md), [캐릭터 데이터 계획](../plan/Character-Definition-Plan.md) | plan 링크를 이 기록으로 변경 |
| [문서 목록](../README.md) | plan 항목을 제거하고 이 기록을 추가 |
