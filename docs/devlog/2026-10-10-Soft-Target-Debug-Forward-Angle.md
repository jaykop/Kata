# 소프트 타겟 디버그 표시와 정면 각도 필터

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 구현 기록, 진단  
대상: KataTargeting GameplayDebugger 카테고리, `UKataTargetingFilterTask_ForwardAngle`, 샘플 소프트 타겟 Preset  
기준: 10e7af6(디버그 범위 표시)과 이 기록을 포함한 후속 커밋

## 배경과 결론

소프트 타겟 범위를 확인할 수단이 없었다. 엔진 CVar `ts.debug.EnableTargetingDebugging`은 Preset이 실행될 때만 AOE 범위를 그린다. 소프트 타겟은 공격을 시작할 때만 갱신되므로 범위가 잠깐 보였다가 사라진다.
GameplayDebugger `KataTargeting` 카테고리가 Soft Target Preset의 AOE 범위와 후보를 계속 그리도록 했다.
몸 정면 기준으로 후보를 좁히는 `Kata Filter Forward Angle` 필터도 추가했고, 디버거에 그 부채꼴을 그리게 했다.

확인 과정에서 두 가지 문제가 드러났다.
- 샘플에 소프트 타겟 Preset이 없어 소프트 타겟이 항상 비어 있었다.
- Preset에서 필터를 정렬 뒤에 두면 정렬 순서가 섞였다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 디버거 소프트 타겟 표시 | 마지막으로 고른 소프트 타겟 이름만 보였다 | AOE 범위(Box·Sphere·Capsule·Cylinder)와 우선순위가 붙은 후보(`S#N`)를 노랑으로 그린다 |
| 정면 각도 필터 | 없었다 | `Kata Filter Forward Angle`: 액터 Forward Vector와 후보 방향의 수평 끼인각이 Max Angle 이하인 후보만 남긴다 |
| 디버거 부채꼴 | 각도 필터의 범위가 보이지 않았다 | Soft Target Preset에 이 필터가 있으면 AOE 수평 도달 거리만큼의 부채꼴을 그린다 |
| 샘플 소프트 타겟 Preset | 없었다. PC BP의 Soft Target Preset이 None이었다 | `TP_SoftTarget`을 만들어 `BP_BlackKnight_PC`·`BP_SilverKnight`에 연결했다 |
| Preset 태스크 순서 | Forward Angle 필터가 정렬 태스크 뒤에 있었다 | 필터를 정렬 앞으로 옮겼다. 규칙은 manual에 적었다 |

## 주요 결정과 이유

- **부채꼴 기준은 카메라가 아니라 몸 정면이다.** 사용자가 Forward Vector 기준을 요청했다. 공격 방향을 정하는 소프트 타겟은 카메라와 관계가 없다는 [기존 결정](2026-10-10-Targeting-Decisions.md)과도 맞는다.
- **각도는 수평면에서 잰다.** 높이 차이 때문에 바로 앞의 큰 대상이나 낮은 대상이 빠지지 않게 하기 위해서다.
- **디버거는 Preset을 실행하지 않고 AOE 범위를 계산한다.** AOE 원점과 회전은 요청 핸들의 소스 Context에서 읽으므로 핸들만 만들었다가 해제한다. 계산 방식은 엔진 `DebugDrawBoundingVolume`과 같다.
  다만 Context를 `UKataTargetingComponent::FindTargets()`와 따로 만든다. 한쪽을 바꾸면 다른 쪽도 맞춰야 표시가 실제와 같다.
- **필터는 정렬보다 앞에 둔다.** 엔진 `UTargetingFilterTask_BasicFilterTemplate::Execute`는 후보를 `RemoveAtSwap`으로 지운다. 지운 자리에 마지막 후보를 옮겨 넣으므로, 정렬 뒤의 필터가 후보를 하나라도 빼면 첫 후보가 가장 우선하는 대상이 아니게 된다.
  정렬 태스크끼리는 정규화 점수를 누적한 뒤 매번 다시 정렬하므로, 서로의 순서는 동점 처리 외에는 결과를 바꾸지 않는다.

## 근거

- [GameplayDebuggerCategory_KataTargeting.cpp](../../Plugins/KataTargeting/Source/KataTargeting/Private/Debug/GameplayDebuggerCategory_KataTargeting.cpp): `CollectSoftTargetRange`, `CollectForwardAngleFan`, `CollectSoftTargetCandidates`.
- [KataTargetingFilterTask_ForwardAngle.cpp](../../Plugins/KataTargeting/Source/KataTargeting/Private/Targeting/Tasks/KataTargetingFilterTask_ForwardAngle.cpp): 코사인 비교로 끼인각을 판정한다.
- 엔진 GameplayTargetingSystem `TargetingFilterTask_BasicFilterTemplate.cpp`의 `RemoveAtSwap`과 `TargetingSortTask_Base.cpp`의 점수 누적·정렬을 읽어 확인했다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 디버거 범위·후보 표시 | 사용자 빌드와 PIE | 2026-10-10 확인 | Box·Capsule·Cylinder 형태 |
| Forward Angle 필터와 부채꼴 표시 | 사용자 빌드와 PIE | 2026-10-10 확인 | 없음 |
| Preset 순서 문제 | 엔진 소스 읽기 | 필터가 순서를 섞는 것을 확인했다 | 순서를 바꾸기 전의 잘못된 선택을 실행으로 재현하지는 않았다 |

## 남은 제한과 후속 작업

- 샘플 공격 `KA_BlackKnight_Attack01`~`04`의 PreCommands에 `Resolve Target`이 없다. 그래서 락온도 이동 입력도 없을 때는 소프트 타겟 쪽으로 돌지 않는다.
- `Kata Sort Screen Center`는 카메라 시선 기준이다. 몸 정면에 가까운 후보를 우선하려면 정면 각도 정렬 태스크가 따로 필요하다. 아직 결정하지 않았다.
- 디버거는 소프트 타겟 Preset의 부채꼴만 그린다. 락온 Preset의 각도 필터는 그리지 않는다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#13 타게팅 시스템](https://github.com/jaykop/Kata/issues/13) | 결과 댓글은 사용자 확인 후 게시 |
| [타게팅 사용법](../manual/Targeting.md) | Forward Angle 태스크, 디버거 표시, 필터·정렬 순서 규칙, 확인 상태 |
| 타게팅 시스템 설계(삭제, [결정 기록](2026-10-10-Targeting-Decisions.md)으로 이관) | 영향 없음. 확정·미확정 사항이 바뀌지 않았다 |
