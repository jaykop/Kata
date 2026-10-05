# 타게팅 사용법

갱신: 2026-10-05  
대상: KataTargeting 플러그인의 타게팅 컴포넌트, 타겟 지점 컴포넌트, Targeting Preset 확장 태스크, 대상·방향 결정 Command와 회전 태스크  
적용 기준: [#13 타게팅 시스템](https://github.com/jaykop/Kata/issues/13) TG-3·TG-4·TG-6, [타게팅 시스템 설계](../plan/Targeting-Plan.md)  
확인 상태: 2026-09-26 사용자가 빌드, Command·태스크 표시, 프리뷰 회전 태스크 동작 확인. 락온·입력 런타임은 미확인. TG-6 락온 지점은 2026-10-02 사용자 빌드와 샘플 캐릭터의 지점 부착까지 확인했다. 이후의 구체 표시·태그 제한·디버거 카테고리 빌드와 락온 런타임은 확인 전이다

## 목적과 준비

PC가 소프트 타겟(액터)과 락온 지점(대상 부위)을 고르고, 액션을 시작할 때 대상과 공격 방향을 정하게 한다.
후보 선택은 엔진 Targeting System의 `UTargetingPreset`이 맡고, Kata는 컴포넌트, 필터·정렬 태스크, Command와 회전 태스크를 제공한다.

- `KataTargeting` 플러그인을 켠다. 엔진 `TargetingSystem`(Beta)과 `GameplayAbilities` 플러그인도 함께 켜진다.
- 팩션 필터를 쓰려면 [팩션 사용법](Factions.md)대로 Kata Factions를 설정하고, 대상 액터가 팀 번호를 돌려줘야 한다.

## 사용 순서

1. PC 캐릭터는 KataFramework의 AKataPlayerCharacter를 부모로 쓴다. 이 클래스가 `Kata Player Targeting Component`를 이미 가진다.
   다른 액터에는 컴포넌트를 직접 추가한다. AI 캐릭터는 KataAI의 `UKataAITargetingComponent`를 사용한다. 시각 후보·Preset 구성은 [AI 사용법](AI.md)을 따른다.
2. Faction에 팩션 태그를 지정한다.
3. 락온할 액터의 부위(메시 소켓)에 `Kata Target Point` 컴포넌트를 붙이고 Role Tags에 락온 역할 태그(샘플: `TargetPoint.LockOn`)를 넣는다.
   한 액터에 여러 개를 둘 수 있다. 지점이 없는 액터는 락온 후보가 아니다.
4. Content Browser에서 Targeting Preset 에셋을 만든다. 예시 구성은 다음과 같다.
   - 소프트 타겟: 엔진 범위 수집(AOE) → Kata Filter Faction → 엔진 거리 정렬 + Kata Sort Screen Center
   - 락온: 범위를 넓힌 AOE → Kata Filter Faction → Kata Expand Target Points(Required Role Tags: `TargetPoint.LockOn`) → Kata Sort Screen Center
   - 왼쪽·오른쪽 전환: 락온 Preset에서 Kata Expand Target Points 뒤에 Kata Filter Lock Side(Left 또는 Right)를 더한 두 에셋
5. 컴포넌트의 Soft Target Preset, Lock On Preset, Switch Left Preset, Switch Right Preset에 지정한다.
   락온 상태를 태그로 알리려면 Locking Status Tag와 Targeted Status Tag를 지정한다(샘플: `Status.LockOn.Locking`, `Status.LockOn.Targeted`).
6. 입력이나 Blueprint에서 `AcquireLock`, `SwitchLockLeft`, `SwitchLockRight`, `ReleaseLock`을 호출한다.
7. 공격 액션의 PreCommands에 `Resolve Target`을 넣는다. 회전 방식은 다음 중 하나를 선택한다.
   - 시작 프레임에 즉시 회전하려면 PreCommands에서 `Resolve Target` 뒤에 `Resolve Facing`을 넣는다.
   - 일정 시간 동안 회전하려면 타임라인 앞쪽에 `Kata Task: Rotate To Facing`을 둔다.

## 주요 설정과 동작 규칙

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| Faction | 이 액터의 팩션 태그 | `GetFactionTeamId()`로 팀 번호를 얻는다. 미등록이면 NoTeam |
| `GetCurrentTarget` | 현재 대상. PC는 락온 지점의 액터, 없으면 소프트 타겟 | 둘 다 없으면 nullptr |
| `ResolveActionTarget` | 액션 시작 때의 대상. PC는 락온 대상, 없고 이동 입력이 있으면 소프트 타겟을 비우고 nullptr, 둘 다 없으면 소프트 타겟을 갱신해 돌려준다 | 후보가 없으면 nullptr |
| `CanKeepActionTarget` | 이어받은 대상을 그대로 써도 되는지. PC는 락온 중이면 락온 대상일 때만, 락온이 없으면 이동 입력이 없을 때만 true | 기반 구현은 true |
| `ResolveFacingDirection` | 액션 시작 때 바라볼 수평 방향. PC는 락온 지점 위치 → 이동 입력 → 액션 대상 순서 | 기반 구현은 액션 대상 쪽. 방향이 없으면 false를 반환하며 회전하지 않는다 |
| `UpdateSoftTarget` | Soft Target Preset을 즉시 실행해 첫 후보를 소프트 타겟으로 둔다 | 후보가 없으면 소프트 타겟을 비운다 |
| `AcquireLock` | Lock On Preset의 첫 지점으로 락온한다. Preset이 지점을 펼치지 않으면 후보가 없다 | 후보가 없으면 상태를 바꾸지 않고 false |
| `SwitchLockLeft` / `SwitchLockRight` | 방향별 전환 Preset의 첫 지점으로 바꾼다. 같은 액터의 다른 부위도 후보다 | 락온 중이 아니거나 후보가 없으면 현재 지점을 유지하고 false |
| `GetLockTarget` / `GetLockPoint` | 락온 지점의 소유 액터 / 락온 지점 | 락온이 없으면 nullptr |
| Max Lock Distance | 락온 지점까지의 거리가 이 값을 넘으면 락온을 잃는다(cm) | 0이면 거리로 풀리지 않는다 |
| Lock Break Tags | 대상 ASC에 하나라도 있으면 락온을 잃는다(예: 사망 태그) | 대상에 ASC가 없으면 보지 않는다 |
| Lock Lost Behavior | 대상 파괴·거리 초과·해제 태그로 락온을 잃었을 때 Release(해제) 또는 Switch To Next(락온 Preset의 다음 지점) | Release. 다음 지점이 없으면 해제 |
| Lock Point Disabled Behavior | 락온 중인 지점이 꺼졌을 때(부위 파괴 등)의 동작. 선택지는 Lock Lost Behavior와 같다 | Release |
| Locking Status Tag / Targeted Status Tag | 락온 중 자기 ASC / 대상 액터 ASC에 붙이는 Loose 태그(`Status` 하위만 선택) | 비어 있거나 ASC가 없으면 붙이지 않는다. 같은 액터의 다른 부위로 바뀌면 대상 태그를 다시 붙이지 않는다 |
| Tick Interval | 락온 유효성 검사 주기. 락온 중에만 Tick한다 | 0.1초 |
| On Lock Target Changed | 락온 지점이 바뀔 때 (Old Point, New Point) | 해제하면 New Point가 nullptr |
| Kata Target Point > Role Tags·Enabled | 지점의 역할 태그(`TargetPoint` 하위만 선택)와 시작 활성 여부. 실행 중에는 `SetTargetPointEnabled`로 켜고 끈다 | 켜짐. 꺼진 지점은 모든 용도에서 후보가 아니다 |
| Kata Target Point > Shape(Sphere Radius·Shape Color) | 에디터에서 지점을 표시하는 와이어 구체의 크기와 색. 판정에는 쓰지 않는다 | 12cm, 주황. 게임에서는 숨겨지고 충돌이 없어 물리 바디도 만들지 않는다 |
| Kata Expand Target Points | 액터 결과를 켜져 있고 Required Role Tags(`TargetPoint` 하위만 선택)를 모두 가진 지점 결과로 펼친다. 액터를 모으는 태스크 바로 뒤에 둔다 | 지점이 없는 액터는 빠진다 |
| Kata Filter Faction | 실행 주체가 후보를 대하는 관계로 거른다 | 기본값은 적대만 남긴다 |
| Kata Filter Lock Side | 카메라에서 본 현재 락온 지점의 왼쪽 또는 오른쪽 후보만 남긴다 | 현재 지점만 빼고 같은 액터의 다른 부위는 남긴다. 락온 중이 아니면 거르지 않는다 |
| Kata Sort Screen Center | 시선 중앙에 가까운 후보에 높은 우선순위를 부여한다. 플레이어 카메라, 없으면 액터 눈 시점 기준 | Weight 1 |
| Resolve Target (Command) | 실행 주체의 컴포넌트로 이번 액션의 대상을 정한다. Keep Valid Target이 켜져 있고 이어받은 대상이 유효하며 `CanKeepActionTarget`이 true면 그대로 둔다 | Keep Valid Target 켬. 컴포넌트가 없으면 아무것도 하지 않는다. 대상을 못 찾으면 대상을 비운다 |
| Resolve Facing (Command) | `ResolveFacingDirection` 방향으로 Yaw를 즉시 맞춘다 | 방향이나 컴포넌트가 없으면 회전하지 않는다 |
| Kata Task: Rotate To Facing | 구간 동안 Rotation Rate로 목표 Yaw에 다가간다. 목표를 넘지 않고, 구간이 끝나면 그 자리에서 멈춘다 | Rotation Rate 720°/s, Duration 0.2초. Update Direction Every Tick 켬: 매 Tick 방향을 다시 구한다. 끄면 시작 방향으로만 돌고 도달하면 끝난다. Single Frame·Duration 0은 설정 오류 |
| 가중치 정렬(Weight, Higher Is Better) | Kata 정렬 태스크는 정규화 점수에 Weight를 곱해 더한다 | 엔진 정렬 태스크와 섞어 쓸 수 있다. 첫 후보가 가장 우선한다 |

- PC의 공격 방향 우선순위는 락온 지점 → 이동 입력 방향 → 소프트 타겟 → 정면 유지다. 소프트 타겟은 방향 기준이므로 락온이나 이동 입력이 있으면 정하지 않는다.
- 이동 입력은 폰의 이동 입력 벡터로 읽는다. 이번 프레임 입력이 없으면 직전 프레임 입력을 쓴다.
- 회전 속도는 한 구간에서 돌 수 있는 각도를 정한다. 기본값(720°/s, 0.2초)은 최대 144°이므로 뒤돌아 공격하려면 속도나 구간을 늘린다.
- 대상 파괴와 지점 비활성화는 검사 주기와 관계없이 알림으로 즉시 처리한다.
- 컴포넌트는 대상을 약한 참조로 보관한다. 실행 주체 자신은 후보에서 뺀다.
- 디버그: 콘솔 `Kata.Targeting.Debug 1`이면 락온 지점(빨강)과 소프트 타겟(노랑, 갱신 후 1초)을 표시한다. Shipping 빌드에는 없다.
- GameplayDebugger의 `KataTargeting` 카테고리는 락온 지점, 소프트 타겟, Lock On Preset이 지금 고를 수 있는 후보 지점을 우선순위(#1부터)와 함께 보이고 지점 위치에 구를 그린다(락온 지점 빨강, 후보 초록). 켠 동안 수집 주기마다 Preset을 실행한다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 팩션 필터가 모든 후보를 뺀다 | 대상이 팀 번호를 돌려주지 않아 중립으로 판정된다 | 대상 액터나 컨트롤러가 팀 인터페이스를 구현하게 한다. AKataCharacter 파생 캐릭터는 이미 구현한다 |
| 전환 후보가 반대쪽에서 나온다 | 좌우는 카메라에서 락온 지점을 바라본 수평 방향 기준이다 | 전환 Preset에 Kata Filter Lock Side가 있는지, Side 값이 맞는지 확인한다 |
| `AcquireLock`이 항상 false다 | 락온 Preset에 Kata Expand Target Points가 없거나, 대상에 Required Role Tags를 가진 켜진 Kata Target Point가 없다 | Preset 구성과 지점의 Role Tags·Enabled를 확인한다 |
| 시야가 가려져도 락온이 유지된다 | 시야 조건은 아직 없다 | 필요하면 요청한다 |
| 이동 입력이 있는데 입력 방향으로 돌지 않는다 | 공격 중 이동 입력을 무시하면(`IgnoreMoveInput`) 입력이 쌓이지 않는다 | 입력 계층(#19)에서 입력 방향 전달을 다룬다 |
| Resolve Facing이나 회전 태스크가 효과가 없다 | `bUseControllerRotationYaw`가 켜진 캐릭터는 컨트롤러가 회전을 덮어쓴다 | 이동 방향 회전 방식의 캐릭터에서 쓴다 |
| 대상을 읽는 시작 조건이 새 대상으로 판정하지 않는다 | 시작 조건은 PreCommands보다 먼저 평가된다 | 이어받은 대상 기준이라는 점을 고려해 조건을 구성한다 |

## 락온 화면 표시와 카메라

`AKataPlayerController`가 락온 지점 변경을 카메라와 MainHUD에 전달한다. 타게팅 컴포넌트에서 별도로 화면 표시를 구현할 필요는 없다.
지점의 Lock On Camera Data는 선택 사항이다. 체크박스를 끄거나 비워 두면 카메라 매니저의 기본 락온 설정을 쓰고, 켜고 지정하면 그 에셋의 구도와 배치 데이터를 그대로 쓴다.
KataTargeting은 UDataAsset 참조와 선택기 제한만 제공하며 KataCamera에 직접 의존하지 않는다.
설정은 [카메라 사용법](Camera.md#락온-카메라), 표시는 [HUD 사용법](HUD.md)을 따른다. 이번 연결의 사용자 빌드·실행은 확인 전이다.

## 확인 상태와 근거

2026-09-25 사용자가 빌드와 Targeting Preset 태스크 목록의 Kata Filter Faction·Kata Filter Lock Side·Kata Sort Screen Center 표시를 확인했다.
2026-09-26 AKataPlayerCharacter 파생 BP에서 PC용 타게팅 컴포넌트 항목 표시를 확인했다(#17).
2026-09-26 사용자가 빌드, 액션 에디터의 Resolve Target·Resolve Facing과 Rotate To Facing 표시, 프리뷰에서 Rotate To Facing 회전을 확인했다(TG-4).
입력 연결(#19)이 없어 락온·전환, 이동 입력 우선순위, 콤보 대상 유지의 런타임 동작은 확인하지 않았다.

- [KataTargetingComponent.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/Targeting/KataTargetingComponent.h): 기반 컴포넌트.
- [KataPlayerTargetingComponent.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/Targeting/KataPlayerTargetingComponent.h): PC 컴포넌트.
- [Tasks](../../Plugins/KataTargeting/Source/KataTargeting/Public/Tasks): 필터·정렬 태스크와 회전 태스크.
- [Commands](../../Plugins/KataTargeting/Source/KataTargeting/Public/Commands): 대상·방향 결정 Command.
- [작업 상태](https://github.com/jaykop/Kata/issues).

