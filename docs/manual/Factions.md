# 팩션 사용법

갱신: 2026-09-25
대상: KataTargeting 플러그인의 팩션 설정과 `UKataFL_Faction`  
적용 기준: [#13 타게팅 시스템](https://github.com/jaykop/Kata/issues/13) TG-2, [타게팅 시스템 설계](../plan/Targeting-Plan.md)  
확인 상태: 2026-09-24 사용자가 빌드·설정 화면·Blueprint 함수 노출 확인. 판정 결과는 미확인

## 목적과 준비

팩션은 액터 사이의 관계(우호, 중립, 적대)를 정한다. 타게팅 필터, AI 인지, 공격 대상 판정이 같은 관계를 쓴다.

- `KataTargeting` 플러그인을 켠다. 모듈이 시작되면 엔진 전역 팀 관계 판정 함수가 Kata 팩션 설정으로 바뀐다.
- 팩션은 Gameplay Tag로 표현한다. 태그 정의는 프로젝트가 소유한다(예: `Faction.Player`, `Faction.Monster.Undead`).
- 액터가 판정 대상이 되려면 액터 자신 또는 폰의 컨트롤러가 `IGenericTeamAgentInterface`를 구현해 팀 번호를 돌려줘야 한다.
  팩션을 컴포넌트로 할당하고 KataFramework 캐릭터가 인터페이스를 구현하는 부분은 아직 구현하지 않았다(#13 TG-3, TG-5).

## 사용 순서

1. Project Settings → Plugins → Kata Factions를 연다.
2. Factions 목록에 팩션 태그를 등록한다. 목록 순서가 팀 번호가 된다(첫 항목이 0).
3. Relations에 관계를 적는다. 한 줄은 두 팩션과 Attitude(Friendly, Neutral, Hostile)다.
   부모 태그로 적으면 그 아래 팩션 모두에 적용된다. 예: `Faction.Player` ↔ `Faction.Monster`, Hostile.
4. 액터의 팀 번호는 `GetFactionTeamId(Faction)`로 구해 팀 인터페이스 구현에서 돌려준다.
5. Blueprint나 C++에서 `IsHostile(Source, Target)` 등으로 관계를 판정한다.

## 주요 설정과 동작 규칙

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| Factions | 팩션 태그 목록. 인덱스가 엔진 `FGenericTeamId` 번호다 | 앞의 255개만 팀 번호를 받는다. 넘거나 중복되면 경고 로그를 남긴다 |
| Relations | 두 팩션과 관계. 방향을 구분하지 않는다 | 비어 있으면 같은 팩션끼리 우호, 나머지는 중립 |
| 관계 결정 규칙 | ① 태그 계층까지 맞는 행 중 두 태그 깊이 합이 가장 큰 행 ② 없고 같은 팩션이면 우호 ③ 그 밖은 중립 | 깊이가 같은 행이 여럿이면 먼저 적은 행. 같은 팩션끼리 싸우게 하려면 `(X, X, Hostile)`을 적는다 |
| `GetActorTeamId(Actor)` | 액터 자신의 팀 인터페이스, 없거나 NoTeam이면 폰의 컨트롤러에서 찾는다 | 찾지 못하면 NoTeam |
| `GetActorFaction(Actor)` | 액터 팀 번호의 팩션 태그 | 팀이 없거나 등록되지 않았으면 빈 태그 |
| `Get Attitude` / `IsFriendly` / `IsNeutral` / `IsHostile` | Source가 Target을 대하는 관계 | 한쪽이 없거나 팀이 없으면 중립 |
| `GetFactionAttitude(A, B)` | 액터 없이 두 태그의 관계를 관계표로 계산 | 빈 태그가 있으면 중립 |
| `GetFactionTeamId(Faction)` | 팩션의 팀 번호. 태그가 정확히 일치해야 한다 | 등록되지 않았으면 NoTeam |

- 엔진 기본 판정은 팀이 다르면 적대, NoTeam끼리도 우호로 본다. Kata는 NoTeam이 끼면 중립으로 판정한다.
- AIPerception의 적·아군 구분 감지도 같은 전역 판정 함수를 쓰므로 Kata 팩션 관계를 따른다.

C++는 `FunctionLibraries/KataFL_Faction.h`를 포함하고 KataTargeting 모듈에 의존한다.
액터 기반 GetActorAttitude·IsHostile 등은 전역 팀 판정 함수를 사용하지만, GetFactionAttitude는 설정의 태그 관계표를 직접 계산한다.
다른 모듈이 전역 판정 함수를 교체하면 두 경로의 결과가 달라질 수 있다.
KataTargeting 종료는 이전 사용자 함수를 보관해 복구하는 방식이 아니라 엔진 기본 판정으로 되돌린다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 모든 판정이 중립 | 액터와 컨트롤러 모두 팀 인터페이스를 구현하지 않았거나 팀 번호가 NoTeam | 팀 인터페이스 구현에서 `GetFactionTeamId`로 구한 번호를 돌려준다 |
| Factions 순서를 바꾼 뒤 관계가 달라짐 | 팀 번호가 목록 인덱스다 | 이미 번호를 저장해 두는 코드가 있으면 태그로 다시 구한다 |
| 프로젝트의 관계 판정 함수가 무시됨 | 전역 판정 함수는 나중에 등록한 쪽이 적용된다 | 한 곳에서만 등록한다 |
| 한쪽만 적대하는 관계 | 관계표는 방향을 구분하지 않는다 | 현재 지원하지 않는다 |

## 확인 상태와 근거

2026-09-24 사용자가 빌드, Kata Factions 설정 화면 편집, `Kata|Faction` Blueprint 함수 노출을 확인했다.
팩션을 할당하는 컴포넌트와 캐릭터의 팀 인터페이스가 없어 실제 판정 결과는 확인하지 않았다.
2026-09-25에는 현재 함수·모듈 소스를 대조해 위 계약만 보강했다. 빌드·실행은 수행하지 않았다.

- [KataFactionSettings.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/Faction/KataFactionSettings.h): `UKataFactionSettings`, `FKataFactionRelation`.
- [KataFL_Faction.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/FunctionLibraries/KataFL_Faction.h): 판정 함수.
- [KataTargetingModule.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/KataTargetingModule.h): 전역 판정 함수 등록과 해제.
- [현재 구현 상태](../devlog/Implementation-Status.md).
