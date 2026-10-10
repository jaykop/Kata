# 그래프 엣지 직선 그리기와 왕복 엣지 라벨 배치

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 구현 기록  
대상: KataGraphEditor `FKataGraphConnectionDrawingPolicy`, `SKataEdNodeEdge`  
기준: 이 기록과 같은 커밋의 KataGraphEditor 변경

## 배경과 결론

그래프 에디터에서 핀을 끌어 엣지를 만들 때 미리보기 선이 직선이 아니라 곡선으로 그려졌다. Kata 연결 정책에는 직선을 그리는 재정의가 있었지만,
엔진이 호출하지 않는 `FVector2D` 오버로드를 재정의하고 있어서 효과가 없었다. 재정의를 엔진이 실제로 호출하는 `FVector2f` 시그니처로 옮겨
미리보기와 연결된 엣지를 모두 직선으로 그리게 했다.

같은 두 노드를 양방향으로 잇는 엣지는 두 라벨이 선 중앙 근처에서 겹쳤다. 반대 방향 엣지가 있을 때 라벨 전체가 자기 선 쪽에 놓이도록 배치를 바꿨다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 연결 정책 오버로드 | `DrawSplineWithArrow`·`DrawPreviewConnector`·`ComputeSplineTangent`를 `FVector2D` 시그니처로 재정의했다. UE 5.6부터 엔진은 `FVector2f` 오버로드만 호출하므로 드래그 프리뷰는 기본 곡선 경로를 탔고, `DrawConnection`의 접선 계산도 에디터 설정의 기본 접선을 썼다 | 세 함수와 `Internal_DrawLineWithArrow`를 `FVector2f` 시그니처로 옮겼다. `ComputeSplineTangent`는 시작점에서 끝점으로 향하는 단위 벡터를 반환해 Hermite 스플라인이 사실상 직선이 된다 |
| 왕복 엣지 라벨 | 라벨 중심을 선 중앙에서 법선 방향으로 30px 띄웠다. 세로 연결에서는 라벨 반폭(약 75px)이 30px보다 커서 반대 방향 라벨과 겹쳤다 | `PerformSecondPassLayout`이 반대 방향 엣지를 찾으면 `PositionBetweenTwoNodesWithOffset`이 선 간격 4.5px + 여백 4px + 법선 방향으로 투영한 라벨 반폭만큼 띄운다. 한 방향 엣지는 기존 30px 배치를 유지한다 |

## 주요 결정과 이유

- 폐기된 `FVector2D` 오버로드는 기본 빌드 설정에서 `final`이 아니라서 재정의해도 컴파일 오류가 나지 않는다. 대신 엔진이 호출하지 않아 조용히 기본 동작으로 돌아간다. Slate·그래프 에디터 가상 함수를 재정의할 때는 엔진 헤더에서 `FVector2f` 오버로드가 있는지 먼저 확인한다.
- 라벨 오프셋은 법선 방향으로 투영한 라벨 크기로 계산한다. 세로 연결은 라벨이 선의 좌우로, 가로 연결은 위아래로 나뉘고 대각선은 그 중간 값이 된다. 반대 방향 엣지는 법선 부호가 반대라서 서로 반대편에 놓인다.
- 한 방향 엣지까지 같은 계산을 적용하면 기존 그래프의 라벨 위치가 모두 바뀐다. 이번 요청 범위가 겹침 해소라서 반대 방향 엣지가 있을 때만 적용했다.

## 근거

- [KataGraphConnectionDrawingPolicy.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/KataGraphConnectionDrawingPolicy.cpp): 직선 그리기, 화살표, 접선 재정의.
- [SKataEdNodeEdge.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/SKataEdNodeEdge.cpp): 반대 방향 엣지 탐지와 라벨 오프셋 계산.
- UE 5.8 `Engine/Source/Editor/GraphEditor/Public/ConnectionDrawingPolicy.h`: `FVector2D` 오버로드의 5.6 폐기 표시와 `FVector2f` 오버로드. `ConnectionDrawingPolicy.cpp`의 기본 `DrawPreviewConnector`·`DrawConnection`이 `FVector2f` 경로만 호출한다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 직선 그리기 | 사용자 에디터 확인(2026-10-10 스크린샷) | 엣지가 직선으로 그려졌다 | — |
| 전체 변경 | 사용자 빌드 보고(2026-10-10) | 빌드 통과 | 왕복 엣지 라벨 배치의 실행 화면 확인은 보고되지 않았다 |

## 남은 제한과 후속 작업

- 같은 방향으로 여러 엣지가 있으면서 반대 방향 엣지도 있을 때, 같은 방향 엣지끼리는 기존처럼 선을 따라 나란히 놓인다. 이 조합의 화면은 확인하지 않았다.
- 선 간격 4.5px는 연결 정책의 `LineSeparationAmount`와 같은 값을 따로 적었다. 한쪽을 바꾸면 다른 쪽도 맞춰야 한다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [에디터 사용법](../manual/Editor-Usage.md) | 영향 없음. 엣지 그리기 방식은 사용법에 적혀 있지 않고 조작은 바뀌지 않았다 |
