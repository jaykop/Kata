# 캐릭터 팩션을 행과 스포너에서 지정

작성: 2026-10-08  
갱신: 2026-10-08  
유형: 결정 기록  
대상: KataFramework 캐릭터 행·스포너, KataTargeting 팩션 값  
기준: #13 하위 작업. 이 기록과 같은 커밋의 소스 변경

## 배경과 결론

팩션은 캐릭터 Blueprint의 `UKataTargetingComponent.Faction`에서만 정할 수 있었다. 같은 Blueprint를 쓰는 캐릭터를 다른 편으로 두려면 Blueprint를 나눠야 했다.
사용자가 캐릭터 테이블이나 스포너에서 정하는 방식을 제안했고, 둘 다 넣기로 결정했다.
팩션은 스포너의 Faction Override → 행의 Faction → Character Class의 컴포넌트 기본값 순서로 정한다. 앞쪽 값이 비어 있으면 다음 값을 쓴다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| `FKataCharacterRow::Faction` | 없음. 행 주석이 팩션을 Character Class 기본값으로 규정 | 공통 행에 추가. `AKataCharacter::ApplyCharacterRow`가 유효한 값을 타게팅 컴포넌트에 기록 |
| `AKataCharacterSpawner::FactionOverride` | 없음 | `PrepareSpawnBatch`가 배치의 행 사본에 기록. 원본 테이블은 바꾸지 않음 |
| `UKataTargetingComponent.Faction` | 팩션을 정하는 유일한 곳 | 런타임 값 저장소로 유지. 행 없이 배치한 액터와 프리뷰의 기본값 |

## 주요 결정과 이유

- 컴포넌트의 Faction을 없애지 않았다. `AKataCharacter`와 `AKataAIController`의 팀 번호가 이 값을 읽는다. 또 KataTargeting은 KataFramework의 행을 참조할 수 없다.
- 스포너의 덮어쓰기는 `UKataSpawnerComponent` 파생으로 만들지 않았다. 그 완료 훅은 BeginPlay 이후에 불려 AI Controller·Perception 초기화보다 늦다.
  배치의 행 사본을 고치면 기존 `OnConstruction` → `ApplyCharacterRow` 경로로 BeginPlay 전에 반영된다.
- 엔진 시야 감지는 감지 쌍을 등록할 때 관계를 판정해 고정한다(`UAISense_Sight`의 대상 등록). 그래서 팩션은 BeginPlay 전에 확정돼야 한다.

## 진단: AI가 PC만 노리는 것처럼 보임

같은 작업에서 사용자가 "적 AI가 다른 팩션을 노리지 않는다"고 보고했다. AI 타게팅 코드에는 PC를 따로 고르는 조건이 없었다.
원인은 관계표였다. 샘플 관계표는 Player↔Enemy만 적대이고, 관계표에 없는 서로 다른 팩션은 중립, 같은 팩션은 우호다.
조치는 [팩션 사용법](../manual/Factions.md)의 문제 해결 표에 적었다.

## 근거

- [KataCharacterRow.h](../../Plugins/KataFramework/Source/KataFramework/Public/Character/KataCharacterRow.h): `Faction`.
- [KataCharacter.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacter.cpp): `ApplyCharacterRow`.
- [KataCharacterSpawner.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataCharacterSpawner.cpp): `PrepareSpawnBatch`.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 행·스포너 팩션 지정 | 사용자 빌드·실행 보고 | 2026-10-08 팩션 테스트 완료 보고 | 개별 시나리오 결과는 보고되지 않음 |
