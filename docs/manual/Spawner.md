# 스포너 사용법

갱신: 2026-10-04  
대상: KataFramework의 NPC 스포너와 인라인 설정 객체  
적용 기준: #21의 GEComponent 방식 최소 스포너  
확인 상태: 2026-10-04 사용자가 행 ID 전환, Source Table 필터, Spawn Area(구·상자), 최소·최대 수량, Nav Mesh Projection을 Editor 빌드 후 실행으로 확인했다. 2026-10-03 사용자 빌드와 스폰 동작 확인. 수량·영역·실패·취소와 선택기 필터의 개별 결과는 보고되지 않았다.

## 목적과 준비

스포너에 NPC 캐릭터 ID를 지정하고, Details의 `Spawner Components` 배열에서 수량·영역 설정을 추가한다.
배열 항목은 스포너가 소유하는 Instanced UObject다. 캐릭터 ID는 Project Settings에 지정한 데이터 컬렉션의 NPC 테이블에서 찾는다. NPC 행의 Character Class는 필수이며 외형 설정은 [캐릭터 데이터 사용법](Character-Data.md)을 따른다.

이번 변경은 기존 프로토타입의 클래스 기반을 ActorComponent에서 UObject로 바꾼다. 에디터를 종료하고 Editor를 전체 재빌드한 뒤 다시 연다.
이전 방식으로 추가한 `Kata Spawner` ActorComponent와 옵션 ActorComponent가 있다면 제거하고 아래 배열에 설정을 다시 작성한다. 기존 설정을 자동 변환하지 않는다.

## 사용 순서

1. 레벨에 배치한 `KataCharacterSpawner`를 선택한다. Blueprint 파생 클래스의 Class Defaults에서도 기본 설정을 편집할 수 있다.
2. Details의 `Kata | Spawning`에서 필요하면 `Source Table`에 NPC 테이블을 지정하고, `Character Id` 드롭다운으로 행을 고른다. Source Table이 비어 있으면 데이터 컬렉션의 모든 NPC 테이블 행이, 지정하면 그 테이블의 행만 나온다. Source Table은 컬렉션의 NPC Character Tables 목록에 있는 테이블이어야 한다.
3. 같은 카테고리의 `Spawner Components` 배열에서 `+`로 항목을 추가한다. 항목의 클래스 선택에서 `Spawn Area`를 선택하고 펼친다.
4. `Enabled`를 켜고 `Min Spawn Count`, `Max Spawn Count`, `Spawn Area Transform`, `Area Shape`, `Collision Handling`을 설정한다. 영역 Transform은 스포너 액터 기준 상대값이다.
   `Area Shape`의 기본값은 Sphere(`Sphere Radius` 기본 200)이고, Box를 고르면 `Box Extent`(기본 X=200, Y=200, Z=0)가 나온다.
   뷰포트에는 활성화된 첫 `Spawn Area`의 영역이 주황색 구(`SphereAreaPreview`) 또는 상자(`SpawnAreaPreview`)로 표시된다. 에디터 전용이며 충돌이 없고 게임에서는 보이지 않는다.
   영역 중심에는 생성될 캐릭터의 메시(`CharacterPreview`)가 실제 생성과 같은 회전으로 표시된다. 행의 메시가 비어 있으면 캐릭터 Blueprint의 기본 메시를 쓰고,
   캐릭터 Blueprint의 Mesh 상대 위치·회전을 반영한다. 애니메이션 없이 기본 포즈로 보인다. 실제 생성 위치는 영역 안에서 무작위로 정해진다.
   DataTable 행을 수정한 뒤에는 스포너를 조금 움직이거나 속성을 다시 편집해야 미리보기가 갱신된다.
5. `Spawn On Begin Play`가 켜져 있으면(기본값) 게임 시작 시 스포너의 BeginPlay에서 `Spawn Characters`를 한 번 호출한다.
   원하는 시점에 생성하려면 이 옵션을 끄고, 스포너 Blueprint나 레벨 Blueprint 등에서 `Spawn Characters`를 직접 호출한다.
6. `On Character Spawned`, `On Character Spawn Failed`, `On Batch Finished`를 바인딩해 결과를 받는다. `Spawn Characters=false`는 요청 준비 단계의 거절이며 로그를 확인한다.

Sphere는 구 내부의 부피에서 균일하게 고르므로 영역 원점보다 아래쪽 위치도 후보가 된다. 바닥 아래가 후보에 들어가지 않게 영역 높이와 반지름을 정한다.
Box는 Z Extent가 0이면 영역 원점을 지나는 평면에서 위치를 고르고, Z를 늘리면 부피 안에서 고른다.
영역은 캐릭터 중심이 놓일 높이에 설정한다. 바닥 높이로 설정하면 캡슐이 겹칠 수 있으며 실제 위치는 Collision Handling의 영향을 받는다.
활성 `Spawn Area`가 없으면 스포너 액터 Transform에서 1개를 생성한다.

## 주요 설정과 실행 계약

| 항목 또는 API | 의미 | 기본값·실패 시 동작 |
|---|---|---|
| Source Table | 캐릭터를 고를 NPC 테이블 | 선택기에는 컬렉션의 NPC 테이블 목록에 있는 테이블만 나온다. 비우면 모든 NPC 테이블. Blueprint로 목록에 없는 테이블을 넣으면 드롭다운이 비고 요청을 거절한다 |
| Character Id | 생성할 NPC 캐릭터 ID | Details 드롭다운에는 Source Table(비었으면 모든 NPC 테이블)의 행만 나온다. Blueprint에서 비우거나 그 범위에 없는 ID를 넣으면 요청을 거절한다. 행 핸들 방식에서 옮긴 스포너는 값이 비어 있으므로 다시 고른다 |
| Spawn On Begin Play | 게임 시작 시 한 번 자동으로 생성할지 여부 | 기본 true. 끄면 `Spawn Characters`를 직접 호출해야 한다 |
| Spawner Components | Details에서 소유·편집하는 설정 객체 배열 | 빈 항목과 비활성 항목은 건너뛴다 |
| Enabled | 다음 생성 작업에 해당 설정을 포함할지 여부 | 기본 true |
| Min Spawn Count / Max Spawn Count | 한 번의 요청에서 만들 최소·최대 수 | 둘 다 기본 1이고 1 이상이다. 두 값이 다르면 요청마다 그 사이(양 끝 포함)에서 무작위로 정한다. Details에서 최소를 최대보다 크게 바꾸면 최대가, 최대를 최소보다 작게 바꾸면 최소가 따라 바뀐다. Blueprint 파생 기본값 등으로 범위가 잘못되면 경고 후 요청을 거절한다 |
| Spawn Area Transform | 스포너 기준 영역의 상대 위치·회전·스케일 | 기본 Identity |
| Area Shape | 영역 모양(Sphere, Box) | 기본 Sphere |
| Sphere Radius | 구 영역의 반지름, cm | 기본 200. Area Shape가 Sphere일 때만 보인다 |
| Box Extent | 상자 영역의 로컬 반경, cm | 기본 (200, 200, 0). Area Shape가 Box일 때만 보인다 |
| Collision Handling | 후보 위치가 막혔을 때 엔진 처리 | 기본 AdjustIfPossibleButAlwaysSpawn. 최종 위치가 영역 밖으로 조정될 수 있다 |
| Calculate Spawn Count / Get Spawn Transform | 수량 계산·개체별 후보 위치 선택 확장점 | C++·Blueprint 파생 설정에서 재정의. 유효한 위치가 없으면 전체 요청을 거절한다 |
| Spawn Characters | 한 번의 생성 작업 시작 | 진행 중이거나 준비 중인 재호출은 거절한다 |
| Cancel Spawning | 대기 요청 취소 | 생성 완료 NPC는 유지한다 |
| Is Spawning / Get Pending Spawn Count | 작업 상태와 대기 수 | 제출 전 예약도 대기 수에 포함한다 |
| Get Spawned Characters / Get Spawned Character Count | 이전 작업을 포함한 유효 생성 개체 조회 | 파괴된 개체는 제외한다. 생존·사망 판정은 아니다 |

활성 `Spawn Area`는 하나만 둔다. 둘 이상이면 요청을 거절한다. 다른 종류의 옵션은 같은 배열에 추가할 수 있다.
영역과 액터의 스케일은 위치 계산에 적용하며, 영역이 캐릭터 스케일을 변경하지 않는다.
요청 시 활성 설정 객체를 복사하고 위치를 미리 계산한다. 진행 중 Character Id·배열·Enabled·수량·영역을 편집해도 다음 작업부터 적용된다.
완료 후 다시 호출하면 추가 개체를 생성한다. 전체 최소·최대 수를 유지하는 정책은 아직 없다.

성공·실패 이벤트의 Spawn Index는 작업 안에서 0부터 시작한다. 비동기 완료 순서는 요청 순서와 다를 수 있다.
전체 완료 이벤트는 성공 수·실패 수·Cancelled를 전달한다. 취소된 대기 요청은 성공·실패에 포함하지 않는다.
파생 설정의 `Calculate Spawn Count`가 0을 반환한 작업은 함수 안에서 즉시 완료 이벤트를 실행한다. 기본 Spawn Area는 최소 1이라 0개 작업이 생기지 않는다. 요청 준비 중 이벤트에서 같은 스포너를 재호출하면 거절한다.
명시적 취소는 완료 이벤트를 실행하지만 스포너 EndPlay에서는 실행하지 않는다.

## NavMesh 위에 생성하기

`Spawner Components` 배열에 `Nav Mesh Projection`을 추가하면 후보 위치를 가장 가까운 NavMesh 위치로 옮긴다. 레벨에 NavMesh가 빌드되어 있어야 한다.

| 항목 | 의미 | 기본값·실패 시 동작 |
|---|---|---|
| 탐색 범위 | Spawn Area 영역 크기를 따른다. Sphere는 반지름, Box는 가장 긴 Extent에 영역·스포너의 가장 큰 축 스케일을 곱한다 | Spawn Area가 없으면 NavMesh의 기본 탐색 범위를 쓴다 |
| Max Attempts | 개체 하나의 위치를 정하려고 후보를 뽑아 투영하는 최대 횟수 | 기본 5. 범위 안에 NavMesh가 없으면 Spawn Area에서 후보를 다시 뽑는다. 모두 실패한 개체는 생성하지 않고 `On Character Spawn Failed`로 알린다 |
| Height Offset | 투영한 위치에서 위로 올릴 높이, cm | 기본 0. NavMesh 위치는 바닥 표면이라 0이면 Collision Handling이 위치를 조정한다. 캐릭터 캡슐 절반 높이를 넣으면 바닥 위에 바로 놓인다 |

## 옵션 확장

`UKataSpawnerComponent`의 C++·Blueprint 파생 설정을 만들고 `Spawner Components` 배열의 항목으로 선택한다.
현재 `On Character Spawned` 훅은 스포너·캐릭터·요청 당시 캐릭터 ID를 전달한다. 설정 사본에 실행 상태를 저장하지 않고, 필요한 처리는 전달된 스포너나 캐릭터에 적용한다.
옵션은 배열 순서로 통지받으며, 앞선 콜백에서 작업을 취소하거나 캐릭터를 제거하면 이후 통지를 중단한다.
위치를 보정하는 옵션은 `Adjust Spawn Transform`을 재정의한다. 스포너는 Spawn Area가 고른 후보를 활성 옵션에 배열 순서로 넘기고, 하나라도 false를 반환하면 후보를 다시 뽑는다.
최대 시도 횟수는 활성 옵션의 `Get Placement Attempts` 중 가장 큰 값이다(기본 1).
이 훅은 캐릭터 BeginPlay 이후다. StateTree·Sense의 초기 설정, 추가 참조 로딩과 적용 시점은 해당 옵션을 구현할 때 설계한다.

## 제한과 문제 해결

- 클래스 선택 항목은 전체 재빌드·에디터 재시작 후 확인한다. 이전 ActorComponent 설정은 인라인 배열로 다시 작성한다.
- `bSpawnOnBeginPlay`는 기본 true이며 게임 시작 시 한 번 생성한다. 에디터의 `SphereAreaPreview` 구·`SpawnAreaPreview` 상자와 `CharacterPreview` 메시로 영역과 캐릭터를 미리 보여 준다. 전용 편집 기즈모는 제공하지 않는다.
- 잘못된 Character Id·중복 Spawn Area·음수 수량·유효하지 않은 Transform은 false와 `LogKataFramework` 경고로 알린다.
- 에셋 로드·캐릭터 생성 실패는 개별 실패 이벤트와 전체 완료 결과로 받는다.
- NavMesh 투영은 `Nav Mesh Projection` 옵션으로 제공한다. 지면 맞춤·개체 간격 보장, 자동 재생성·NPC 제거·Roaming·AI Override는 아직 없다.
- 이 작업에서 샘플 Blueprint·레벨 에셋은 만들지 않았다.

## 확인 상태와 근거

GEComponent의 인라인 설정 패턴을 참고해 소스와 사용 절차를 변경했다. 에이전트는 빌드·테스트·별도 검사·UI 실행을 수행하지 않았다.
2026-10-03 사용자가 현재 구조를 빌드하고 스폰이 정상 동작함을 보고했다. 개별 시나리오의 결과는 별도로 보고되지 않았다.

- [스포너 액터](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataCharacterSpawner.h): 설정 배열과 생성·취소·결과 계약.
- [설정 기반](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataSpawnerComponent.h): 인라인 UObject와 완료 통지.
- [수량·영역 설정](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataSpawnerComponent_SpawnArea.h): Spawn Area와 계산 확장점.
- [결정과 구현 기록](../devlog/2026-09-30-Spawner-Component-Design.md).
- [작업 상태](https://github.com/jaykop/Kata/issues).

2026-10-03 기존 상태 기록에 남은 사용자 PIE 보고에서는 생성 NPC의 착지와 에디터 영역·캐릭터 미리보기를 확인했다. 이번 문서 이전 작업에서 빌드·PIE를 다시 실행하지 않았다.
