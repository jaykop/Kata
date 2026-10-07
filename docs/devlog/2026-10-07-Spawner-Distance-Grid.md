# 거리 관리 스포너의 그리드 조회

작성: 2026-10-07  
갱신: 2026-10-07  
유형: 구현 기록  
대상: KataFramework 월드 스포너 관리자  
기준: `5e890fe` 이후 미커밋 작업 트리. 다른 작업자의 GAS Inspector·Config 변경은 포함하지 않음

## 배경과 결론

스포너 개선의 원래 요청인 그리드 기반 작동을 구현했다. [거리 활성화](2026-10-07-Spawner-Distance-Activation.md)는 모든 거리 관리 스포너를 평가 주기마다 순회했다. 이제 관리자가 스포너를 월드 XY 셀에 등록하고, 플레이어 주변 셀과 활성 스포너만 평가한다. 거리 판정 규칙은 바꾸지 않았다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 평가 대상 | 등록된 모든 거리 관리 스포너 | 플레이어 셀 주변 스포너 + 활성 스포너 |
| 등록 | 약한 참조 목록 | BeginPlay 위치의 셀과 스포너별 셀 기록 |
| 셀 크기 | 없음 | `Grid Cell Size` 설정, 기본 5000cm, 월드 시작 때 고정 |
| 활성 판정 | 없음 | 원점 범위 안, 생성 중, NPC 기록 또는 진행 중인 거리 제거 배치가 있으면 활성 |

## 주요 결정과 이유

- 셀은 XY만 쓴다. 높이 차이는 실제 판정의 3D 거리가 처리하며, 셀은 후보를 줄이는 데만 쓴다.
- 조회 반경은 등록된 가장 큰 SpawnDistance를 셀 크기로 나눠 올림한 값이다. 해제 때 줄이지 않아 반경이 넓어질 수는 있어도 범위 안 스포너를 놓치지 않는다.
- 생존 NPC·원점 이탈 판정은 셀 조회와 독립이다. 원점에서 멀리 이동한 NPC나 떠나는 원점의 이탈을 놓치지 않도록 활성 스포너를 별도 목록으로 매 평가에 포함한다.
- 거리 제거 배치가 Destroy 거절 기록을 늦게 돌려줄 수 있어, 진행 중인 거리 제거 배치 수를 활성 조건에 넣었다. 빠지면 살아 남은 NPC가 조회 밖에서 영구히 평가되지 않는다.
- 셀 크기는 등록된 좌표와 맞아야 하므로 월드 수명 동안 고정한다. 스포너 이동은 셀 갱신 없이 지원하지 않으며 고정 배치를 전제로 한다.
- 조회 셀 수가 점유 셀 수보다 많으면 점유 셀을 반경으로 거른다. 셀 크기에 비해 거리가 매우 큰 설정에서 빈 셀 탐색을 피한다.
- 그리드는 기존 `UKataSpawnerSubsystem` 안에 두었다. 별도 Grid Subsystem을 만들지 않는다는 계획의 책임 경계를 따른다.

## 근거

- [월드 관리자](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataSpawnerSubsystem.cpp): `GetDistanceCell`, `RegisterDistanceSpawner`, `BuildDistancePass`, `ProcessDistanceChecks`.
- [스포너](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataCharacterSpawner.cpp): `HasActiveDistanceState`, 거리 제거 배치 수.
- [공용 설정](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataSpawnerSettings.h): `GridCellSize`.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 구현 | 에이전트 소스 작성 | 셀 등록·조회·활성 목록 | 성능 측정 |
| 실행 | 사용자 빌드·PIE 보고 (2026-10-07) | 거리 생성·제거가 그리드 추가 전과 동일하게 동작 | 빌드 타깃, 셀 경계·다수 스포너 시나리오 |

에이전트는 빌드·테스트·별도 검사를 실행하지 않았다.

## 남은 제한과 후속 작업

- 이동하는 거리 관리 스포너의 셀 갱신.
- 에디터·GameplayDebugger의 셀과 거리 범위 표시.
- 셀 크기와 평가 주기의 실제 부하 측정.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| 연결 이슈 | 미게시. 로컬 초안의 그리드 항목 갱신 |
| [스포너 사용법](../manual/Spawner.md) | 그리드 절, Grid Cell Size 설정, 제한 갱신 |
| [전체 계획](../plan/Spawner-Scheduling-And-Grid-Plan.md) | S5 정의와 같아 변경 없음 |
| [문서 목록](../README.md) | 기록 링크 추가 |
