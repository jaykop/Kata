# 스포너 관리자 구성과 타임슬라이싱 계획 종료

작성: 2026-10-16  
갱신: 2026-10-16  
유형: 결정 기록  
대상: KataFramework 스포너 관리자, 생성 Subsystem, 거리 활성화와 그리드  
기준: [#76](https://github.com/jaykop/Kata/issues/76)에서 삭제한 `docs/plan/Spawner-Scheduling-And-Grid-Plan.md`(2026-10-06 작성, 2026-10-07 갱신)의 구성 결정을 옮긴 기록. 계획에는 연결 이슈가 없었다

## 배경과 결론

사용자의 원래 요청은 스포너의 타임슬라이싱과 그리드 기반 작동이었고, 작업 중 플레이어 거리 기반 생성·제거가 더해졌다. 2026-10-07 사용자는 거리 컴포넌트 → 그리드 → (선택) 상태 보존 순서를 승인했다. 상태 보존 없는 거리 활성화를 먼저 제공했고, 상태 보존 재생성은 [휴면 계약 기록](2026-10-07-Spawner-Dormancy-Contract.md)의 후속 검토안으로 남겼다.

계획의 S1~S5는 단계별 기록으로 구현됐다. 이 기록은 그 단계들이 공유하는 구성 결정과 단계별 기록의 위치를 모은다. 현재 API와 설정은 [스포너 사용법](../manual/Spawner.md)을 따른다.

## 주요 결정과 이유

| 항목 | 결정 | 이유 |
|---|---|---|
| 관리자 | 새 `UKataSpawnerSubsystem`이 공간 인덱스, 플레이어 위치 관찰, 활성화 판단, 위치 준비·디스폰 작업과 공용 프레임 예산을 맡는 유일한 스케줄러다 | 단일 캐릭터 생성 API에 플레이어·거리·스포너 정책을 결합하지 않는다. 같은 관리자가 거리와 타임슬라이싱을 함께 맡으므로 Grid·TimeSlicing Subsystem을 따로 두지 않는다 |
| 생성 Subsystem | `UKataCharacterSpawnSubsystem`은 거리와 플레이어를 모른다. 분산 요청은 로드 완료 후 준비 큐에 두고 관리자가 남은 예산으로 생성시킨다. 자체 Tick·예산은 없다 | 두 관리자가 생성 비용을 중복 집행하지 않는다. 준비 큐는 요청 그룹 핸들로 구분해 스포너 참조를 저장하지 않는다 |
| 스포너 | `AKataCharacterSpawner`가 설정 사본, 고정한 행·영역, 배치 ID, 진행 커서와 생성 NPC 기록을 소유한다. 액터마다 Tick·거리 타이머를 두지 않는다 | 거리 상태는 관리자가 결정하고 NPC 참조와 배치 실행 상태는 스포너가 소유한다. 같은 상태를 양쪽에 복제하지 않는다 |
| 거리 설정 | 기존 인라인 스포너 컴포넌트 UObject로 추가한다. 별도 ActorComponent는 두지 않는다 | 전용 스포너에 붙이는 범위이고 인라인 옵션 확장 계약이 이미 있다. 일반 Actor에도 필요해지면 독립 ActorComponent를 검토한다 |
| 이탈 기준 | PC Pawn과 스폰된 각 NPC의 현재 위치 사이 거리(사용자 요구). 전투 여부를 따로 판정하지 않는다 | 원점에서 이동한 NPC도 플레이어 가까이 있으면 유지된다 |
| 거리 계산 | XY 그리드로 후보를 찾고 실제 3D 거리로 판정한다 | 다층 지형에서 XY만으로 활성화하지 않는다 |
| 경계 | `DespawnDistance > SpawnDistance` | 경계 왕복 전환을 막는다 |
| 플레이어 미준비 | 새 활성화를 보류하고 기존 NPC를 유지한다 | 플레이어 상실을 거리 이탈로 해석하지 않는다 |
| World Partition | 로드된 스포너만 등록한다 | 언로드를 넘는 세대·사망 상태 저장은 별도 범위다([#39](https://github.com/jaykop/Kata/issues/39), [#88](https://github.com/jaykop/Kata/issues/88)) |

제외 범위: NPC 격자 배치·간격 보장, 풀링, 사망 시스템 자체, 세이브·언로드를 넘는 상태 보존, 자동 결원 보충, World Partition 자체 구현.

## 단계별 기록

| 계획 ID | 작업 | 기록 |
|---|---|---|
| S1 | 배치 실행 상태와 고정 Context 분리 | [배치 실행 분리](2026-10-06-Spawner-Batch-Execution.md) |
| S2 | 관리자와 생성 서비스 연결, 공용 예산 | [타임슬라이싱과 로드 완료 큐](2026-10-07-Spawner-Time-Slicing.md) |
| S3 | 세대별 NPC 제거와 Controller 수명 | [디스폰과 생성 Controller 수명](2026-10-07-Spawner-Despawn-Lifecycle.md) |
| S4 | 거리 생성 방식(원점 진입, 개체별 거리 제거, 재진입) | [거리 활성화](2026-10-07-Spawner-Distance-Activation.md), [Activation 통합](2026-10-07-Spawner-Activation-Mode.md) |
| S5 | 거리 관리 스포너의 그리드 조회 | [그리드 조회](2026-10-07-Spawner-Distance-Grid.md) |
| S6 | 미리보기·디버그·문서 | 현재 상태는 [스포너 사용법](../manual/Spawner.md)을 따른다 |
| S7 | 상태 보존 재생성(선택) | 진행하지 않았다. [휴면 계약 기록](2026-10-07-Spawner-Dormancy-Contract.md) |

## 근거

- 삭제한 계획의 권장 구성·확정 표·작업 표. 원문은 Git 기록에 있다.
- [스포너 사용법](../manual/Spawner.md): `UKataSpawnerSubsystem`만 Tick하고 생성 Subsystem은 준비 큐를 보관한다는 현재 계약.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 결정 이전 | 계획과 단계별 기록·manual 대조(에이전트, 2026-10-16) | 구성 결정을 이 기록에 옮겼다 | 프레임 비용은 측정하지 않았다. 단계별 실행 확인은 각 기록을 따른다 |

## 남은 제한과 후속 작업

- 수동 단일 생성 API는 공용 스포너 예산의 보장 대상이 아니다.
- 상태 보존 재생성은 필요해지면 [#88 세이브 계층](https://github.com/jaykop/Kata/issues/88)에서 다시 본다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| `docs/plan/Spawner-Scheduling-And-Grid-Plan.md`, `docs/plan/Spawner-Dormancy-Contract-Plan.md` | 삭제. 원문은 Git 기록에 있다 |
| 단계별 스포너 기록 | 삭제한 계획 링크를 이 기록 또는 휴면 계약 기록으로 바꿨다 |
| [문서 목록](../README.md) | plan 항목을 지우고 이 기록을 더했다 |
