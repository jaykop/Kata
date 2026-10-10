# Shrink의 피벗 선행 스윕

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 진단 / 구현 기록  
대상: KataCamera `UKataCameraFeature_Shrink`, KataCamera GameplayDebugger 카테고리  
기준: 이 기록과 같은 커밋의 작업 트리

## 배경과 결론

캐릭터가 벽을 정면으로 보고 붙어 서면 카메라가 갑자기 벽 안으로 들어가는 문제가 보고됐다.
원인은 Boom Arm 배치의 `PivotOffset.X`가 100으로 설정되어 피벗이 캐릭터 캡슐보다 앞쪽에 놓인 데이터 값이었다.
사용자가 값을 고쳐 증상은 해결됐다. 같은 문제가 다른 오프셋으로 다시 생기지 않도록 Shrink가 뷰 타깃 위치에서 피벗까지 먼저 스윕하게 했다.
오프셋을 눈으로 확인할 수 있게 GameplayDebugger에 기준점과 피벗을 잇는 선도 추가했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| Shrink 스윕 구간 | 피벗에서 카메라까지만 스윕했다. 오프셋이 더해진 피벗이 벽 안이나 너머에 있어도 뷰 타깃과 피벗 사이는 검사하지 않았다 | 뷰 타깃 위치 → 피벗을 먼저 스윕하고, 막히면 그 프레임의 스윕 시작점을 충돌 지점으로 당긴 뒤 카메라까지 스윕한다 |
| 시작부터 겹친 스윕 | 피벗의 구가 벽과 겹치면 `Hit.Time`이 0이 되어 카메라가 피벗에서 `MinDistance` 떨어진 곳, 즉 벽 안으로 한 프레임에 당겨졌다 | 선행 스윕이 피벗을 장애물 밖으로 빼내므로 이 경로는 뷰 타깃 위치 자체가 지형에 걸친 경우에만 남는다 |
| 선행 스윕 직후의 떨림 | 첫 구현은 피벗을 구가 표면에 정확히 닿는 지점으로 당겼다. 두 번째 스윕이 그 지점에서 시작해 프레임마다 시작 겹침 판정이 바뀌었고, 카메라가 `MinDistance`와 원래 거리 사이를 오가며 튀었다 | 당긴 지점에서 기준점 쪽으로 2cm 더 물러나 표면에서 띄운다 |
| Shrink 디버그 문자열 | `ratio`와 `blocked`/`clear`만 표시했다 | 선행 스윕이 막히면 `pivot-blocked`를 붙인다 |
| GameplayDebugger 피벗 표시 | 오프셋이 적용된 피벗에 노란 점만 그렸다 | 뷰 타깃 위치에 흰 점 `Pivot Base`를 그리고 피벗까지 노란 선으로 잇는다. 스냅샷에 `ViewTargetLocation`을 추가했다 |

## 주요 결정과 이유

- 선행 스윕의 시작점은 `ViewTarget->GetActorLocation()`이다. Character에서는 캡슐 중심이라 정상 플레이에서 지형 밖에 있다고 볼 수 있다. 피벗 래그가 없는 실제 위치이므로 `PivotOffset`, Spline 레일 원점, 피벗 래그 어느 것이 피벗을 밀어내도 같은 방식으로 막힌다.
- 당긴 시작점은 Shrink 안에서만 쓴다. `Context.PivotLocation`과 카메라 회전은 바꾸지 않아, 피벗이 장애물 밖일 때는 결과가 이전과 같고 구도·조준선에도 영향이 없다.
- 물러나는 거리 2cm는 설정값이 아닌 코드 상수로 두었다. 표면 접촉 판정의 오차만 피하면 되는 값이고, 선행 스윕이 막혔을 때만 적용된다.
- 선행 스윕에는 보간을 두지 않았다. 카메라 위치는 두 번째 스윕이 막힐 때만 달라지고, 그 비율 보간은 기존 설정을 그대로 따른다.
- 사용자는 피벗을 캐릭터 BP의 전용 컴포넌트로 정의하고 오프셋을 그 기준으로 적용하는 구조를 제안했다. 이번 원인이 구조가 아니라 값이었고, 구조를 바꿔도 컴포넌트를 앞쪽에 두면 재발하므로 보류했다. 체형이 다른 플레이어 캐릭터가 늘어나거나 배치 간 피벗 불일치가 실제로 생기면 다시 검토한다.
- `PivotOffset`에 피벗을 캡슐 안에 두라는 경고 주석을 다는 안은 사용자가 넣지 않기로 했다.

## 근거

- [KataCameraFeature_Shrink.cpp](../../Plugins/KataCamera/Source/KataCamera/Private/KataCameraFeature_Shrink.cpp): `Evaluate`의 선행 스윕과 시작 겹침 처리.
- [KataCameraPlacement.cpp](../../Plugins/KataCamera/Source/KataCamera/Private/KataCameraPlacement.cpp): Boom Arm이 카메라 Yaw 기준으로 `PivotOffset`을 더하고, Spline이 레일 원점을 피벗으로 쓰는 위치.
- [KataPlayerCameraManager.cpp](../../Plugins/KataCamera/Source/KataCamera/Private/KataPlayerCameraManager.cpp): `ApplyPivotLag`와 디버그 스냅샷 기록.
- [GameplayDebuggerCategory_KataCamera.cpp](../../Plugins/KataCamera/Source/KataCamera/Private/Debug/GameplayDebuggerCategory_KataCamera.cpp): `Pivot Base`·피벗 선 표시.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 원인 | 소스 읽기와 사용자 데이터 확인 | `PivotOffset.X` 100을 고친 뒤 증상이 사라졌다고 사용자가 보고했다 | 없음 |
| 선행 스윕·디버그 표시 | 사용자 Editor 빌드와 PIE | 첫 구현은 `PivotOffset.X` 100 재현에서 카메라가 튀었다. 2cm 물러남을 넣은 뒤 빌드 통과와 떨림 없음을 보고했다 | 벽·천장 당김 회귀, Game 타깃 |

## 남은 제한과 후속 작업

- 뷰 타깃 위치 자체가 지형에 걸친 경우에는 여전히 카메라가 `MinDistance`까지 당겨진다.
- `ProbeRadius` 기본값 12cm는 FOV 90°, 16:9 기준 근평면 모서리 거리(약 15cm)보다 작아, 벽에 바짝 붙었을 때 화면 가장자리가 잘려 보일 수 있다. 이번 변경 범위에는 넣지 않았다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [연결 이슈 #20](https://github.com/jaykop/Kata/issues/20) | CAM-3 Shrink 보완. 결과 댓글은 사용자 확인 후 게시 |
| [카메라 사용법](../manual/Camera.md) | Shrink 스윕 설명, `Pivot Base` 표시, 확인 상태 갱신 |
| [카메라 시스템 계획](../plan/Camera-Plan.md) | 레일 피벗 결정은 유지하므로 영향 없음 |
