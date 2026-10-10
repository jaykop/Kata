# 스포너의 AI 설정 덮어쓰기

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 구현 기록, 결정 기록  
대상: KataFramework 스포너 설정 컴포넌트, KataAI AI Data, NPC 행  
기준: main d5a40fb 이후 미커밋 작업 트리. 같은 작업 트리에 이 작업과 무관한 KataFramework·KataRuntime 미커밋 변경이 함께 있다.

## 배경과 결론

같은 NPC 행을 배치마다 다른 AI로 쓰고 싶다는 요청이 있었다. 예를 들어 같은 몬스터를 어떤 곳에서는 경비로, 다른 곳에서는 순찰로 둔다. 기존 스포너 설정 컴포넌트의 완료 훅(`OnCharacterSpawned`)은 캐릭터 BeginPlay 이후에 호출되므로 초기 AI 설정을 바꿀 수 없었다.
스포너 설정 컴포넌트 `AI Override`를 추가했다. AI Data 교체, Linked StateTree 슬롯 병합, LeashDistance 덮어쓰기와 AI 끄기(`bDisableAI`)를 배치 행 사본에 기록해 BeginPlay 전에 적용한다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 |
|---|---|---|
| AI 설정 소비 경로 | Controller의 StateTree·슬롯·Sense, AI 타게팅의 Preset·Home, Evaluator의 Leash·재시도, 디버거가 모두 `IKataAIPawnInterface::GetKataAIData()`로 읽는다 | Pawn이 돌려주는 AI Data만 바꾸고 소비 코드는 수정하지 않았다 |
| 적용 시점 | `TryStartKataAI`가 BeginPlay에서 한 번 Perception과 Linked 오버라이드를 고정한다 | Faction Override와 같은 행 사본 경로로 BeginPlay 전에 적용한다 |
| 스포너 확장 지점 | 행 사본을 바꾸는 훅이 없었다 | `UKataSpawnerComponent::ModifySpawnRow`(C++ virtual)를 추가했다. Faction 기록과 중복 설정 검사 뒤 배열 순서로 호출한다 |
| 덮어쓰기 값 | 없음 | KataAI에 `FKataAIDataOverride`(슬롯, LeashDistance), 합성 함수 `KataFL::ComposeAIData`를 추가했다 |
| 전달 | 없음 | `FKataNPCCharacterRow::AIDataOverride`(편집 불가 Transient)에 싣고 `AKataAICharacter::ApplyCharacterRow`에서 합성한다 |
| AI 끄기 | AI Data를 비우면 "행 값 유지"로 해석돼 "AI 없음"을 지정할 수 없었다 | `bDisableAI`가 행 사본의 AI Data와 덮어쓰기를 모두 비운다 |

## 주요 결정과 이유

- **덮어쓰기 대상을 AI Data 교체·Linked 슬롯·LeashDistance로 정했다.** 사용자가 확정했다. Linked 슬롯과 추격 거리는 배치 장소에 따라 달라지는 값이다. 반면 Senses·Targeting Preset·재시도 값은 몬스터 타입 단위라 행의 AI Data에 둔다. AI Controller Class는 바꿀 일이 없다는 사용자 판단으로, Home 기준 위치는 AI Data 밖의 위치 정보라서 제외했다.
- **AI Data 통째 교체만 제공하는 안은 채택하지 않았다.** 처음에는 가장 단순한 안으로 검토했다. 그러나 배치용 값 두 개를 바꾸려고 타입 단위 필드를 모두 복사한 변형 에셋이 생긴다. AI Data에는 상속이 없어 기준 에셋을 고치면 변형 에셋마다 같은 수정이 필요하다는 점을 사용자와 확인했다. 교체는 기준을 고르는 선택 항목으로 남겼다.
- **전달 경로는 행 사본의 편집 불가 Transient 필드(a1)로 정했다.** 사용자가 확정했다. Faction Override와 같은 경로라서 비동기 로드, 시간 분산 생성, 거리 재생성에 별도 처리가 필요 없다. 행 필드를 테이블에서 편집할 수 있게 여는 안은 요청 범위를 넓히므로 제외했다. 스폰 서브시스템에 FinishSpawning 직전 콜백을 두는 안은 서브시스템 API와 행 적용 순서를 함께 바꿔야 해서 제외했다.
- **합성 사본은 캐릭터마다 만든다.** Outer를 Pawn으로 둔 Transient 사본이며, 기존 `AIData` UPROPERTY가 수명을 유지한다. 덮어쓸 값이 없으면 복제하지 않는다. 배치 단위 공유는 비용을 측정한 뒤 판단한다.
- **기준 AI Data 없이 슬롯·Leash만 덮어쓰면 경고 후 AI를 시작하지 않는다.** 마스터 StateTree가 없으면 두 값은 의미가 없고, 대개 설정 실수이기 때문이다. 의도적으로 AI 없이 생성하는 경우는 `bDisableAI`로 구분했다.
- **한 스포너에 AI Override는 하나만 허용한다.** 덮어쓰기 결과를 예측하기 쉽게 하려는 것이며, 중복 Spawn Area를 거절하는 방식과 같다.
- **AI Data 상속(부모 에셋 + 필드 오버라이드)은 후속 검토로 남겼다.** 스포너 밖에서도 변형 에셋의 중복이 문제가 될 때 별도 이슈로 다룬다.

## 근거

- [AI Override](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataSpawnerComponent_AIOverride.h): 설정 항목과 `ModifySpawnRow` 구현.
- [설정 기반](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataSpawnerComponent.h): `ModifySpawnRow`의 호출 시점과 규칙.
- [스포너 액터 구현](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataCharacterSpawner.cpp): `PrepareSpawnBatch`의 Faction 기록, 스냅샷·중복 검사, 행 사본 수정 순서.
- [AI Data](../../Plugins/KataAI/Source/KataAI/Public/Data/KataAIData.h): `FKataAIDataOverride`.
- [AI Data 합성](../../Plugins/KataAI/Source/KataAI/Public/Data/KataFL_AIData.h): `KataFL::ComposeAIData`.
- [AI 캐릭터 구현](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataAICharacter.cpp): 행 적용 시 합성.
- [팩션 지정 기록](2026-10-08-Character-Faction-Source.md): 행 사본에 기록하는 선례.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 소스 전체 | 사용자 빌드 보고(2026-10-10) | 빌드 통과 | 빌드 타깃 미상 |
| 슬롯·Leash 덮어쓰기, AI 끄기, 중복 거절 | 사용자 PIE 확인 보고(2026-10-10) | 확인 완료 보고 | 개별 시나리오별 결과는 보고되지 않음 |

에이전트는 빌드·테스트·별도 검사를 실행하지 않았다.

## 남은 제한과 후속 작업

- Blueprint 파생 스포너 설정은 `ModifySpawnRow`를 재정의할 수 없다.
- AI Controller Class, Home 기준 위치, Senses·Targeting Preset은 덮어쓰지 않는다.
- DataTable CSV·JSON 내보내기가 편집 불가 Transient 필드를 포함하는지는 확인하지 않았다.
- AI를 끈 NPC에서도 Controller가 `AI start skipped ... AIData=None` 로그를 Log 수준으로 남긴다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#48](https://github.com/jaykop/Kata/issues/48) | 구현 완료, 사용자 빌드 통과와 PIE 확인 완료. 결과 댓글 게시 |
| [스포너 사용법](../manual/Spawner.md) | AI 설정 덮어쓰기 항목 추가, `ModifySpawnRow` 확장 설명, 제한 항목 갱신 |
| [KataAI 사용법](../manual/AI.md) | 캐릭터 AIData 참조의 사본 동작과 스포너 덮어쓰기 연결 |
