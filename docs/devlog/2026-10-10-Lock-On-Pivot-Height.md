# 락온 방향의 피벗 높이 기준

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 진단 / 구현 기록  
대상: KataCamera `AKataPlayerCameraManager` 락온 회전  
기준: 이 기록과 같은 커밋의 작업 트리

## 배경과 결론

BlackKnight에 락온한 채 다가갈수록 카메라가 아래로 내려가는 문제가 보고됐다.
GameplayDebugger로 보면 락온 지점(`Lock Focus`)이 피벗(노란 점)보다 낮았으므로 카메라는 위에서 내려다봐야 했다.
원인은 락온 방향의 시작점이었다. 락온 회전을 카메라가 도는 피벗이 아니라 뷰 타깃 위치(`Pivot Base`, 캡슐 중심)에서 쟀다.
락온 지점이 두 점 사이 높이에 있으면 피벗 기준으로는 내려다봐야 하지만, 계산은 올려다보는 Pitch를 냈다. Boom Arm은 이 Pitch로 피벗을 돌아 카메라를 아래로 보냈다. 가까울수록 각도가 커져 더 내려갔다.
락온 방향의 높이를 피벗 높이에 맞춰 고쳤다.

처음에는 이 동작을 설계대로라고 잘못 진단했다. `PitchOffsetByDistance` 주석이 "락온 Pitch가 카메라 위치를 움직인다"는 전제를 적고 있었기 때문이다. 이 전제 자체는 맞다. 하지만 Pitch를 잴 시작점이 궤도 중심과 어긋나 있었다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 락온 방향 시작점 | `ViewPawn->GetActorLocation() + PivotLagOffset`. Boom Arm의 `PivotOffset`이나 Spline 레일 원점 높이를 반영하지 않았다 | 시작점에 `PivotHeightFromPawn`(피벗의 폰 기준 높이)을 더한다 |
| 피벗 높이 값 | 매니저는 배치 평가 전에 락온 회전을 구하므로 그 시점의 피벗을 알 수 없었다 | 매 프레임 블렌드된 결과 피벗(래그 적용 전)의 높이를 저장하고, 다음 프레임 락온 방향에 쓴다. 파이프라인이 끊기거나 매니저가 정리되면 0으로 되돌린다 |
| 거리 곡선의 거리 | 캡슐 중심에서 락온 지점까지 쟀다 | 같은 시작점에서 잰다. `... By Distance` 곡선 값이 조금 달라질 수 있다 |
| 디버그 조준선 | 주황 선은 원래 피벗에서 그려 실제 계산 방향과 달랐다 | 코드 변경 없음. 이제 높이 기준이 맞아 표시와 계산이 같은 Pitch를 가리킨다 |

## 주요 결정과 이유

- 피벗의 높이만 반영하고 수평 오프셋은 넣지 않았다. Boom Arm의 `PivotOffset` X/Y는 시점 Yaw에 따라 돈다. 이것을 방향에 넣으면 방향이 Yaw를, Yaw가 피벗을, 피벗이 다시 방향을 바꾸는 되먹임이 생긴다. 피벗 래그가 피벗 대신 폰 위치를 따라가도록 한 것도 같은 이유였다. 이번 문제의 원인은 높이 차이였으므로 높이만 맞추면 된다.
- 직전 프레임 값을 쓴다. Boom Arm의 `PivotOffset.Z`는 고정값이고, 배치 블렌드 중에도 높이가 한 프레임 사이에 크게 변하지 않는다. 배치 평가 순서를 바꾸거나 Placement에 피벗 질의 함수를 추가하는 방안보다 변경이 작다.
- 고려했지만 채택하지 않은 대안이 두 가지 있다.
  - CameraOffset 추가: 오프셋은 궤도 Pitch를 바꾸지 못하므로 원인을 해결하지 않는다.
  - 궤도 Pitch를 락온 방향에서 분리: 락온은 Yaw만 돌리고 대상을 올려다보는 것은 Framing의 `CameraRotation`이 맡는 설계다. 높이 기준을 고쳐 증상이 사라졌으므로 진행하지 않았다.

## 근거

- [KataPlayerCameraManager.cpp](../../Plugins/KataCamera/Source/KataCamera/Private/KataPlayerCameraManager.cpp): `UpdateLockOn`의 `Direction` 계산과 `UpdateViewTargetInternal`의 `PivotHeightFromPawn` 기록.
- [KataCameraPlacement.cpp](../../Plugins/KataCamera/Source/KataCamera/Private/KataCameraPlacement.cpp): Boom Arm이 `PivotLocation - ViewRotation.Vector() * Distance`로 카메라를 두는 위치.
- [GameplayDebuggerCategory_KataCamera.cpp](../../Plugins/KataCamera/Source/KataCamera/Private/Debug/GameplayDebuggerCategory_KataCamera.cpp): `Pivot Base`·피벗·`Lock Focus` 표시.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 원인 | 사용자 GameplayDebugger 스크린샷과 소스 읽기 | `Lock Focus`가 `Pivot Base`보다 높고 피벗보다 낮은 상태에서 카메라가 내려갔다 | 없음 |
| 수정 | 사용자 Editor 빌드와 PIE | 빌드 통과. BlackKnight에 다가가도 카메라가 내려가지 않는다고 보고했다 | Game 타깃, 피벗보다 높은 락온 지점, Spline 배치 |

## 남은 제한과 후속 작업

- `PivotOffset` X/Y가 큰 배치에서는 락온 Yaw와 거리가 실제 궤도 중심과 조금 어긋난다.
- `ChooseLockOnSide`의 좌우 판정은 여전히 뷰 타깃 위치에서 잰다. Yaw만 쓰므로 높이 변경과는 관계가 없다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [연결 이슈 #20](https://github.com/jaykop/Kata/issues/20) | 카메라 시스템 이슈. 결과 댓글은 사용자 확인 후 게시 |
| [카메라 사용법](../manual/Camera.md) | 락온 방향의 기준점과 거리 곡선의 기준을 추가 |
