# 타게팅 사용법

갱신: 2026-09-26  
대상: KataTargeting 플러그인의 타게팅 컴포넌트, Targeting Preset 확장 태스크, 대상·방향 결정 Command와 회전 태스크  
적용 기준: [#13 타게팅 시스템](https://github.com/jaykop/Kata/issues/13) TG-3·TG-4, [타게팅 시스템 설계](../plan/Targeting-Plan.md)  
확인 상태: 2026-09-26 사용자가 빌드, Command·태스크 표시, 프리뷰 회전 태스크 동작 확인. 락온·입력 런타임은 미확인

## 목적과 준비

PC가 소프트 타겟과 락온 대상을 고르고, 액션을 시작할 때 대상과 공격 방향을 정하게 한다.
후보 선택은 엔진 Targeting System의 `UTargetingPreset`이 맡고, Kata는 컴포넌트, 필터·정렬 태스크, Command와 회전 태스크를 제공한다.

- `KataTargeting` 플러그인을 켠다. 엔진 `TargetingSystem`(Beta)과 `GameplayAbilities` 플러그인도 함께 켜진다.
- 팩션 필터를 쓰려면 [팩션 사용법](Factions.md)대로 Kata Factions를 설정하고, 대상 액터가 팀 번호를 돌려줘야 한다.

## 사용 순서

1. PC 캐릭터는 KataFramework의 AKataPlayerCharacter를 부모로 쓴다. 이 클래스가 `Kata Player Targeting Component`를 이미 가진다.
   다른 액터에는 컴포넌트를 직접 추가한다. 몬스터는 이후 KataAI가 제공하는 파생 컴포넌트를 쓴다.
2. Faction에 팩션 태그를 지정한다.
3. Content Browser에서 Targeting Preset 에셋을 만든다. 예시 구성은 다음과 같다.
   - 소프트 타겟: 엔진 범위 수집(AOE) → Kata Filter Faction → 엔진 거리 정렬 + Kata Sort Screen Center
   - 락온: 범위를 넓힌 AOE → Kata Filter Faction → Kata Sort Screen Center
   - 왼쪽·오른쪽 전환: 락온 Preset에 Kata Filter Lock Side(Left 또는 Right)를 더한 두 에셋
4. 컴포넌트의 Soft Target Preset, Lock On Preset, Switch Left Preset, Switch Right Preset에 지정한다.
5. 입력이나 Blueprint에서 `AcquireLock`, `SwitchLockLeft`, `SwitchLockRight`, `ReleaseLock`을 호출한다.
6. 공격 액션의 PreCommands에 `Resolve Target`을 넣는다. 방향은 둘 중 하나로 맞춘다.
   - 시작 프레임에 바로 돌리려면 PreCommands에서 `Resolve Target` 뒤에 `Resolve Facing`을 넣는다.
   - 도는 과정을 보이려면 타임라인 앞쪽에 `Kata Task: Rotate To Facing`을 둔다.

## 주요 설정과 동작 규칙

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| Faction | 이 액터의 팩션 태그 | `GetFactionTeamId()`로 팀 번호를 얻는다. 미등록이면 NoTeam |
| `GetCurrentTarget` | 현재 대상. PC는 락온 대상, 없으면 소프트 타겟 | 둘 다 없으면 nullptr |
| `ResolveActionTarget` | 액션 시작 때의 대상. PC는 락온 대상, 없고 이동 입력이 있으면 소프트 타겟을 비우고 nullptr, 둘 다 없으면 소프트 타겟을 갱신해 돌려준다 | 후보가 없으면 nullptr |
| `CanKeepActionTarget` | 이어받은 대상을 그대로 써도 되는지. PC는 락온 중이면 락온 대상일 때만, 락온이 없으면 이동 입력이 없을 때만 true | 기반 구현은 true |
| `ResolveFacingDirection` | 액션 시작 때 바라볼 수평 방향. PC는 락온 대상 → 이동 입력 → 액션 대상 순서 | 기반 구현은 액션 대상 쪽. 방향이 없으면 false라 돌지 않는다 |
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
| Resolve Target (Command) | 실행 주체의 컴포넌트로 이번 액션의 대상을 정한다. Keep Valid Target이 켜져 있고 이어받은 대상이 유효하며 `CanKeepActionTarget`이 true면 그대로 둔다 | Keep Valid Target 켬. 컴포넌트가 없으면 아무것도 하지 않는다. 대상을 못 찾으면 대상을 비운다 |
| Resolve Facing (Command) | `ResolveFacingDirection` 방향으로 Yaw를 즉시 맞춘다 | 방향이 없거나 컴포넌트가 없으면 돌지 않는다 |
| Kata Task: Rotate To Facing | 구간 동안 Rotation Rate로 목표 Yaw에 다가간다. 목표를 넘지 않고, 구간이 끝나면 그 자리에서 멈춘다 | Rotation Rate 720°/s, Duration 0.2초. Update Direction Every Tick 켬: 매 Tick 방향을 다시 구한다. 끄면 시작 방향으로만 돌고 도달하면 끝난다. Single Frame·Duration 0은 설정 오류 |
| 가중치 정렬(Weight, Higher Is Better) | Kata 정렬 태스크는 정규화 점수에 Weight를 곱해 더한다 | 엔진 정렬 태스크와 섞어 쓸 수 있다. 첫 후보가 가장 우선한다 |

- PC의 공격 방향 우선순위는 락온 대상 → 이동 입력 방향 → 소프트 타겟 → 정면 유지다. 소프트 타겟은 방향 기준이므로 락온이나 이동 입력이 있으면 정하지 않는다.
- 이동 입력은 폰의 이동 입력 벡터로 읽는다. 이번 프레임 입력이 없으면 직전 프레임 입력을 쓴다.
- 회전 속도는 한 구간에서 돌 수 있는 각도를 정한다. 기본값(720°/s, 0.2초)은 최대 144°이므로 뒤돌아 공격하려면 속도나 구간을 늘린다.
- 대상 파괴는 검사 주기와 관계없이 대상의 OnEndPlay로 즉시 처리한다.
- 컴포넌트는 대상을 약한 참조로 보관한다. 실행 주체 자신은 후보에서 뺀다.
- 디버그: 콘솔 `Kata.Targeting.Debug 1`이면 락온 대상(빨강)과 소프트 타겟(노랑, 갱신 후 1초)을 표시한다. Shipping 빌드에는 없다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 팩션 필터가 모든 후보를 뺀다 | 대상이 팀 번호를 돌려주지 않아 중립으로 판정된다 | 대상 액터나 컨트롤러가 팀 인터페이스를 구현하게 한다. AKataCharacter 파생 캐릭터는 이미 구현한다 |
| 전환 후보가 반대쪽에서 나온다 | 좌우는 카메라에서 락온 대상을 바라본 수평 방향 기준이다 | 전환 Preset에 Kata Filter Lock Side가 있는지, Side 값이 맞는지 확인한다 |
| 시야가 가려져도 락온이 유지된다 | 시야 조건은 아직 없다 | 필요하면 요청한다 |
| 이동 입력이 있는데 입력 방향으로 돌지 않는다 | 공격 중 이동 입력을 무시하면(`IgnoreMoveInput`) 입력이 쌓이지 않는다 | 입력 계층(#19)에서 입력 방향 전달을 다룬다 |
| Resolve Facing이나 회전 태스크가 효과가 없다 | `bUseControllerRotationYaw`가 켜진 캐릭터는 컨트롤러가 회전을 덮어쓴다 | 이동 방향 회전 방식의 캐릭터에서 쓴다 |
| 대상을 읽는 시작 조건이 새 대상으로 판정하지 않는다 | 시작 조건은 PreCommands보다 먼저 평가된다 | 이어받은 대상 기준이라는 점을 고려해 조건을 구성한다 |

## 확인 상태와 근거

2026-09-25 사용자가 빌드와 Targeting Preset 태스크 목록의 Kata Filter Faction·Kata Filter Lock Side·Kata Sort Screen Center 표시를 확인했다.
2026-09-26 AKataPlayerCharacter 파생 BP에서 PC용 타게팅 컴포넌트 항목 표시를 확인했다(#17).
2026-09-26 사용자가 빌드, 액션 에디터의 Resolve Target·Resolve Facing과 Rotate To Facing 표시, 프리뷰에서 Rotate To Facing 회전을 확인했다(TG-4).
입력 연결(#19)이 없어 락온·전환, 이동 입력 우선순위, 콤보 대상 유지의 런타임 동작은 확인하지 않았다.

- [KataTargetingComponent.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/Targeting/KataTargetingComponent.h): 기반 컴포넌트.
- [KataPlayerTargetingComponent.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/Targeting/KataPlayerTargetingComponent.h): PC 컴포넌트.
- [Tasks](../../Plugins/KataTargeting/Source/KataTargeting/Public/Tasks): 필터·정렬 태스크와 회전 태스크.
- [Commands](../../Plugins/KataTargeting/Source/KataTargeting/Public/Commands): 대상·방향 결정 Command.
- [현재 구현 상태](../devlog/Implementation-Status.md).
