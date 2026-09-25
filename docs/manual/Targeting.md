# 타게팅 사용법

갱신: 2026-09-25  
대상: KataTargeting 플러그인의 타게팅 컴포넌트와 Targeting Preset 확장 태스크  
적용 기준: [#13 타게팅 시스템](https://github.com/jaykop/Kata/issues/13) TG-3, [타게팅 시스템 설계](../plan/Targeting-Plan.md)  
확인 상태: 2026-09-25 사용자가 빌드와 Preset 태스크 표시 확인. 런타임 동작은 미확인

## 목적과 준비

PC가 소프트 타겟과 락온 대상을 고르게 한다. 후보 선택은 엔진 Targeting System의 `UTargetingPreset`이 맡고,
Kata는 컴포넌트와 필터·정렬 태스크를 제공한다.

- `KataTargeting` 플러그인을 켠다. 엔진 `TargetingSystem`(Beta)과 `GameplayAbilities` 플러그인도 함께 켜진다.
- 팩션 필터를 쓰려면 [팩션 사용법](Factions.md)대로 Kata Factions를 설정하고, 대상 액터가 팀 번호를 돌려줘야 한다.
- 액션 시작 때 대상을 자동으로 정하는 Command는 아직 없다(#13 TG-4). 지금은 컴포넌트 함수를 직접 호출한다.

## 사용 순서

1. PC 캐릭터에 `Kata Player Targeting Component`를 추가한다. 몬스터는 이후 KataAI가 제공하는 파생 컴포넌트를 쓴다.
2. Faction에 팩션 태그를 지정한다.
3. Content Browser에서 Targeting Preset 에셋을 만든다. 예시 구성은 다음과 같다.
   - 소프트 타겟: 엔진 범위 수집(AOE) → Kata Filter Faction → 엔진 거리 정렬 + Kata Sort Screen Center
   - 락온: 범위를 넓힌 AOE → Kata Filter Faction → Kata Sort Screen Center
   - 왼쪽·오른쪽 전환: 락온 Preset에 Kata Filter Lock Side(Left 또는 Right)를 더한 두 에셋
4. 컴포넌트의 Soft Target Preset, Lock On Preset, Switch Left Preset, Switch Right Preset에 지정한다.
5. 입력이나 Blueprint에서 `AcquireLock`, `SwitchLockLeft`, `SwitchLockRight`, `ReleaseLock`을 호출한다.
   액션을 시작하는 쪽은 `ResolveActionTarget`으로 대상을 얻는다.

## 주요 설정과 동작 규칙

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| Faction | 이 액터의 팩션 태그 | `GetFactionTeamId()`로 팀 번호를 얻는다. 미등록이면 NoTeam |
| `GetCurrentTarget` | 현재 대상. PC는 락온 대상, 없으면 소프트 타겟 | 둘 다 없으면 nullptr |
| `ResolveActionTarget` | 액션 시작 때의 대상. PC는 락온 대상, 없으면 소프트 타겟을 갱신해 돌려준다 | 후보가 없으면 nullptr |
| `UpdateSoftTarget` | Soft Target Preset을 즉시 실행해 첫 후보를 소프트 타겟으로 둔다 | 후보가 없으면 소프트 타겟을 비운다 |
| `AcquireLock` | Lock On Preset의 첫 후보로 락온한다 | 후보가 없으면 상태를 바꾸지 않고 false |
| `SwitchLockLeft` / `SwitchLockRight` | 방향별 전환 Preset의 첫 후보로 바꾼다 | 락온 중이 아니거나 후보가 없으면 현재 대상을 유지하고 false |
| Max Lock Distance | 이 거리를 넘으면 락온을 잃는다(cm) | 0이면 거리로 풀리지 않는다 |
| Lock Break Tags | 대상 ASC에 하나라도 있으면 락온을 잃는다(예: 사망 태그) | 대상에 ASC가 없으면 보지 않는다 |
| Lock Lost Behavior | 락온을 잃었을 때 Release(해제) 또는 Switch To Next(락온 Preset의 다음 대상) | Release. 다음 대상이 없으면 해제 |
| Tick Interval | 락온 유효성 검사 주기. 락온 중에만 Tick한다 | 0.1초 |
| On Lock Target Changed | 락온 대상이 바뀔 때 (Old, New) | 해제하면 New가 nullptr |
| Kata Filter Faction | 실행 주체가 후보를 대하는 관계로 거른다 | 기본값은 적대만 남긴다 |
| Kata Filter Lock Side | 카메라에서 본 현재 락온 대상의 왼쪽 또는 오른쪽 후보만 남긴다 | 락온 대상 자신은 뺀다. 락온 중이 아니면 거르지 않는다 |
| Kata Sort Screen Center | 시선 중앙에 가까운 후보를 앞세운다. 플레이어 카메라, 없으면 액터 눈 시점 기준 | Weight 1 |
| 가중치 정렬(Weight, Higher Is Better) | Kata 정렬 태스크는 정규화 점수에 Weight를 곱해 더한다 | 엔진 정렬 태스크와 섞어 쓸 수 있다. 첫 후보가 가장 우선한다 |

- 대상 파괴는 검사 주기와 관계없이 대상의 OnEndPlay로 즉시 처리한다.
- 컴포넌트는 대상을 약한 참조로 보관한다. 실행 주체 자신은 후보에서 뺀다.
- 디버그: 콘솔 `Kata.Targeting.Debug 1`이면 락온 대상(빨강)과 소프트 타겟(노랑, 갱신 후 1초)을 표시한다. Shipping 빌드에는 없다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 팩션 필터가 모든 후보를 뺀다 | 대상이 팀 번호를 돌려주지 않아 중립으로 판정된다 | 대상 액터나 컨트롤러가 팀 인터페이스를 구현하게 한다. KataFramework 조합은 TG-5에서 제공한다 |
| 전환 후보가 반대쪽에서 나온다 | 좌우는 카메라에서 락온 대상을 바라본 수평 방향 기준이다 | 전환 Preset에 Kata Filter Lock Side가 있는지, Side 값이 맞는지 확인한다 |
| 시야가 가려져도 락온이 유지된다 | 시야 조건은 아직 없다 | 필요하면 요청한다 |
| 액션 대상이 자동으로 정해지지 않는다 | 대상 결정 Command가 아직 없다(TG-4) | 지금은 `ResolveActionTarget` 결과를 직접 넘긴다 |

## 확인 상태와 근거

2026-09-25 사용자가 빌드와 Targeting Preset 태스크 목록의 Kata Filter Faction·Kata Filter Lock Side·Kata Sort Screen Center 표시를 확인했다.
입력 연결과 캐릭터 팀 인터페이스가 없어 락온·전환 등 런타임 동작은 확인하지 않았다.

- [KataTargetingComponent.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/Targeting/KataTargetingComponent.h): 기반 컴포넌트.
- [KataPlayerTargetingComponent.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/Targeting/KataPlayerTargetingComponent.h): PC 컴포넌트.
- [Tasks](../../Plugins/KataTargeting/Source/KataTargeting/Public/Targeting/Tasks): 필터·정렬 태스크.
- [현재 구현 상태](../devlog/Implementation-Status.md).
