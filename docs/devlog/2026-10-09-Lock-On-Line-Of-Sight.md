# 락온 시야 조건

작성: 2026-10-09  
갱신: 2026-10-10  
유형: 구현 기록  
대상: KataTargeting `UKataPlayerTargetingComponent`, `UKataTargetingFilterTask_LineOfSight`, 샘플 Targeting Preset  
기준: 이 기록과 함께 커밋한 작업 트리. 같은 파일을 수정한 다른 작업의 미커밋 변경은 포함하지 않는다

## 배경과 결론

락온한 대상과 카메라 사이에 장애물이 있어도 락온이 풀리지 않았다. 처음 락온할 때와 대상을 전환할 때도 벽 너머 대상을 골랐다.
당시 락온 해제 조건은 지점 비활성화, 대상 파괴, 최대 거리, 해제 태그뿐이었다. 시야 조건은 계획에서 "필요할 때 추가"로 미뤄 둔 상태였다.
사용자와 정한 방식으로 시야 조건을 구현했다. 후보 선택은 Preset 필터가 맡고, 락온 유지는 컴포넌트 Tick이 판정한다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 처음 락온·전환 후보 | 엔진과 Kata 필터 모두 가림을 보지 않아 벽 너머 지점도 후보였다 | `Kata Filter Line Of Sight` 추가. 후보 위치까지 트레이스해 가려진 후보를 뺀다 |
| 락온 유지 | `IsLockPointValid`에 시야 검사가 없었다 | Tick에서 시야를 판정하고, 가려진 상태가 유예 시간 동안 이어지면 `LockLostBehavior`를 적용한다 |
| 공용 판정 | 없음 | `KataTargetingView::HasLineOfSight`를 필터와 컴포넌트가 함께 쓴다 |
| 샘플 Preset | 시야 필터 없음 | `TP_KataTest_LockOn`은 Expand Target Points 뒤에, `SwitchLeft`·`SwitchRight`는 Lock Side 뒤에 필터를 넣었다 |

## 주요 결정과 이유

- **카메라 기준 트레이스.** 플레이어가 보는 화면과 판정이 일치해야 한다. 기존 `GetViewPoint`를 그대로 써서 플레이어 폰은 카메라를, 그 밖의 액터는 눈 시점을 쓴다. KataCamera에는 의존하지 않는다.
- **무시 대상.** 3인칭 카메라는 캐릭터 뒤에 있고 지점은 대상 몸 안에 있는 경우가 많다. 그래서 실행 주체와 대상 액터, 두 액터에 붙은 액터를 무시한다.
- **유예 시간은 가려지기 시작한 시각으로 잰다.** 기둥 뒤를 잠깐 지나는 경우를 견딘다. DeltaTime을 누적하지 않고 월드 시각을 기록해 Tick 간격 설정과 상관없이 유예를 잰다. 기본값은 0.5초다.
- **후보 선택은 Preset에 맡긴다.** `IsLockPointValid`에 시야를 넣으면 유예 없이 판정되고, Preset 구성과 별개로 트레이스가 늘어난다. 필터로 두면 디버거 후보 목록과 `SwitchToNext`에도 같은 규칙이 적용된다.
- **시야 판정은 기본으로 켠다.** 사용자가 이 동작을 기대했으므로 기존 컴포넌트도 새 기본값을 따른다.

## 근거

- [KataPlayerTargetingComponent.cpp](../../Plugins/KataTargeting/Source/KataTargeting/Private/Targeting/KataPlayerTargetingComponent.cpp): `UpdateLockLineOfSight`, `TickComponent`, `SetLockPoint`의 시각 초기화.
- [KataTargetingViewUtils.cpp](../../Plugins/KataTargeting/Source/KataTargeting/Private/Targeting/Tasks/KataTargetingViewUtils.cpp): `HasLineOfSight`.
- [KataTargetingFilterTask_LineOfSight.cpp](../../Plugins/KataTargeting/Source/KataTargeting/Private/Targeting/Tasks/KataTargetingFilterTask_LineOfSight.cpp): 후보 필터.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 코드 | 사용자 빌드 보고(2026-10-09) | 빌드 성공 | 없음 |
| 샘플 Preset | 에이전트가 unreal-mcp로 태스크 추가·저장 후 다시 읽음 | 태스크 순서와 기존 태스크 설정 유지 확인 | 없음 |
| 락온 동작 | 사용자 PIE 확인(2026-10-09) | 확인 완료로 보고 | 세부 항목별 결과는 따로 보고받지 않았다 |

## 남은 제한과 후속 작업

- 유예 시간과 채널은 컴포넌트 프로퍼티다. 캐릭터 행 데이터에서 지정하려면 별도 작업이 필요하다.
- 판정이 Tick 간격(0.1초)마다 이뤄지므로 해제가 최대 한 간격 늦을 수 있다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#13](https://github.com/jaykop/Kata/issues/13) | 결과 댓글은 사용자 확인 후 게시 |
| [타게팅 사용법](../manual/Targeting.md) | 설정 표, Preset 예시, 문제 해결 갱신 |
| 타게팅 시스템 설계(삭제, [결정 기록](2026-10-10-Targeting-Decisions.md)으로 이관) | 해제 조건과 시야 조건 결정 갱신 |
