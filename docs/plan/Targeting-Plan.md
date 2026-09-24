# 타게팅 시스템 설계

작성: 2026-09-24  
갱신: 2026-09-24  
연결 이슈: [#13 타게팅 시스템 (KataTargeting)](https://github.com/jaykop/Kata/issues/13)  
현재 상태 근거: [현재 구현 상태](../devlog/Implementation-Status.md) · [플러그인 분리 모듈화 계획](Plugin-Modularization-Plan.md)  
대체 관계: 없음

## 목적과 현재 상태

PC와 몬스터가 같은 방식으로 대상 후보를 고르고, 고른 대상을 Kata 액션의 `FKataContext::TargetActor`로 넘기는
타게팅 시스템을 `KataTargeting` 플러그인에 만든다. 팩션도 이 플러그인이 관리한다.

현재 타게팅 코드는 없다. 코어에는 이 설계가 쓰는 확장 지점이 있다.

- `UKataCommand`와 `PreCommands`: 액션 시작 때 타임라인보다 먼저 한 번 실행한다. PreCommands 안에서만
  `UKataActionInstance::SetTargetActor`로 대상을 바꿀 수 있다.
- `UKataEdge::bKeepTarget`: 콤보 전이에서 대상을 넘길지 정한다. 그래프는 액션 시작 직후의 대상을 다시 기록한다.

## 범위

- 포함: 타게팅 컴포넌트, 엔진 `UTargetingPreset` 기반 후보 선택과 확장 태스크, PC 소프트 타겟과 락온,
  대상을 정하는 Command, 팩션과 `UKataFL_Faction`.
- 제외:
  - Pressure·Capability: AI만 쓰는 개념이므로 KataAI(PM-5)에서 다룬다. 2026-09-24 사용자 결정.
  - 인지 기록·어그로·AIController·StateTree Task: KataAI.
  - 락온 카메라 화면 구성: KataCamera.
  - 입력을 락온·전환 호출로 잇는 부분: KataFramework 입력 계층.
- 선행: [#1](https://github.com/jaykop/Kata/issues/1) PM-3(`KataTargeting` 플러그인 뼈대).

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 후보 선택 | 확정 | PC와 몬스터 모두 `UTargetingPreset`으로 후보를 모으고 거르고 정렬한다 |
| PC 대상 종류 | 확정 | 소프트 타겟과 락온. 락온이 켜져 있으면 락온 대상이 우선한다 |
| 소프트 타겟 결정 시점 | 확정 | 2026-09-24 사용자 결정. PreCommand가 실행 주체의 타게팅 컴포넌트를 통해 소프트 타겟을 갱신하고 대상을 정한다 |
| 대상 재사용 옵션 | 확정 | 2026-09-24 사용자 결정. 대상을 정하는 Command에 "대상이 유효하면 다시 구하지 않음" 옵션을 두고 기본값을 켠다 |
| 락온 전환 | 확정 | 2026-09-24 사용자 결정. 왼쪽·오른쪽 전환을 각각의 입력 액션으로 호출하고, 방향별 전환 Preset의 후보 중 점수가 가장 높은 대상으로 확정한다 |
| 팩션 | 확정 | 2026-09-24 사용자 결정. Gameplay Tag 기반 Kata 팩션과 관계표를 두고 엔진 팀 인터페이스로 연결한다 |
| 팩션 판정 함수 | 확정 | 2026-09-24 사용자 결정. KataTargeting 플러그인의 `UKataFL_Faction` |
| 타게팅 상태 소유 | 확정 | `UKataComponent`가 아니라 전용 타게팅 컴포넌트가 소유한다 |
| Pressure 위치 | 확정 | 2026-09-24 사용자 결정. #13에서 제외하고 KataAI에서 다룬다 |
| 입력 방향 | 확정 | 2026-09-24 사용자 결정. 스틱 입력 방향을 타게팅 계산에 쓰지 않는다. 전환 방향은 호출하는 함수가 정한다 |
| 첫 번째 플레이어 조회 함수 | 확정 | 2026-09-24 사용자 결정. 만들지 않는다. 엔진 `Get Player Controller`·`Get Player Character`(Player Index 0)를 쓴다 |
| 팩션 관계 기본값 | 확정 | 2026-09-24 사용자 결정. 관계표에 없는 두 팩션은 중립이다. 적대 관계는 명시해야 생긴다 |
| 팀 번호 부여 | 확정 | 2026-09-24 사용자 결정. 관계표 설정에 등록한 팩션 순서대로 0부터 자동 부여한다. 255개를 넘으면 설정 검증에서 경고한다 |

## 설계

### 타게팅 컴포넌트 `UKataTargetingComponent`

실행 주체 액터에 붙는다. 캐릭터 클래스에 묶지 않으며 KataFramework의 캐릭터가 기본으로 가진다.

- 상태: `SoftTarget`, `LockTarget`. 실행 상태이므로 에셋에 저장하지 않는다.
- 설정: 소프트 타겟용 Preset, 락온용 Preset, 왼쪽·오른쪽 전환용 Preset, 팩션 태그.
- 대상 조회: `GetCurrentTarget()`은 유효한 `LockTarget`이 있으면 그것을, 없으면 `SoftTarget`을 돌려준다.
- 소프트 타겟: `UpdateSoftTarget()`이 소프트 타겟 Preset을 즉시 실행해 갱신한다.
- 락온: `AcquireLock()`, `SwitchLockLeft()`, `SwitchLockRight()`, `ReleaseLock()`. 전환 함수는 방향별 Preset의 후보 중
  점수가 가장 높은 대상으로 바꾸고, 후보가 없으면 현재 대상을 유지한다. 락온 대상이 파괴되거나 유효하지 않게 되면 해제 또는
  자동 전환한다. 어느 쪽으로 할지는 컴포넌트 설정으로 둔다.
- AI도 같은 컴포넌트를 쓴다. AI가 고른 대상은 `LockTarget` 자리를 쓴다. 누가 이 값을 정하는지는 KataAI가 다룬다.

### 대상을 정하는 Command

KataTargeting이 `UKataCommand` 파생 클래스를 제공한다. 액션의 `PreCommands`에 넣는다.

1. 실행 주체에서 `UKataTargetingComponent`를 찾는다. 없으면 아무것도 하지 않는다.
2. "대상이 유효하면 다시 구하지 않음"(기본값 켬)이 켜져 있고 이어받은 대상이 유효하면 끝낸다.
   콤보 전이의 Keep Target과 뜻을 맞추기 위한 옵션이다.
3. 컴포넌트의 `UpdateSoftTarget()`을 호출한 뒤 `GetCurrentTarget()`을 `Instance->SetTargetActor`로 넘긴다.

제한: 시작 조건은 PreCommands보다 먼저 평가된다. 대상을 읽는 시작 조건(예: 거리 조건)은 새로 구한 대상이 아니라
이어받은 대상으로 판정한다.

### Preset과 확장 태스크

엔진 `TargetingSystem`(UE 5.8에서 Beta, `Plugins/Experimental`)을 쓴다. 엔진 동작에서 확인한 제약과 대응은 다음과 같다.

- 정렬 점수: 각 정렬 태스크가 원점수를 자기 최고점으로 나눠 0~1로 정규화한 뒤 누적한다. 태스크별 가중치가 없으므로
  가중치를 가진 정렬 기반 클래스를 만든다.
- 필요한 확장 태스크: 팩션 필터(적대만), 화면 중심 정렬, 현재 락온 대상 기준 좌·우 필터.
  좌·우 필터는 방향을 태스크 설정값으로 가지므로 왼쪽용과 오른쪽용 Preset을 따로 만든다.
- 요청 입력: `FTargetingSourceContext`의 SourceActor로 실행 주체를 넘긴다. 좌·우 필터는 실행 주체의 타게팅 컴포넌트에서
  현재 락온 대상을 읽는다. 스틱 입력 방향은 쓰지 않으므로 요청에 추가 데이터를 싣지 않는다.
- 액션 시작 시점에는 즉시 실행 요청을 쓴다.

### 팩션

- 팩션은 Gameplay Tag로 표현한다(예: `Faction.Player`, `Faction.Monster.Undead`). 태그 정의는 샘플 프로젝트가 소유하고
  플러그인은 임의의 `FGameplayTag`를 받는다.
- 관계표는 KataTargeting의 프로젝트 설정(Developer Settings)에 데이터로 둔다. 태그 계층을 따라 부모 태그 규칙을 적용한다.
- 팩션은 타게팅 컴포넌트의 설정값으로 할당한다.
- 엔진 연결: 팩션 태그마다 `FGenericTeamId`를 부여하고, 관계 판정 함수(`FGenericTeamId::SetAttitudeSolver`)가 관계표를 읽게 한다.
  `IGenericTeamAgentInterface`는 액터나 컨트롤러가 구현해야 하므로 KataFramework의 캐릭터와 AIController가 구현하고 컴포넌트 값을 전달한다.
  이렇게 하면 AIPerception의 적·아군 구분도 Kata 팩션을 따른다.

### `UKataFL_Faction`

KataTargeting 플러그인의 Blueprint 함수 라이브러리. 두 액터를 받아 관계를 판정한다.

- `IsFriendly(A, B)`, `IsNeutral(A, B)`, `IsHostile(A, B)`.
- 엔진 `FGenericTeamId::GetAttitude(A, B)`를 감싼다. Kata 컴포넌트가 없어도 엔진 팀 인터페이스를 구현한 액터는 같은 규칙으로 판정된다.

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| TG-2 | 높음 | 팩션: 관계표 설정, 팀 번호 연결, `UKataFL_Faction` | [#1](https://github.com/jaykop/Kata/issues/1) PM-3 | 두 액터의 팩션 관계를 판정한다 |
| TG-3 | 높음 | `UKataTargetingComponent`와 Preset 확장 태스크 | TG-2 | 소프트 타겟과 락온 대상을 Preset으로 고른다 |
| TG-4 | 높음 | 대상을 정하는 Command | TG-3 | PreCommands로 액션 대상이 정해지고 콤보에서 이어진다 |
| TG-5 | 보통 | KataFramework 조합: 캐릭터에 컴포넌트, 캐릭터·AIController의 팀 인터페이스 | TG-2~TG-4 | KataFramework 캐릭터에서 바로 쓸 수 있다 |

## 영향과 제한

- 모듈 경계: 팩션과 타게팅은 KataTargeting에 둔다. 코어는 팩션을 알지 못한다.
  KataTargeting은 `Kata`, `TargetingSystem`, `AIModule`(팀 인터페이스)에 의존한다.
- 엔진 Beta 의존: `TargetingSystem`의 API가 바뀔 수 있다. 사용하는 범위를 Preset, 태스크 상속, 즉시 실행 요청으로 한정한다.
- 싱글플레이 전용이므로 대상 상태를 복제하지 않는다.
- 런타임 확인: PC 입력과 AI 실행 경로가 아직 없다. 테스트 실행 경로는 나중에 별도로 만든다(2026-09-24 사용자 결정).

## 사용자 확인 항목

- 각 단계의 빌드와 Details 표시는 사용자가 확인한다. 런타임 동작 확인은 테스트 실행 경로가 생긴 뒤에 한다.

## 완료 시 갱신할 문서

- [현재 구현 상태](../devlog/Implementation-Status.md): KataTargeting 항목.
- 새 manual: 타게팅 컴포넌트, Preset, 팩션 설정 사용법.
- [플러그인 분리 모듈화 계획](Plugin-Modularization-Plan.md): PM-4 완료 조건과 Pressure의 KataAI 이동.
- [문서 목록](../README.md): 이 문서 링크.
