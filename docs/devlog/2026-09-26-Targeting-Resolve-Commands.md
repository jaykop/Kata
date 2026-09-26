# 대상·방향 결정 Command와 회전 태스크

작성: 2026-09-26  
갱신: 2026-09-26  
유형: 구현 기록  
대상: KataTargeting `UKataCommand_ResolveTarget`, `UKataCommand_ResolveFacing`, `UKataTask_RotateToFacing`, 타게팅 컴포넌트  
기준: [#13](https://github.com/jaykop/Kata/issues/13) TG-4 구현 작업 트리. 같은 시점 다른 작업의 미커밋 문서 변경은 포함하지 않는다.

## 배경과 결론

TG-3까지는 액션을 시작하는 쪽이 `ResolveActionTarget` 결과를 직접 넘겨야 했다. TG-4에서 액션의 PreCommands로 대상을 정하는 Command를 추가했다.
계획 단계에서 사용자가 PC 공격 방향의 우선순위를 정했고, 방향을 맞추는 Command와 구간 동안 도는 태스크를 함께 추가했다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 대상 결정 | 호출자가 직접 전달 | PreCommand `Resolve Target`. Keep Valid Target(기본 켬)과 `CanKeepActionTarget` 거부권 |
| 방향 | 없음 | PreCommand `Resolve Facing`(즉시), `Kata Task: Rotate To Facing`(구간 동안 일정 속도) |
| 기반 컴포넌트 | `GetCurrentTarget`, `ResolveActionTarget` | `CanKeepActionTarget`, `ResolveFacingDirection` 추가(BlueprintNativeEvent) |
| PC 컴포넌트 | 락온 → 소프트 타겟 | 락온 → 이동 입력(소프트 타겟 비움) → 소프트 타겟. 방향도 같은 순서 |
| 의존 | Kata 코어 없음 | `Kata` 플러그인, Public `KataRuntime` |

## 주요 결정과 이유

- **락온 대상이 최우선이다(사용자 결정).** Keep Valid Target이 켜져 있어도 락온이 이어받은 대상과 다르면 다시 구한다.
  Command는 기반 타입만 알기 때문에 유지 여부를 컴포넌트의 `CanKeepActionTarget`에 묻는다. 기반 구현은 항상 유지한다.
- **PC 공격 방향 우선순위는 락온 대상 → 이동 입력 → 소프트 타겟 → 정면이다(사용자 결정).**
- **소프트 타겟은 공격 방향의 기준일 뿐이다(사용자 결정).** 더 우선하는 락온이나 이동 입력이 있으면 소프트 타겟을 정하지 않고 대상을 비운다.
  입력 방향으로 돌면서 등 뒤의 소프트 타겟을 대상으로 두는 어긋남이 생기지 않는다.
- **대상과 방향을 다른 Command로 나눴다.** 대상은 Context의 데이터이고 회전은 액터를 바꾸는 부작용이다.
  대상만 필요하거나 방향만 필요한 액션이 있으므로 PreCommands에서 골라 쓴다.
- **즉시 회전 Command와 회전 태스크를 모두 둔다(사용자 확인).** Command는 시작 프레임에 맞추고, 태스크는 Rotation Rate로 Yaw를 더해 도는 과정을 보인다.
  둘 다 컴포넌트의 `ResolveFacingDirection`을 쓴다. 태스크의 방향은 매 Tick 다시 구하는 것이 기본이다(사용자 결정).
- **이동 입력은 폰의 이동 입력 벡터에서 읽는다.** 입력 계층(#19)이 아직 없기 때문이다. 공격 입력과 이동 입력의 처리 순서가 정해지지 않았으므로
  이번 프레임 입력이 없으면 직전 프레임 입력을 쓴다. 엔진 `APawn::Internal_AddMovementInput`은 `IsMoveInputIgnored()`가 참이면 입력을 버리므로
  공격 중 이동 입력을 무시하면 입력 방향이 없는 것으로 판정된다.
- **회전 태스크의 기본 속도는 720°/s다.** 속도는 도는 양이 아니라 빠르기이며, 목표를 넘지 않으므로 360°/s를 넘어도 한 바퀴 이상 돌지 않는다.
  0.1~0.3초 선딜레이 안에 돌려면 360°/s보다 빨라야 한다.

## 근거

- [KataCommand_ResolveTarget.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/Commands/KataCommand_ResolveTarget.h): 대상 결정과 유지 조건.
- [KataCommand_ResolveFacing.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/Commands/KataCommand_ResolveFacing.h): 즉시 회전.
- [KataTask_RotateToFacing.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/Tasks/KataTask_RotateToFacing.h): 구간 회전과 설정 오류.
- [KataPlayerTargetingComponent.h](../../Plugins/KataTargeting/Source/KataTargeting/Public/Targeting/KataPlayerTargetingComponent.h): PC 우선순위.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 빌드 | 사용자 Editor 빌드(2026-09-26) | 성공 | Game 타깃 |
| 에디터 표시 | 사용자 에디터 확인 | PreCommands 목록에 Resolve Target·Resolve Facing, 태스크 목록에 Rotate To Facing | - |
| 회전 태스크 | 사용자 프리뷰 확인 | 정상 동작 | 매 Tick 갱신과 고정 방향의 차이, 락온 대상 추적 |
| 대상 결정·우선순위 | 소스 작성 | - | 락온·이동 입력 우선순위, 콤보 대상 유지의 런타임(입력 계층 이후) |

에이전트는 빌드·테스트를 실행하지 않았다.

## 남은 제한과 후속 작업

- 입력 방향 전달과 공격 중 이동 입력 처리: [#19](https://github.com/jaykop/Kata/issues/19).
- 몬스터용 `CanKeepActionTarget`·`ResolveFacingDirection` 재정의: [#22](https://github.com/jaykop/Kata/issues/22).
- 시작 조건이 PreCommands보다 먼저 평가되는 기존 제한은 그대로다.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [현재 구현 상태](Implementation-Status.md) | KataTargeting 범위·의존, TG-4 항목, 확인 범위 |
| [타게팅 사용법](../manual/Targeting.md) | 사용 순서, 설정 표, 우선순위, 제한 |
| [타게팅 시스템 설계](../plan/Targeting-Plan.md) | 대상 결정 Command 항목을 구현에 맞게 정정 |
