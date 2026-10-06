# 스포너 배치 실행 분리

작성: 2026-10-06  
갱신: 2026-10-06  
유형: 구현 기록  
대상: KataFramework 스포너·캐릭터 생성 서비스  
기준: 1단계 구현 변경. 사용자 빌드 성공 보고, 실행 미확인.

## 배경과 결론

사용자는 플레이어 위치 기반 스폰·디스폰과 타임슬라이싱을 단계적으로 구현하기로 하고, 배치 실행 분리에 대한 브리핑 후 1단계 시작을 요청했다.

스포너의 준비·위치 계산·제출·결과·종료 처리를 분리하고 진행 커서를 추가했다. 이번 단계에서는 분리한 작업을 같은 호출에서 처리한다. 월드 예산이나 거리 활성화 기능을 구현한 것으로 보지 않는다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 시작 정보 | ID·설정 복사, 행은 개별 요청마다 조회 | FKataSpawnBatchContext에 NPC 행과 스포너 Transform까지 고정 |
| 배치 수명 | 액터의 개별 배열·카운터 | UKataSpawnBatchState에 모아 GC 추적, 호출 중 강한 참조 유지 |
| 위치 계산 | 개체·시도·옵션 중첩 반복문 | 후보 선택·옵션 하나 보정·후보 확정을 진행 커서로 분리 |
| 대기 집계 | 전체 개체의 예약 맵 크기 | RemainingCount로 미제출 개체 포함, 제출 직전에 요청 하나만 예약 |
| 행 기반 생성 | ID로 테이블 재조회 | RequestSpawnFromRow 추가. 기존 RequestSpawn은 기존 조회 경로 유지 |
| NavMesh | 스포너의 현재 스케일 | 기본 배치 보정에서는 시작 시 고정한 Transform 사용 |

## 주요 결정과 이유

전체 후보 확인 후 제출이라는 기존 계약을 보존하기 위해 준비 위치 배열을 유지했다. 배열 제거와 실제 프레임 분산은 관리자를 연결하는 후속 단계에 맡긴다.

새 배치 Context 매개변수로 기존 Blueprint 이벤트를 교체하지 않았다. C++ 구현은 GetSpawnBatchContext를 조회하고 기존 위치 보정 이벤트를 그대로 호출한다. 따라서 기존 Blueprint가 현재 액터 상태를 직접 읽는 계산까지 고정해 주지는 않는다.

실행 객체는 설정 객체와 별개다. UObject 참조를 리플렉션 프로퍼티로 보관하고 준비·제출·결과·종료 함수의 강한 참조로 콜백 중 취소·참조 해제에도 현재 실행 데이터를 유지한다. 취소는 여전히 이미 생성한 NPC를 제거하지 않는다.

일반 생성 API의 행은 준비한 사본을 내부 공용 요청 함수로 이동시켜, 새 연결 때문에 행 사본 생성이 불필요하게 한 번 더 늘어나지 않게 했다.

## 근거

- [배치 타입](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataSpawnBatchTypes.h).
- [스포너 실행](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataCharacterSpawner.cpp).
- [행 사본 요청](../../Plugins/KataFramework/Source/KataFramework/Public/Character/KataCharacterSpawnSubsystem.h).
- [NavMesh 보정](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataSpawnerComponent_NavMeshProjection.cpp).

## 확인 범위와 결과

에이전트가 관련 소스를 읽고 구현했다. 에이전트는 빌드·테스트·lint·정적 분석·별도 리뷰·PIE·성능 측정을 실행하지 않았다.

2026-10-06 사용자가 1단계 변경의 빌드 성공을 보고했다. 빌드 타깃은 지정하지 않았다. 생성·실패·취소 동작의 사용자 실행 결과는 아직 보고되지 않았다.

## 남은 제한과 후속 작업

- 위치 준비와 요청 제출은 여전히 같은 호출에서 처리하며 로드 완료 후 실제 생성도 기존 경로다.
- 관리자의 예산·준비 완료 생성 큐, 디스폰과 그리드는 후속 단계다.
- 사용자 확인 시 기존 생성·0개 완료·개별 실패·결과 이벤트 중 취소·EndPlay의 동작과 대기 수를 확인한다.
- 이번 작업에 직접 맞는 열린 이슈는 없어 공개 초안을 준비했다. 승인을 받기 전 게시하지 않는다.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [스포너 사용법](../manual/Spawner.md) | 고정 행·Transform, 대기 수, 기존 Blueprint 훅의 범위와 실행 미확인 |
| [전체 계획](../plan/Spawner-Scheduling-And-Grid-Plan.md) | 1단계 호환성 경계와 2단계 이후 분산 경로 구분 |
| [캐릭터 데이터 사용법](../manual/Character-Data.md) | 기존 단일 생성 노드의 사용 계약은 바뀌지 않아 수정하지 않음 |
| [문서 목록](../README.md) | 이번 기록 링크 추가 |
