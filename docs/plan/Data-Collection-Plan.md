# 게임 데이터 컬렉션과 행 ID 참조 계획

작성: 2026-10-03  
갱신: 2026-10-03  
연결 이슈: [#31 게임 데이터 컬렉션과 행 ID 참조 통일 (KataFramework)](https://github.com/jaykop/Kata/issues/31). 로드맵 [#24](https://github.com/jaykop/Kata/issues/24)  
현재 상태 근거: 구현 없음. 현재 캐릭터 참조 방식은 [Character Data](../manual/Character-Data.md), [Spawner](../manual/Spawner.md)  
대체 관계: [캐릭터 데이터 테이블과 비동기 생성 계획](Character-Definition-Plan.md)의 "캐릭터 지정 방식"(`FDataTableRowHandle` 하나로 지정) 결정을 이 계획의 행 ID 참조로 대체한다

## 목적과 현재 상태

지금은 캐릭터를 가리킬 때마다 `FDataTableRowHandle`로 테이블과 행을 모두 고른다.
같은 데이터를 쓰는 곳마다 테이블을 따로 지정하므로 데이터 출처가 흩어지고, 행 핸들이 없는 곳(인벤토리, 저장, 테스트 명령)에서는 행을 이름으로 찾을 방법이 없다.
장비·무기 시스템([#30](https://github.com/jaykop/Kata/issues/30))도 같은 방식으로 데이터를 참조해야 한다.

목표는 게임 데이터 테이블 목록을 Project Settings 한 곳에서 정의하고, 데이터 참조를 행 ID 하나로 통일하는 것이다.

현재 `FDataTableRowHandle`을 쓰는 곳은 다음과 같다.

| 위치 | 용도 |
|---|---|
| `AKataGameMode::PlayerCharacterRow` | PC 캐릭터 행 |
| `AKataCharacterSpawner::CharacterRow`, `ActiveRow` | 스폰할 캐릭터 행 |
| `UKataAsyncAction_SpawnCharacter::SpawnKataCharacter` | Blueprint 비동기 생성 노드 입력 |
| `UKataCharacterSpawnSubsystem::RequestSpawn`, `FKataOnCharacterSpawnedSignature`, 내부 요청 | 생성 요청과 완료 알림 |
| `UKataSpawnerComponent::OnCharacterSpawned` 등 | 스포너 콜백 |

## 범위

- 포함: `FKataRowBase`, 데이터 설정 클래스, 행 조회, 캐릭터 영역 ID `FKataCharacterId`와 에디터 행 선택, 위 표의 참조 전환, 테스트 에셋 재설정 안내.
- 장비 영역: `FKataEquipmentId`와 Equipment·Weapon 테이블 칸은 이 계획의 공용 구조를 써서 [#30](https://github.com/jaykop/Kata/issues/30) EQ-1에서 추가한다. 행 구조체가 그때 생기기 때문이다.
- 제외: 기능별 튜닝 값(`UKataFactionSettings`, `UKataHitTraceSettings`)의 이전, 엔진 Data Registry 도입, 기존 값의 자동 변환.

## 구성

### `FKataRowBase`

- Kata 데이터 행 구조체의 공용 기반이다. `FKataCharacterRow`의 부모를 `FTableRowBase`에서 `FKataRowBase`로 바꾼다. 구조체 이름과 필드는 그대로이므로 기존 DataTable 에셋은 그대로 로드된다.
- 역할은 두 가지다. 데이터 설정에 등록할 수 있는 테이블을 이 기반을 상속한 행 구조체로 제한하고, 모든 Kata 행에 공통 데이터 검증을 붙이는 지점이 된다.
- 공통 필드는 두지 않는다. 필요해질 때 추가한다.

### 데이터 설정

- `UDeveloperSettings` 파생 클래스를 `KataFramework`에 둔다(Project Settings의 Game 항목, `Config=Game`).
- 데이터 종류마다 테이블 한 칸을 `UDataTable` 소프트 참조로 둔다. 이번 범위는 PC 캐릭터, NPC 캐릭터다.
- 테이블을 나눠 관리해야 하면 그 칸에 엔진 `UCompositeDataTable`을 넣는다. `UDataTable`의 하위 클래스이므로 코드는 바뀌지 않는다.
- 이 클래스는 게임 데이터 테이블 목록만 담는다. 기능별 튜닝 값은 기존 설정 클래스에 둔다.

### 행 ID와 조회

- ID는 데이터 영역 단위로 만든다. `FKataCharacterId`는 PC·NPC 테이블을 함께 찾고, `FKataEquipmentId`(#30)는 Equipment·Weapon 테이블을 함께 찾는다. 테이블마다 ID를 나누면 Blueprint가 구조체 상속을 자동 변환하지 않아 무기 ID를 장비 장착 함수에 넣을 수 없게 된다.
- ID 구조체는 행 이름(`FName`)을 담는다. 에디터에서는 그 영역 테이블들의 행 이름 드롭다운으로 표시한다. 드롭다운 표시는 영역 ID들이 공유하는 코드 하나로 처리한다.
- 조회는 영역 테이블에서 행을 찾아 행 구조체로 돌려준다. PC 행이 필요한 곳(`AKataGameMode`)은 찾은 행이 PC 행인지 검사한다. 지금 비동기 생성이 하는 행 구조 검사와 같다.
- 같은 영역 안에서 행 이름은 겹치지 않아야 한다. 에디터 데이터 검증에서 중복을 경고하고, 실행 중 조회가 중복을 만나면 오류 로그를 남긴다.

### 참조 전환

- 위 표의 `FDataTableRowHandle`을 `FKataCharacterId`로 바꾼다. 프로퍼티 이름은 이번에 정리한다(예: `PlayerCharacterRow` → `PlayerCharacter`).
- 생성 서브시스템이 테이블 수명을 붙잡던 `DataTableKeepAlive`는 데이터 설정이 테이블을 로드해 유지하므로 정리 대상이다.
- 기존 값은 자동 변환하지 않는다. 사용자가 아래 에셋에서 행을 다시 고른다.
  - `Content/KataTest/BP_KataTestGameMode`의 PC 캐릭터
  - `Content/KataTest/Maps/LV_TestMap`에 배치한 `AKataCharacterSpawner`의 캐릭터
- 2026-10-03 기준 비동기 생성 노드와 생성 델리게이트를 쓰는 Blueprint는 없다. 핀 타입이 바뀌어도 깨지는 그래프는 없다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 참조 방식 통일 | 확정 | 2026-10-03 사용자 결정. 데이터 참조를 행 ID 하나로 통일하고 테이블은 데이터 설정에서 찾는다 |
| `FKataRowBase` | 확정 | 2026-10-03 사용자 제안. 등록 제한과 공용 검증 역할로 도입하고 필드는 두지 않는다 |
| ID 형태 | 확정 | 2026-10-03 사용자 결정. 데이터 영역별 ID 타입(`FKataCharacterId`, `FKataEquipmentId`) |
| 종류별 테이블 수 | 확정 | 2026-10-03 사용자 결정. 종류마다 한 칸. 분할이 필요하면 Composite를 넣는다 |
| 행 이름 중복 | 확정 | 2026-10-03 사용자 결정. 같은 영역 안 중복 금지, 에디터 데이터 검증으로 경고 |
| 기존 값 이전 | 확정 | 2026-10-03 사용자 결정. 자동 변환 없이 테스트 에셋 두 곳을 수동 재설정한다 |
| Blueprint 매개변수 규칙 | 확정 | 2026-10-03 사용자 결정. AGENTS.md 규칙의 범위를 공용 판정 함수(`UKataFL_*`)로 명확히 했다. 공개 참조 타입인 행 ID는 Blueprint 함수 매개변수에 쓸 수 있다 |
| 조회 위치 | 제안 | 에디터 드롭다운·프리뷰와 게임이 함께 쓰도록 데이터 설정의 정적 조회 함수로 둔다. 서브시스템이 필요한 상태(로드한 테이블 유지)가 생기면 Engine Subsystem을 검토한다 |
| 테이블 로드 시점 | 제안 | 첫 조회 때 동기 로드하고 이후 유지한다. 테이블에는 소프트 참조만 있어 가볍다. 행 안의 에셋은 지금처럼 생성·장착 때 비동기로 로드한다 |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| DC-1 | 높음 | `FKataRowBase` 도입과 `FKataCharacterRow` 부모 변경, 영역 내 행 이름 중복 검증 | 없음 | 기존 캐릭터 테이블이 그대로 열리고, 중복 행 이름에 검증 경고가 나온다 |
| DC-2 | 높음 | 데이터 설정 클래스와 행 조회 | DC-1 | Project Settings에서 PC·NPC 테이블을 지정하고 행 이름으로 행을 찾을 수 있다 |
| DC-3 | 높음 | `FKataCharacterId`와 에디터 행 선택 드롭다운 | DC-2 | 프로퍼티에서 PC·NPC 테이블의 행 이름을 드롭다운으로 고를 수 있다 |
| DC-4 | 높음 | GameMode·스포너·비동기 생성 API·서브시스템·델리게이트 전환 | DC-3 | 행 ID로 PC 생성과 스포너 생성이 동작한다 |

## 영향과 제한

- 공개 API: `SpawnKataCharacter`, `RequestSpawn`, 생성 델리게이트, 스포너 콜백의 매개변수 타입이 바뀐다. C++·Blueprint 호출부를 함께 고친다.
- 직렬화: `FKataCharacterRow` 부모 변경은 기존 테이블에 영향이 없다. 참조 프로퍼티는 타입과 이름이 바뀌므로 기존 값이 버려지고, 위 두 에셋을 다시 설정해야 한다.
- 설정 파일: 데이터 테이블 지정이 `Config/DefaultGame.ini`에 저장된다.
- 모듈 경계: 모두 `KataFramework` 안에서 처리한다. 코어 `Kata`는 바뀌지 않는다. 행 선택 드롭다운은 `KataFrameworkEditor`에 둔다.

## 사용자 확인 항목

- 구현 완료: DC 항목별 완료 조건을 만족하는 코드와 문서.
- 실행 확인: 사용자가 Editor 빌드 후 Project Settings에 테이블을 지정하고, 테스트 GameMode와 `LV_TestMap` 스포너의 캐릭터를 다시 고른 뒤 PC 생성과 스포너 생성을 확인한다.

## 완료 시 갱신할 문서

- [#31](https://github.com/jaykop/Kata/issues/31): 구현·확인 상태.
- [Character Data](../manual/Character-Data.md), [Spawner](../manual/Spawner.md): 행 ID 참조, 데이터 설정, 재설정 안내.
- [캐릭터 데이터 테이블과 비동기 생성 계획](Character-Definition-Plan.md): "캐릭터 지정 방식" 결정의 대체 표시.
- devlog: 참조 방식 통일과 수동 재설정 결정.
- [문서 목록](../README.md): 이 계획 링크.
