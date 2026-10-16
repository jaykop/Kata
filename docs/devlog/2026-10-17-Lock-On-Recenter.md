# 락온 대상이 없을 때 카메라 중앙 정렬

작성: 2026-10-17  
갱신: 2026-10-17  
유형: 구현 기록 / 결정 기록  
대상: KataCamera `AKataPlayerCameraManager`, KataFramework `UKataInputHandlerComponent`  
기준: 이 기록과 같은 커밋의 작업 트리

## 배경과 결론

락온 입력을 눌러도 락온할 대상이 없으면 아무 일도 일어나지 않았다. 시점을 돌려 둔 상태에서 캐릭터가 향한 쪽을 다시 보려면 시점을 직접 돌려야 했다.
락온 획득에 실패하면 카메라를 캐릭터 정면으로 돌리는 중앙 정렬을 추가했다. 카메라 매니저가 정렬 API와 설정을 갖고, 입력 처리 컴포넌트가 락온 획득 실패를 확인해 정렬을 요청한다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 락온 토글 실패 | `AcquireLock()`이 false를 돌려줘도 아무것도 하지 않았다 | `UKataInputHandlerComponent::HandleToggleLock`이 폰의 정면 Yaw로 `AKataPlayerCameraManager::StartRecenter`를 호출한다 |
| 정렬 API | 없음 | `StartRecenter(TargetYaw)`, `StopRecenter()`, `IsRecentering()`. Blueprint에서도 호출할 수 있다 |
| 정렬 설정 | 없음 | 매니저의 Kata\|Camera\|Recenter. Duration 0.25초, Curve(비면 Ease In-Out), Pitch -10도, Cancel Angle 2도 |
| 정렬 진행 | 없음 | 시작 순간의 컨트롤 회전에서 목표 Yaw·Pitch까지 최단 각도로 블렌드하고, Rotation 단계 전에 컨트롤 회전에 기록한다 |
| 정렬 취소 | 없음 | 누적 시점 입력이 Cancel Angle을 넘거나, 락온 초점이 생기거나, 폰이 아닌 대상을 보거나, 매니저가 종료되면 멈춘다 |
| 곡선 검증 | Default Lock On Settings의 Blend In Curve만 검증했다 | Recenter Curve도 같은 규칙(단조 증가, 0→0·1→1)으로 경고한다 |

## 주요 결정과 이유

- 진입 경로: KataCamera는 KataTargeting을 참조하지 않는다. 락온 결과를 아는 KataFramework의 입력 처리 컴포넌트가 카메라 API를 호출한다. `AKataPlayerController`가 락온 초점을 카메라에 넘기는 기존 경로와 의존 방향이 같다. 카메라에는 정렬할 Yaw만 넘기므로 락온이 아닌 다른 경로에서도 같은 API를 쓸 수 있다.
- 목표 고정: 목표 Yaw는 호출 순간의 폰 정면으로 고정한다. 정렬 중 폰이 돌아도 목표를 따라가지 않으므로 정렬 시간이 일정하다.
- Pitch: 사용자가 매니저 설정값으로 맞추는 안을 확정했다(2026-10-17). 목표 Pitch는 활성 카메라 데이터의 Pitch 범위로 자른다. 범위 밖을 목표로 돌면 정렬이 끝난 뒤 첫 시점 입력에서 Pitch가 범위 안으로 튀기 때문이다.
- 정렬 위치: 정렬은 컨트롤 회전 자체를 바꾸므로 매니저가 Rotation 단계보다 먼저 처리한다. 그 뒤 Rotation 단계의 Feature는 정렬된 컨트롤 회전을 입력으로 받는다.
- 입력 감지 위치: 엔진 `APlayerController::UpdateRotation`은 이번 프레임의 회전 입력을 `ProcessViewRotation`의 `OutDeltaRot`로 넘긴 뒤 컨트롤 회전에 더한다. 카메라 매니저의 뷰 갱신은 그 뒤에 실행되므로 그때는 입력이 이미 비어 있다. 그래서 매니저가 `ProcessViewRotation`을 override해 입력을 센다. 입력 처리 컴포넌트의 Look에서 세면 Blueprint의 Add Controller Yaw Input처럼 다른 경로로 들어온 시점 입력을 놓친다.
- 취소 임계값: 게임패드의 락온 토글은 오른쪽 스틱 누름(R3)이다. 누를 때 스틱이 살짝 기울면 시점 입력이 들어가 정렬이 바로 취소될 수 있다. 그래서 정렬 중 누적한 Yaw·Pitch 입력의 절댓값 합이 Cancel Angle을 넘을 때만 취소한다. 사용자가 2도 안을 확정했다(2026-10-17). 0이면 어떤 입력에도 취소한다. 취소한 프레임의 입력은 그대로 적용되므로 정렬이 멈춘 회전에서 시점이 이어진다.
- 락온 우선: 정렬 중 락온 초점을 받으면 `SetLockOnFocus`가 정렬을 멈춘다. 락온 전환은 그 순간의 컨트롤 회전에서 시작하므로 회전이 끊기지 않는다. 락온 초점이 있을 때 `StartRecenter`는 false를 돌려준다.
- 정렬을 끄는 설정은 요청 범위 밖이라 두지 않았다.

## 근거

- [KataPlayerCameraManager.h](../../Plugins/KataCamera/Source/KataCamera/Public/KataPlayerCameraManager.h): Recenter 설정과 `StartRecenter` 계약.
- [KataPlayerCameraManager.cpp](../../Plugins/KataCamera/Source/KataCamera/Private/KataPlayerCameraManager.cpp): `StartRecenter`·`UpdateRecenter`·`ProcessViewRotation`, `SetLockOnFocus`의 정렬 중단, `UpdateViewTargetInternal`의 호출 순서.
- [KataInputHandlerComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Input/KataInputHandlerComponent.cpp): `HandleToggleLock`의 획득 실패 분기.
- 엔진 `PlayerController.cpp`의 `APlayerController::UpdateRotation`: `RotationInput`을 `ProcessViewRotation`에 넘긴 뒤 `SetControlRotation`을 호출하는 순서.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 구현 | 사용자 Editor 빌드와 PIE(2026-10-17) | 제시한 확인 항목(대상 없을 때 정렬, 시점 입력 취소, R3 입력, 정렬 중 락온, 기존 락온 회귀)의 통과를 보고했다 | 항목별 관찰 결과는 따로 보고되지 않았다. Game 타깃과 Spline 배치에서의 정렬은 확인 전이다 |

에이전트는 빌드·테스트·별도 검사를 실행하지 않았다.

## 남은 제한과 후속 작업

- 정렬을 끄는 설정은 없다. 필요해지면 후속 이슈로 정한다.
- GameplayDebugger 카메라 카테고리는 정렬 상태를 표시하지 않는다.
- 정렬 목표는 폰 Actor 회전의 Yaw다. 메시만 따로 돌리는 폰에서는 보이는 정면과 다를 수 있다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [연결 이슈 #123](https://github.com/jaykop/Kata/issues/123) | 구현과 사용자 빌드·PIE 확인 완료. 결과 댓글은 사용자 확인 후 게시 |
| [카메라 사용법](../manual/Camera.md) | "대상이 없을 때 중앙 정렬" 단락 추가 |
| [입력 사용법](../manual/Input.md) | Toggle Lock Action의 실패 시 동작에 정렬 추가 |
| [카메라 시스템 계획](../plan/Camera-Plan.md) | 영향 없음. 이 기능은 계획 범위 밖의 후속 기능이다 |
