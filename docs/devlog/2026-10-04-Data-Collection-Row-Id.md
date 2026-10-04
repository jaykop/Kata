# 게임 데이터 컬렉션과 행 ID 참조

작성: 2026-10-04  
갱신: 2026-10-04  
유형: 결정 기록  
대상: KataFramework 데이터 행 참조(`FKataRowBase`, `UKataDataCollection`, `UKataDataSettings`, `FKataCharacterId`), 캐릭터 생성·스포너, `KataFrameworkEditor` 드롭다운  
기준: 커밋 `bd62f55`(DC-1~DC-3), `67a8690`(DC-4와 후속 결정). [#31](https://github.com/jaykop/Kata/issues/31)

## 배경과 결론

캐릭터를 가리킬 때마다 `FDataTableRowHandle`로 테이블과 행을 함께 골랐다. 같은 데이터를 쓰는 곳마다 테이블을 따로 지정해 데이터 출처가 흩어졌고,
행 핸들이 없는 곳(인벤토리, 저장, 테스트 명령)에서는 행을 이름으로 찾을 방법이 없었다. 장비·무기 시스템([#30](https://github.com/jaykop/Kata/issues/30))도 같은 참조가 필요했다.

게임 데이터 테이블을 데이터 컬렉션 에셋 하나로 묶어 Project Settings에 지정하고, 데이터 참조를 영역별 행 ID로 통일했다.
GameMode, 스포너, 비동기 생성 API와 델리게이트를 모두 캐릭터 ID로 옮겼고, 사용자가 빌드와 실행으로 확인했다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 행 기반 구조체 | `FKataCharacterRow : FTableRowBase` | `FKataRowBase`를 두고 `FKataCharacterRow`의 부모를 바꿨다. 필드는 없고, 테이블 편집 때 같은 영역의 행 이름 중복을 경고한다 |
| 테이블 지정 | 참조하는 곳마다 테이블 지정 | `UKataDataCollection`(PrimaryDataAsset)이 PC 테이블 한 칸과 NPC 테이블 목록을 하드 참조로 가진다. `UKataDataSettings`(Game > Kata Data)는 컬렉션 하나만 가리킨다 |
| 참조 타입 | `FDataTableRowHandle` | 공용 기반 `FKataRowId`와 캐릭터 영역 ID `FKataCharacterId`. 디테일 패널은 검색 가능한 행 이름 드롭다운으로 보여 준다 |
| 생성 경로 | 행 핸들, 요청 중 테이블 유지 | `RequestSpawn`, `Spawn Kata Character`, `OnCharacterSpawned`, 스포너 훅이 캐릭터 ID를 쓴다. 행은 요청 시 실제 행 구조로 복사하므로 테이블 유지 코드를 없앴다 |
| PC·NPC 구분 | 행 핸들의 `RowType`으로 선택기 제한 | ID 드롭다운이 같은 `RowType` 메타로 행을 거른다. GameMode는 PC 행이 아니면 경고 후 Default Pawn Class로 시작하고, 스포너는 NPC 행이 아니면 요청을 거절한다 |
| 스포너 테이블 | 행 핸들의 테이블 | `Source Table`로 컬렉션 NPC 목록의 테이블 하나를 골라 Character Id 드롭다운을 좁힌다 |
| 중복 검증 | 없음 | 컬렉션의 Validate Assets, 컬렉션 편집, 테이블 편집 때 경고한다. 실행 중 중복은 오류 로그 후 앞쪽 테이블의 행을 쓴다 |

## 주요 결정과 이유

- **행 ID로 통일(참조 방식 B).** 행 핸들을 유지하고 이름 조회만 더하는 방식(A)도 검토했다. 데이터 출처를 한 곳으로 정하는 것이 컬렉션을 만드는 이유이고, 두 방식이 섞이면 어느 테이블의 행인지 헷갈리는 문제가 남아 B를 택했다.
- **설정 클래스가 아니라 컬렉션 에셋.** 처음에는 설정 클래스가 테이블 칸을 직접 가지도록 구현했으나, 사용자 의도는 "설정에는 컬렉션 하나를 지정하고 컬렉션이 테이블을 가진다"였다. 컬렉션을 바꿔 끼우면 데이터 묶음 전체(테스트용, 실제용)를 바꿀 수 있고, 테이블 구성이 ini가 아니라 Content 에셋으로 버전 관리된다. 테이블은 소프트 참조만 담아 가벼우므로 하드 참조로 두어 수명 관리를 단순하게 했다.
- **영역별 ID.** 테이블마다 ID를 만들면 Blueprint가 구조체 상속을 자동 변환하지 않아 무기 ID를 장비 장착 함수에 넣을 수 없게 된다. 범용 ID 하나는 Blueprint 함수 매개변수에 영역 메타를 붙일 수 없어 잘못된 종류를 막지 못한다. GameplayTag ID는 행과 태그를 이중으로 관리해야 한다.
- **`FKataRowBase`.** 빈 기반은 나중에 추가해도 비용이 같으므로, 등록 제한과 공용 검증이라는 역할이 있을 때 도입했다. 행 구조 경로가 바뀌지 않아 기존 테이블은 그대로 로드된다.
- **NPC 테이블 목록과 스포너 Source Table.** 사용자는 스포너에 테이블을 직접 지정하는 구조를 제안했다. 아무 테이블이나 받으면 스포너가 넘기는 ID를 다른 시스템이 찾지 못하고, 컬렉션 테이블과 이름이 겹쳐도 검증되지 않는다. 그래서 스포너는 컬렉션 NPC 목록의 테이블 중에서만 고르게 했다. Creature처럼 종류별 테이블은 목록에 더하면 되고, 행 구조는 `FKataNPCCharacterRow` 계열이면 된다. PC 테이블은 GameMode 하나가 쓰므로 한 칸을 유지했다.
- **테이블 수.** 처음에는 종류마다 한 칸으로 정했다. 분할이 필요하면 `UCompositeDataTable`을 넣으면 코드 변경이 없다. NPC 영역은 위 결정으로 목록이 됐다.
- **기존 값 수동 재설정.** 영향 받는 에셋이 테스트 GameMode와 테스트 맵 스포너뿐이고 플러그인을 배포하지 않아, 호환 코드를 남기는 자동 변환 대신 다시 고르게 했다.
- **Blueprint 매개변수 규칙의 범위.** AGENTS.md의 "Blueprint 공개 함수 매개변수에 Kata 전용 구조체 금지"는 조건 공용 함수가 실행기 내부 Context를 받지 않게 하려던 규칙이다. 행 ID는 외부에서 데이터를 가리키는 공개 참조 타입이라 해당하지 않으며, 규칙 문구를 공용 판정 함수(`UKataFL_*`)로 좁혔다.

## 진단 기록

- **행 이름 중복 검증 위치.** `FTableRowBase::IsDataValid`는 테이블과 행 이름을 받지 않아 행 단위로는 다른 테이블과 비교할 수 없다. 두 테이블을 모두 아는 컬렉션의 `IsDataValid`에서 검사해 Validate Assets 경고를 낸다. 테이블 편집 콜백(`OnDataTableChanged`)은 테이블 로드 중에도 불리므로 이미 로드된 컬렉션만 검사한다.
- **Source Table 선택기 필터.** 프로퍼티 메타 `GetAssetFilter`와 UFUNCTION을 생성 코드까지 확인했지만, 레벨에 배치한 스포너의 Details 선택기에는 적용되지 않았다(사용자 스크린샷). 원인은 확인하지 못했다. 스포너 Details 커스터마이즈에서 `SObjectPropertyEntryBox`의 `OnShouldFilterAsset`에 필터를 직접 연결해 해결했다.
- **엔진 행 구조 필터의 한계.** `RequiredAssetDataTags`의 `RowStructure`는 행 구조가 정확히 같은 테이블만 고른다. 파생 행 구조의 테이블을 받기 위해 NPC 목록에는 쓰지 않고 행 구조를 코드로 검사한다.

## 근거

- [KataDataCollection.h](../../Plugins/KataFramework/Source/KataFramework/Public/Data/KataDataCollection.h): 테이블 칸, 조회, 중복 검증.
- [KataDataSettings.h](../../Plugins/KataFramework/Source/KataFramework/Public/Data/KataDataSettings.h): 컬렉션 지정과 유지.
- [KataRowId.h](../../Plugins/KataFramework/Source/KataFramework/Public/Data/KataRowId.h): `FKataRowId`, `FKataCharacterId::Find`.
- [KataRowIdCustomization.cpp](../../Plugins/KataFramework/Source/KataFrameworkEditor/Private/Customizations/KataRowIdCustomization.cpp): 드롭다운, `RowType`·`SourceTableProperty` 필터.
- [KataCharacterSpawnerDetails.cpp](../../Plugins/KataFramework/Source/KataFrameworkEditor/Private/Customizations/KataCharacterSpawnerDetails.cpp): Source Table 선택기 필터.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| DC-1~DC-3 | 2026-10-03~04 사용자 Editor 빌드 보고 | 빌드 성공, Character Id 드롭다운 표시 | 단계별 실행 결과는 따로 보고되지 않음 |
| DC-4와 후속 결정 | 2026-10-04 사용자 Editor 빌드·PIE 보고 | PC·NPC 생성, Source Table 필터 확인 | Game 대상 빌드 |

## 남은 제한과 후속 작업

- Blueprint 노드 입력 핀에는 행 ID 드롭다운이 없다. 그래프 핀 팩토리가 필요하다.
- 컬렉션의 테이블 종류(칸)는 플러그인 코드에 고정되어 있다. NPC 영역만 목록이다.
- 장비 영역 ID(`FKataEquipmentId`)와 Equipment·Weapon 테이블 칸은 [#30](https://github.com/jaykop/Kata/issues/30) EQ-1에서 추가한다.
- `GatherAssetsToLoad`를 `FKataRowBase`로 올려 장비 행 비동기 로드와 공유할지는 #30에서 정한다.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [#31](https://github.com/jaykop/Kata/issues/31) | 결과 댓글과 닫기(사용자 확인 후) |
| [Character Data](../manual/Character-Data.md) | 컬렉션 설정, 캐릭터 ID 사용법, 재설정 안내 |
| [Spawner](../manual/Spawner.md) | Source Table, Character Id |
| [캐릭터 데이터 테이블과 비동기 생성 계획](../plan/Character-Definition-Plan.md) | "캐릭터 지정 방식" 결정의 대체 표시 |
| 게임 데이터 컬렉션과 행 ID 참조 계획 | 이슈 종료에 따라 삭제. 결정은 이 문서로 옮겼다 |
