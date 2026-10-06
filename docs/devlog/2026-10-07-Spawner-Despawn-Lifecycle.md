# 스포너 디스폰과 생성 Controller 수명

작성: 2026-10-07  
갱신: 2026-10-07  
유형: 구현 기록  
대상: KataFramework의 스포너·월드 관리자·행 기반 캐릭터 생성  
기준: 1단계 커밋 `da3eada` 이후 미커밋 2단계 타임슬라이싱과 이번 3단계 변경

## 배경과 결론

사용자가 디스폰 시점과 거리 설정의 역할을 확인한 뒤 3단계 기반 구현을 요청했다. `DespawnCharacters`는 생성 대기를 취소하고 해당 스포너의 이전 배치까지 포함한 생성 NPC와 소유 AIController를 공용 예산으로 정리한다. 제거 중에는 새 생성과 중복 제거를 거절하며 완료 후 다시 생성할 수 있다.

플레이어 거리로 자동 실행하는 조건과 DistanceActivation 설정은 후속 4단계다. 이번 단계는 수동 호출로 NPC의 제거·재생성을 확인할 수 있는 기반을 제공한다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 제거 명령 | CancelSpawning으로 대기만 취소 | 별도 DespawnCharacters·IsDespawning·GetPendingDespawnCount·OnDespawnFinished |
| NPC 기록 | 생성 NPC의 약한 참조 목록 | 생성 세대와 NPC·직접 만든 Controller의 공유 수명 기록 추가 |
| Controller 구분 | 현재 빙의 대상만 조회 가능 | SpawnDefaultController의 실제 생성 결과 기록, 다른 Pawn 이전 시 소유 해제 |
| 제거 실행 | 없음 | NPC와 Controller를 별도 단계로 순환 처리 |
| 생성·제거 예산 | 생성만 분산 | 같은 시간 예산과 별도 단계 상한, 프레임별 첫 처리 종류 교대 |
| 종료 중 제거 | 없음 | 진행 중 작업을 관리자가 이어받음. 선택적 Despawn On End Play 추가 |
| 실패 | 제거 결과 없음 | Destroy 거절을 결과·로그로 알리고 생존 스포너에 실패 기록 반환 |

## 주요 결정과 이유

현재 Controller와 생성한 Controller는 같은 개념이 아니다. `AKataCharacter::SpawnDefaultController`가 만든 실제 객체를 Possess 전에 기록한다. 엔진 함수는 생성 결과를 반환하지 않으므로, 소유 추적이 연결된 경우 생성 매개변수와 Possess 순서를 구현에서 유지하고 실제 반환 객체를 기록한다. 소유 추적이 없는 캐릭터는 기존 Super 경로를 사용한다.

소유 Controller의 Pawn 변경을 관찰해 다른 Pawn으로 이전하면 기록에서 제외한다. 제거 직전에도 다른 Pawn을 빙의하는지 확인하고 PlayerController는 제거하지 않는다. 디스폰 중 PawnPendingDestroy가 외부 Controller를 제거하지 않도록 빙의만 해제하고, 기록한 AIController는 관리자에서 별도로 정리한다. 기존 KataAI의 EndPlay·UnPossess에 따른 StateTree 종료 경로는 유지한다.

기록은 약한 UObject 참조만 보관하고 생성 서비스·캐릭터·스포너·제거 작업이 공유한다. 생성 서비스가 FinishSpawning 전에 연결하므로 BeginPlay 안에서 디스폰을 요청해도 결과 전달 전 NPC를 놓치지 않는다. 세대가 디스폰을 요청했으면 생성 서비스는 결과 이벤트를 생략하고 예약한 제거 작업에 맡긴다. 일반 취소의 결과 전달 전 롤백에는 같은 소유 기반 정리 함수를 사용한다.

제거 대상을 관리자에 넘긴 뒤 생성 취소 이벤트를 알린다. 이벤트 안에서 새 배치가 제거 대상에 섞이지 않게 제거 상태를 먼저 설정한다. 완료 이벤트 직전에는 상태를 해제해 완료 이벤트에서 새 생성 또는 실패 재시도가 가능하다. 공유 작업의 동일성을 확인해 오래된 완료가 새 작업을 해제하지 않는다.

제거 큐는 스포너의 수명과 분리된다. 진행 중에 스포너가 종료돼도 관리자가 남은 작업을 처리한다. 일반 스포너의 기존 종료 계약을 유지하려고 Despawn On End Play는 기본 false다. 거리 관리 모드의 종료 정책은 4단계에서 연결한다. 월드 전체 종료에서는 결과를 알리지 않고 엔진 월드 정리에 맡긴다.

Destroy 한 번은 중간에 끊지 않는다. NPC와 Controller 각각을 한 단계로 세며 무효한 기록 확인도 단계 상한에 포함한다. 현재 기본 제거 상한은 프레임당 2단계로, 측정에 따른 보장값이 아니다. 생성과 제거가 동시에 쌓여도 한 종류가 계속 시간 예산을 먼저 쓰지 않도록 첫 처리 차례를 매 프레임 교대한다.

## 근거

- [디스폰 API](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataCharacterSpawner.h): 호출·상태·결과 계약과 종료 옵션.
- [수명 기록](../../Plugins/KataFramework/Source/KataFramework/Public/Character/KataCharacterSpawnOwnership.h): 세대·NPC·Controller의 약한 참조와 디스폰 요청.
- [캐릭터](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacter.cpp): Controller 생성·이전 관찰과 종료 구독 정리.
- [제거 함수](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataFL_Spawning.cpp): NPC 제거와 소유 Controller 구분.
- [월드 관리자](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataSpawnerSubsystem.cpp): 제거 커서·단계 상한·순환과 종료 처리.
- [생성 서비스](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacterSpawnSubsystem.cpp): 결과 전달 전 소유 기록 연결과 디스폰 인계.

## 확인 범위와 결과

| 대상 | 수행자·방법 | 결과 | 미확인 범위 |
|---|---|---|---|
| 이번 3단계 변경 | 에이전트 구현 | 소스·문서 반영 | 빌드·PIE·UI·프로파일·별도 검사 |
| 기존 1단계 | 사용자 빌드 보고 | 성공, 타깃 미지정 | 이번 2·3단계의 검증 근거로 확대하지 않음 |

빌드·테스트·lint·정적 분석·별도 리뷰와 테스트 코드 추가는 수행하지 않았다. 사용법의 수동 확인 절차는 실행 결과가 아니다.

이후 사용자가 `KataCharacter.cpp`의 지역 변수 `Controller`가 `APawn::Controller`를 가리는 C4458 빌드 오류를 보고했다. 두 지역 변수를 `OwnedControllerPtr`로 변경했으며 수정 후 재빌드 성공은 아직 보고되지 않았다.

## 남은 제한과 후속 작업

4단계에서 DistanceActivation의 생성·제거 거리와 그리드 조회를 연결한다. 전투 중 이탈·재진입 정책은 아직 미확정이다. 현재 공개 제거는 스포너 전체 생성 기록을 대상으로 하며 세대 ID는 후속 관리 범위를 구분할 수 있도록 보존한다.

후속 설계: [개체별 휴면 계약 결정](2026-10-07-Spawner-Dormancy-Contract.md)에서 사용자의 NPC 현재 거리·스냅샷 복원 요구와 계약 선행 순서를 반영했다. 위 전투·재진입 미확정 문장은 3단계 당시 판단이며, 수동 제거 API와 후속 거리 휴면 경로는 구분한다.

소유 추적은 행 기반 생성 서비스가 연결한다. 임의 Blueprint 코드가 직접 만든 Controller나 외부 NPC를 자동으로 이 스포너 소유로 취급하지 않는다. 제거 실패 기록은 생존 스포너가 재시도할 수 있지만 소유자 종료 후 실패에는 경고만 남기고 무한 재시도하지 않는다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 사유 |
|---|---|
| [스포너 사용법](../manual/Spawner.md) | 수동 제거·결과·종료 옵션·실패·눈으로 확인할 절차 |
| [캐릭터 데이터](../manual/Character-Data.md) | 생성 전 소유 기록과 결과 전달 전 디스폰 인계 |
| [AI 사용법](../manual/AI.md) | 기존 EndPlay·StateTree 종료 계약 유지, 별도 변경 없음 |
| [계획](../plan/Spawner-Scheduling-And-Grid-Plan.md) | 수명 기록·수동 제거·종료 호환성 경계 |
| [문서 목록](../README.md) | 이 기록 링크 |
| 연결 이슈 | 열린 이슈 검색에 대응 항목 없음. 기존 신규 초안은 공개 게시 승인 전이며 미게시 |
