# 스포너 사용법

갱신: 2026-10-11  
대상: KataFramework의 NPC 스포너와 인라인 설정 객체  
적용 기준: #21의 GEComponent 방식 최소 스포너, [#41](https://github.com/jaykop/Kata/issues/41)의 사망 기록  
확인 상태: 2026-10-04 사용자가 행 ID 전환, Source Table 필터, Spawn Area(구·상자), 최소·최대 수량, Nav Mesh Projection을 Editor 빌드 후 실행으로 확인했다. 2026-10-03 사용자 빌드와 스폰 동작 확인. 수량·영역·실패·취소와 선택기 필터의 개별 결과는 보고되지 않았다. 2026-10-11 사용자가 거리 관리 스포너에서 죽은 NPC가 다시 생성되지 않는 것과 시체 제거 시 AIController가 함께 정리되는 것을 PIE로 확인했다. `On Character Died`·`Get Dead Character Count`의 값은 따로 보고되지 않았다.

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
5. `Activation`에서 생성 시작 방식을 고른다. `Begin Play`(기본값)는 게임 시작 시 BeginPlay에서 `Spawn Characters`를 한 번 호출한다.
   `Manual`은 자동으로 생성하지 않으므로 트리거·레벨 Blueprint·게임 이벤트 등에서 `Spawn Characters`를 직접 호출한다. `Player Distance`는 [플레이어 거리로 생성·제거하기](#플레이어-거리로-생성제거하기)를 따른다. `Disabled`는 레벨에 배치한 스포너를 지우지 않고 끈다. 자동으로 생성하지 않고 `Spawn Characters`·`Cancel Spawning`·`Despawn Characters` 호출도 경고 로그와 함께 거절한다.
6. `On Character Spawned`, `On Character Spawn Failed`, `On Batch Finished`를 바인딩해 결과를 받는다. `Spawn Characters=false`는 요청 준비 단계에서 거절됐다는 뜻이므로 로그를 확인한다.

여러 프레임에 나눠 생성하려면 같은 카테고리에서 `Use Time Slicing`을 켠다(기본 false). Project Settings > Plugins > Kata Spawner에서 월드 공용 예산을 설정한다. Begin Play와 Manual 모두 이 옵션을 따른다. Player Distance는 항상 분산하고 Disabled는 생성하지 않으므로 이 옵션을 숨긴다.

Sphere는 구 내부에서 위치를 균일하게 선택하므로 영역 원점보다 아래쪽 위치도 후보가 된다. 바닥 아래가 후보에 들어가지 않게 영역 높이와 반지름을 정한다.
Box는 Z Extent가 0이면 영역 원점을 지나는 평면에서 위치를 고르고, Z를 늘리면 부피 안에서 고른다.
영역은 캐릭터 중심이 놓일 높이에 설정한다. 바닥 높이로 설정하면 캡슐이 겹칠 수 있으며 실제 위치는 Collision Handling의 영향을 받는다.
활성 `Spawn Area`가 없으면 스포너 액터 Transform에서 1개를 생성한다.

## 주요 설정과 실행 계약

| 항목 또는 API | 의미 | 기본값·실패 시 동작 |
|---|---|---|
| Source Table | 캐릭터를 고를 NPC 테이블 | 선택기에는 컬렉션의 NPC 테이블 목록에 있는 테이블만 나온다. 비우면 모든 NPC 테이블. Blueprint로 목록에 없는 테이블을 넣으면 드롭다운이 비고 요청을 거절한다 |
| Character Id | 생성할 NPC 캐릭터 ID | Details 드롭다운에는 Source Table(비었으면 모든 NPC 테이블)의 행만 나온다. Blueprint에서 비우거나 그 범위에 없는 ID를 넣으면 요청을 거절한다. 행 핸들 방식에서 옮긴 스포너는 값이 비어 있으므로 다시 고른다 |
| Faction Override | 이 스포너가 만드는 캐릭터의 팩션. 배치를 시작할 때 행 사본의 Faction을 덮어쓴다 | 비우면 행의 Faction, 행도 비었으면 Character Class의 기본값을 쓴다. 원본 테이블은 바뀌지 않으며 다음 배치부터 적용한다 |
| Activation | 생성을 시작하는 방식(Begin Play, Manual, Player Distance, Disabled) | 기본 Begin Play. BeginPlay에서 한 번 읽으며 이후 변경은 반영하지 않는다 |
| Spawn Distance / Despawn Distance | Player Distance의 진입·이탈 거리, cm | 기본 3000·4000. Activation이 Player Distance일 때만 보인다 |
| Use Time Slicing | 위치 준비·로드 제출·실제 생성을 월드 공용 예산으로 분산 | 기본 false. 시작 시 고정하며 변경은 다음 배치부터 적용한다 |
| Despawn On End Play | 스포너 종료 시 생성 NPC 정리를 관리자에게 넘김 | 기본 false. 월드 전체 종료는 엔진 정리를 따름. 이미 시작한 디스폰은 이 옵션과 관계없이 계속 진행 |
| Spawner Components | Details에서 소유·편집하는 설정 객체 배열 | 빈 항목과 비활성 항목은 건너뛴다 |
| Enabled | 다음 생성 작업에 해당 설정을 포함할지 여부 | 기본 true |
| Min Spawn Count / Max Spawn Count | 한 번의 요청에서 만들 최소·최대 수 | 둘 다 기본 1이고 1 이상이다. 두 값이 다르면 요청마다 그 사이(양 끝 포함)에서 무작위로 정한다. Details에서 최소를 최대보다 크게 바꾸면 최대가, 최대를 최소보다 작게 바꾸면 최소가 따라 바뀐다. Blueprint 파생 기본값 등으로 범위가 잘못되면 경고 후 요청을 거절한다 |
| Spawn Area Transform | 스포너 기준 영역의 상대 위치·회전·스케일 | 기본 Identity |
| Area Shape | 영역 모양(Sphere, Box) | 기본 Sphere |
| Sphere Radius | 구 영역의 반지름, cm | 기본 200. Area Shape가 Sphere일 때만 보인다 |
| Box Extent | 상자 영역의 로컬 반경, cm | 기본 (200, 200, 0). Area Shape가 Box일 때만 보인다 |
| Collision Handling | 후보 위치가 막혔을 때 엔진 처리 | 기본 AdjustIfPossibleButAlwaysSpawn. 최종 위치가 영역 밖으로 조정될 수 있다 |
| Calculate Spawn Count / Get Spawn Transform | 수량 계산·개체별 후보 위치 선택 확장점 | C++·Blueprint 파생 설정에서 재정의. 잘못된 수량은 요청 거절. 잘못된 후보 위치는 일반 모드에서 전체 요청 거절, 분산 모드에서는 해당 개체 실패 |
| Spawn Characters | 한 번의 생성 작업 시작 | 생성·제거 진행 중이거나 설정 준비 중이면 거절한다 |
| Cancel Spawning | 대기 요청 취소 | 생성 완료 NPC는 유지한다 |
| Despawn Characters | 대기 생성 취소 후 이전 배치를 포함한 이 스포너의 NPC·소유 AIController 제거 | 수락하면 true. 준비·제거 진행 중과 월드 종료 중에는 false. 다음 관리자 Tick부터 처리 |
| Is Despawning / Get Pending Despawn Count | 제거 상태와 아직 정리하지 않은 생성 기록 수 | NPC 제거 뒤에도 소유 Controller가 남았으면 진행 중이다. 외부에서 제거한 개체의 기록도 대기 수에 포함할 수 있음 |
| On Despawn Finished | 제거 완료 이벤트 | 제거한 NPC 수·NPC 또는 Controller의 Destroy 실패 수. 종료 중에는 호출하지 않음 |
| Is Spawning / Get Pending Spawn Count | 작업 상태와 대기 수 | 제출 전 예약도 대기 수에 포함한다 |
| Get Spawned Characters / Get Spawned Character Count | 이전 작업을 포함한 유효 생성 개체 조회 | 파괴된 개체는 제외하고 아직 제거되지 않은 시체는 포함한다. 생존·사망 판정은 아니다 |
| On Character Died | 이 스포너가 만든 NPC가 죽었을 때 한 번. 사망 정리 뒤 사망 연출 전에 온다 | 종료 중에는 호출하지 않는다. 사망 흐름은 [사망 사용법](Death.md)을 따른다 |
| Get Dead Character Count | 이 스포너가 만든 NPC 중 지금까지 죽은 수 | 시체를 제거해도 줄지 않는다. 재생성 정책에는 쓰지 않는다 |
| 시체 제거 | 사망 Ability가 제거를 요청하면 공용 제거 큐가 NPC와 소유 AIController를 함께 정리한다 | `Is Despawning`·`On Despawn Finished`에 포함하지 않는다. 수동 `Despawn Characters`는 시체를 포함한 전체 기록을 제거한다. 스포너가 먼저 사라졌으면 NPC를 직접 Destroy한다 |

활성 `Spawn Area`는 하나만 둔다. 둘 이상이면 요청을 거절한다. 다른 종류의 옵션은 같은 배열에 추가할 수 있다.
영역과 액터의 스케일은 위치 계산에 적용하며, 영역이 캐릭터 스케일을 변경하지 않는다.
배치 시작 시 캐릭터 ID와 NPC 행, 활성 설정 객체, 스포너 Transform과 실행 방식을 복사한다. 진행 중 Character Id·NPC 행·배열·Enabled·수량·영역·Use Time Slicing을 편집해도 다음 작업부터 적용된다.
NavMesh 탐색 범위도 배치 시작 시 스포너 Transform의 스케일을 따른다. 위치 선택 훅에서 액터를 이동하거나 스케일을 바꿔도 이번 배치의 기본 후보 영역과 NavMesh 탐색 범위는 유지된다.
기존 Blueprint 위치 보정 훅의 매개변수는 유지한다. 파생 훅이 액터의 현재 Transform이나 원본 테이블을 직접 읽는 경우에는 그 훅 자체의 계산까지 고정하지 않는다.
배치 내부에는 위치 계산·보정·제출의 진행 커서와 전체 미완료 수를 보관한다. `Get Pending Spawn Count`는 아직 제출하지 않은 개체까지 포함하며, 완료 판단은 제출한 핸들 맵의 크기와 분리한다.
일반 모드는 모든 후보를 같은 호출에서 확인한 뒤 요청을 제출한다. 분산 모드는 수락 후 후보 선택·보정·재시도·확정을 작업별로 진행하고, 준비한 위치 하나씩 로드 요청을 제출한다. 전체 수량만큼 위치 배열을 미리 만들지 않는다. 플레이어 거리 기반 생성·제거는 아래 [플레이어 거리로 생성·제거하기](#플레이어-거리로-생성제거하기)를 따른다.
완료 후 다시 호출하면 추가 개체를 생성한다. 전체 개체 수를 최소·최대 범위로 유지하는 정책은 아직 없다.

### 분산 모드의 공용 설정

`UKataSpawnerSubsystem`만 Tick하며, `UKataCharacterSpawnSubsystem`은 로드 완료 요청을 그룹별 준비 큐에 보관한다. 관리자는 생성 차례와 위치 준비 차례를 따로 순환한다. 생성 쪽은 준비 큐 소비 후 위치 준비·제출 순서이며, 제거 쪽과 먼저 처리할 순서를 프레임마다 교대해 같은 시간 예산을 공유한다.

| Project Settings > Plugins > Kata Spawner | 초기값 | 계약 |
|---|---|---|
| Time Budget Ms | 1.0 ms | 작업 사이에서 확인하는 시간 상한 |
| Max Preparation Steps Per Frame | 64 | 위치 준비·제출 방문 상한. 준비 큐 확인에도 별도로 같은 방문 상한을 적용한다 |
| Max Spawn Attempts Per Frame | 2 | 실제 캐릭터 생성 시도 상한. 생성 실패도 포함한다 |
| Max Despawn Steps Per Frame | 2 | NPC 또는 소유 Controller 하나를 처리하는 단계 상한. 이미 무효한 기록 확인도 포함한다 |
| Max Outstanding Requests | 64 | 모든 분산 그룹의 로드 중·준비 완료 요청을 합한 상한 |
| Max Outstanding Requests Per Spawner | 8 | 한 스포너의 로드 중·준비 완료 요청을 합한 상한 |
| Distance Evaluation Interval | 0.25 s | 거리 관리 스포너 전체를 한 번 평가하는 최소 간격. 한 평가가 여러 프레임에 걸치면 끝난 뒤 다음 간격을 센다 |
| Max Distance Checks Per Frame | 16 | 한 프레임에 거리를 평가하는 스포너 수. 스포너 하나에는 원점과 그 스포너의 모든 NPC 판정이 포함된다 |
| Grid Cell Size | 5000 cm | 거리 관리 스포너를 찾는 셀 한 변의 길이. 최소 100. 월드 시작 때 고정한다 |

기본 수치는 조정용 초기값이며 성능 측정에 근거한 보장값이 아니다. 정수 설정은 런타임에서도 최소 1, 시간은 최소 0.01 ms로 제한하고 유효하지 않은 시간값은 1.0 ms로 처리한다. 공용 설정은 다음 관리자 Tick에서 읽는다. 대기 상한을 낮춰도 기존 요청을 취소하지 않으며 수량이 줄어들 때까지 추가 제출을 보류한다.

시간 제한은 개별 작업을 중간에 끊지 않는 부드러운 상한이다. 캐릭터 하나의 Deferred Spawn·행 적용·FinishSpawning·BeginPlay·결과 이벤트, 위치 보정 훅 하나는 상한을 넘길 수 있다. 시작 시 행·설정 사본 생성과 수량 계산, 실제 에셋 로드·GC, 일반 생성 API와 분산 옵션을 끈 스포너의 비용은 이 예산에 포함하지 않는다.

NPC·Controller의 Destroy와 종료 콜백도 단계 중간에 끊지 않는다. 디스폰 시작 호출에서 기존 생성 요청을 취소하는 정리는 동기 처리하고, NPC·소유 Controller의 제거 큐 실행에 공용 예산을 적용한다.

분산 모드의 `Spawn Characters=true`는 수락이다. 수락 후 잘못된 후보 위치·보정 실패·로드 실패·생성 실패는 해당 개체의 실패 이벤트로 받는다. `Get Pending Spawn Count`는 아직 위치를 준비하지 않은 개체도 포함한다.

`Cancel Spawning`은 미제출 작업과 로드·준비 큐를 함께 취소한다. 이미 결과를 전달한 NPC는 유지한다. 분산 생성의 BeginPlay에서 해당 배치를 취소하거나 스포너를 종료하면 아직 결과를 전달하지 않은 새 NPC와 직접 생성한 AIController를 정리하며 성공·실패 이벤트를 생략한다. 별도 디스폰을 요청한 경우에는 그 제거 작업에 맡긴다. 월드 종료 시 등록과 대기 요청을 결과 이벤트 없이 정리한다.

성공·실패 이벤트의 Spawn Index는 작업 안에서 0부터 시작한다. 비동기 완료 순서는 요청 순서와 다를 수 있다.
전체 완료 이벤트는 성공 수·실패 수·Cancelled를 전달한다. 취소된 대기 요청은 성공·실패에 포함하지 않는다.
파생 설정의 `Calculate Spawn Count`가 0을 반환한 작업은 함수 안에서 즉시 완료 이벤트를 실행한다. 기본 Spawn Area는 최소 1이라 0개 작업이 생기지 않는다. 요청 준비 중 이벤트에서 같은 스포너를 재호출하면 거절한다.
명시적 취소는 완료 이벤트를 실행하지만 스포너 EndPlay에서는 실행하지 않는다.

## 생성한 NPC 제거하기

`Despawn Characters`를 직접 호출한다. 플레이어 거리로 자동 제거하려면 Activation을 Player Distance로 고르며, 그 스포너에서는 이 수동 호출을 거절한다. 일반 생성과 분산 생성 모두 같은 제거 API를 사용하고, 제거는 `Use Time Slicing`과 관계없이 월드 공용 예산으로 진행한다.

1. 생성한 NPC가 있는 스포너 참조에서 `Despawn Characters`를 호출한다.
2. 생성 중이었다면 대기를 취소하고 `On Batch Finished`를 Cancelled=true로 알린다. 이 이벤트보다 먼저 제거 상태를 설정하므로 이벤트 안의 새 생성은 거절한다.
3. `Is Despawning`이 true인 동안 NPC와 그 NPC가 직접 생성한 AIController를 순차 정리한다. 다른 스포너나 외부에서 생성한 NPC는 대상이 아니다.
4. `On Despawn Finished` 이후 다시 `Spawn Characters`를 호출할 수 있다. 빈 작업도 다음 관리자 Tick에서 0·0으로 완료한다. 제거 중 반복 호출은 false다.

생성 기록에는 배치의 세대 ID와 약한 NPC·Controller 참조를 보관한다. 공개 수동 제거는 이전 배치를 포함한 이 스포너의 전체 생성 기록을 대상으로 한다. 아직 결과 콜백을 받기 전인 NPC도 생성 도중 디스폰을 요청하면 같은 제거 작업으로 추적한다.

Controller 소유는 `SpawnDefaultController`가 실제로 생성한 결과로 판별한다. 외부 Controller가 NPC를 빙의했다고 소유 기록에 넣지 않는다. 기록한 Controller가 다른 Pawn으로 이전하면 소유 기록에서 제외하며 제거 직전에도 다른 Pawn을 빙의하는지 확인한다. PlayerController는 제거하지 않는다. NPC 종료의 기존 AI·StateTree 정리 경로를 사용하고, 디스폰에서는 빙의를 해제한 뒤 소유 AIController를 별도 단계에서 제거한다.

NPC가 Destroy를 거절하면 해당 Controller도 유지한다. 실패 기록은 완료 후 스포너에 반환하며 다시 `Despawn Characters`를 호출해 재시도할 수 있다. `Failed Actor Count`는 NPC와 소유 Controller의 Destroy 실패를 센다. 이미 제거됐거나 외부로 이전한 Controller는 실패가 아니다. 스포너가 종료돼 결과를 받을 수 없는 제거 실패는 `LogKataFramework` 경고로 확인한다.

`Despawn On End Play`를 켜면 스포너 파괴·레벨 언로드 중에도 남은 월드 관리자가 생성 기록을 넘겨받아 정리한다. 이미 시작한 제거 작업은 옵션이 꺼져 있어도 스포너 종료 후 계속한다. 이때 결과 이벤트는 호출하지 않는다. 월드 전체 종료는 새 제거 작업을 만들지 않고 엔진의 월드 정리를 따른다. 기본 false인 일반 스포너는 기존처럼 완료 NPC를 유지한다.

### 사용자 수동 확인 절차

빌드·실행 미확인인 안내다. 샘플 에셋이나 자동화 테스트는 추가하지 않았다.

- Spawn Area 수량을 3으로 고정하고 생성한 뒤, 기존 샘플 입력이나 Level Blueprint에서 스포너 참조의 `Despawn Characters`를 호출한다. 화면의 NPC가 없어지는지 확인한다.
- PIE의 World Outliner에서 해당 NPC와 직접 생성된 AIController가 정리되는지 확인한다. Controller Class와 Auto Possess AI 설정으로 실제 생성된 Controller가 있어야 한다.
- `On Despawn Finished`에서 다시 `Spawn Characters`를 호출해 재생성을 확인한다.
- 분산 생성 도중 제거를 호출하면 추가 생성이 멈추고 이미 나온 NPC도 사라지는지 확인한다.

프레임별 제거 상한·완료 이벤트 횟수는 눈으로 판단하는 항목이 아니다. 별도 진단 출력 없이 화면에서 정상처럼 보였다는 이유로 그 계약까지 확인한 것으로 기록하지 않는다.

## 플레이어 거리로 생성·제거하기

스포너의 `Activation`을 `Player Distance`로 고르면 월드 관리자가 플레이어 Pawn과의 거리로 이 스포너의 생성·제거를 관리한다. 상태는 보존하지 않으며 다시 생성한 NPC는 초기 상태다.

| 항목 | 의미 | 기본값·실패 시 동작 |
|---|---|---|
| Spawn Distance | 스포너 원점과 플레이어 Pawn의 3D 거리가 이 값 이하이면 진입, cm | 기본 3000. 스포너를 선택하면 초록 구로 보인다 |
| Despawn Distance | NPC 또는 원점과 플레이어 Pawn의 거리가 이 값을 넘으면 이탈, cm | 기본 4000. Spawn Distance보다 커야 한다. 아니면 경고 후 이 스포너는 생성하지 않는다. 스포너를 선택하면 빨간 구로 보인다 |
| Is Distance Managed | BeginPlay에서 유효한 거리 설정을 고정해 관리 중인지 | Blueprint Pure |
| Get Pending Respawn Count | 다음 원점 진입 때 다시 생성할 수 | 최초 진입 전에는 0 |

동작 규칙은 다음과 같다.

1. BeginPlay에서 Activation과 두 거리를 고정한다. 이후 변경은 반영하지 않는다. BeginPlay에서 바로 생성하지 않고 첫 거리 평가를 기다린다.
2. 스포너 원점이 Spawn Distance 안에 들어오면 한 번 생성을 시작한다. 최초 진입은 Spawn Area의 수량을 그때 한 번 정하고, 이후 진입은 거리로 제거한 수만큼만 생성한다. 위치는 매번 Spawn Area에서 새로 고른다.
3. 원점은 Despawn Distance 밖으로 나가야 이탈로 본다. 이탈 시 진행 중인 생성을 취소하고 아직 생성하지 않은 수를 다음 진입 때 생성할 수에 더한다. 범위 안에 머무는 동안에는 다시 생성하지 않는다.
4. 생성한 NPC는 스포너 원점이 아니라 자기 현재 위치로 판정한다. 플레이어와의 거리가 Despawn Distance를 넘은 NPC만 개별로 제거하고 다음 진입 때 생성할 수에 더한다. NPC가 원점에서 멀리 이동해도 플레이어 근처에 있으면 유지한다.
5. 생성 실패, 외부 Destroy 등으로 이미 사라진 NPC는 다시 생성하지 않는다. NPC가 Destroy를 거절하면 다음 평가에서 다시 판정한다.
   죽은 NPC는 거리 평가에서 빼므로 거리로 제거하거나 다시 생성할 수에 더하지 않는다. 시체는 사망 흐름이 제거한다.
6. 생성은 `Use Time Slicing`과 관계없이 분산 경로를 쓴다. 거리 제거는 수동 제거와 같은 공용 제거 큐로 처리하며 `Is Despawning`·`On Despawn Finished`에는 포함하지 않는다.
7. 거리 관리 중에는 `Spawn Characters`·`Despawn Characters`가 false를 반환하고 `Cancel Spawning`은 무시한다. `On Character Spawned`·`On Character Spawn Failed`·`On Batch Finished`는 그대로 호출되며, 원점 이탈로 취소한 생성은 Cancelled=true다.
8. 플레이어 Pawn이 없으면 평가를 보류한다. 플레이어 상실을 이탈로 보지 않으며 기존 NPC를 유지한다. 플레이어 판정은 첫 번째 PlayerController의 Pawn이다.

현재 제한은 다음과 같다. 평가 주기 사이의 판정이므로 이탈 판정 후 실제 Destroy까지 몇 프레임 지연이 있고, 그 사이 플레이어가 돌아와도 제거를 취소하지 않는다. 거리 범위는 스포너를 선택했을 때만 와이어 구로 그리며, 셀은 에디터에 보여 주지 않는다.

이전 버전의 `Spawn On Begin Play`를 끈 스포너는 로드할 때 Manual로 자동으로 옮긴다. 레벨을 다시 저장하면 변경이 유지된다. 레벨을 다시 저장하려면 액터를 조금 수정해 레벨이 수정 상태(`*`)가 되게 한다. 이전 `Distance Activation` 항목은 테스트 레벨을 이전·재저장한 뒤 제거했으며 더 이상 읽지 않는다.

### 그리드로 평가 대상 좁히기

관리자는 거리 관리 스포너를 모두 순회하지 않고 그리드 셀로 평가 대상을 고른다. 셀은 에셋이나 액터가 아니며 레벨에서 편집할 대상이 없다.

- 셀은 월드 XY 평면을 `Grid Cell Size` 정사각형으로 나눈 칸이다. 셀 좌표는 X·Y를 각각 셀 크기로 나눈 뒤 내림한 정수이며 높이는 무시한다.
- 거리 관리 스포너는 BeginPlay 위치의 셀에 한 번 등록되고 EndPlay에서 빠진다. 실행 중 스포너를 옮겨도 셀은 바뀌지 않으므로 거리 관리 스포너는 고정 배치로 사용한다.
- 평가를 시작할 때 플레이어가 있는 셀에서 등록된 가장 큰 Spawn Distance를 덮는 반경의 셀에 있는 스포너만 고른다. 실제 진입 판정은 그 뒤 3D 거리로 한다.
- 원점이 범위 안이거나, 생성 중이거나, NPC 기록이나 진행 중인 거리 제거가 남은 스포너는 조회 셀 밖이어도 계속 평가한다. 플레이어를 따라온 NPC나 멀리 떠난 원점의 이탈을 놓치지 않기 위해서다.
- 셀 크기는 성능 설정이며 생성·제거 거리와 관계없다. 월드 시작 때 읽으므로 변경은 다음 PIE·월드부터 적용된다. Spawn Distance보다 너무 작으면 조회할 셀이 많아지고, 너무 크면 한 셀에 많은 스포너가 모인다. 조회 반경이 점유 셀 수보다 넓으면 점유 셀만 반경으로 거른다.

### 거리 관리 수동 확인 절차

빌드·실행 미확인인 안내다.

- Spawn Area 수량을 고정한 스포너의 Activation을 Player Distance로 고르고, 플레이어 시작 위치를 Spawn Distance 밖에 둔다. 시작 직후 NPC가 없고, 다가가면 생성되는지 확인한다.
- NPC 하나만 플레이어로부터 Despawn Distance 밖으로 떨어뜨렸을 때 그 개체만 사라지는지 확인한다. 같은 스포너의 가까운 NPC는 유지돼야 한다.
- 원점에서 Despawn Distance 밖으로 나갔다가 돌아오면, 거리로 제거된 수만큼만 원점 영역에 다시 생성되는지 확인한다. 범위 안에 머무는 동안에는 늘어나지 않아야 한다.
- Spawn Distance와 Despawn Distance 사이를 오가면 생성·제거가 반복되지 않는지 확인한다.
- 생성된 NPC를 외부에서 Destroy한 뒤 이탈·재진입해도 그 개체는 다시 생성되지 않는지 확인한다.
- 이 스포너에서 `Spawn Characters`·`Despawn Characters`를 호출하면 false와 경고 로그가 나오는지 확인한다.

## NavMesh 위에 생성하기

`Spawner Components` 배열에 `Nav Mesh Projection`을 추가하면 후보 위치를 가장 가까운 NavMesh 위치로 옮긴다. 레벨에 NavMesh가 빌드되어 있어야 한다.

| 항목 | 의미 | 기본값·실패 시 동작 |
|---|---|---|
| 탐색 범위 | Spawn Area 영역 크기를 따른다. Sphere는 반지름, Box는 가장 긴 Extent에 영역·스포너의 가장 큰 축 스케일을 곱한다 | Spawn Area가 없으면 NavMesh의 기본 탐색 범위를 쓴다 |
| Max Attempts | 개체 하나의 위치를 정할 때 후보 위치를 선택해 NavMesh에 투영하는 최대 횟수 | 기본 5. 범위 안에 NavMesh가 없으면 Spawn Area에서 후보를 다시 뽑는다. 모두 실패한 개체는 생성하지 않고 `On Character Spawn Failed`로 알린다 |
| Height Offset | 투영한 위치에서 위로 올릴 높이, cm | 기본 0. NavMesh 위치는 바닥 표면이라 0이면 Collision Handling이 위치를 조정한다. 캐릭터 캡슐 절반 높이를 넣으면 바닥 위에 바로 놓인다 |

## AI 설정 덮어쓰기

`Spawner Components` 배열에 `AI Override`를 추가하면 이 스포너가 만드는 NPC의 AI 설정을 배치마다 바꾼다. 같은 NPC 행으로 경비·순찰처럼 행동이 다른 개체를 배치할 때 쓴다.
배치를 시작할 때 행 사본에 기록하므로 캐릭터 BeginPlay 전에 적용되고, 설정 변경은 다음 배치부터 반영된다. 원본 테이블과 AI Data 에셋은 바뀌지 않는다.
한 스포너에서 AI Override는 하나만 켤 수 있다. 둘 이상 켜면 `Spawn Characters`가 false를 반환하고 경고 로그를 남긴다.

| 항목 | 의미 | 기본값·실패 시 동작 |
|---|---|---|
| Disable AI | 인지·행동 로직 없이 생성한다. Controller는 빙의하지만 StateTree·Perception·AI 타게팅을 시작하지 않는다 | 기본 false. 켜면 아래 항목은 비활성으로 표시되고 무시된다 |
| AI Data | 행의 AI Data 대신 기준으로 쓸 AI Data | 비우면 행의 AI Data를 쓴다. 생성 전에 행의 다른 에셋과 함께 비동기로 로드한다 |
| Overrides > Linked State Tree Slots | 기준 AI Data의 Linked 슬롯에 태그 단위로 병합할 하위 트리 | 같은 태그는 교체하고 기준에 없는 태그는 추가한다. 태그가 빈 항목은 경고 후 제외하고, 같은 태그가 둘이면 뒤 항목을 쓴다. 슬롯 규칙은 [KataAI 사용법](AI.md)을 따른다 |
| Overrides > Leash Distance | Home에서 허용할 추격 거리, cm | 체크박스를 켰을 때만 덮어쓴다. 0이면 무제한 |

슬롯이나 Leash Distance를 덮어쓰면 캐릭터마다 기준 AI Data의 Transient 사본을 만들어 적용한다. 둘 다 비워 두면 사본을 만들지 않고 기준 에셋을 그대로 쓴다.
기준 AI Data가 없는데 슬롯이나 Leash Distance만 덮어쓰면, 덮어쓰기를 무시하고 `LogKataAI` 경고를 남긴 뒤 AI를 시작하지 않는다. 슬롯과 추격 거리는 마스터 StateTree가 있어야 의미가 있기 때문이다. AI 없이 생성하려면 `Disable AI`를 켠다.
Senses·Targeting Preset 같은 몬스터 타입 단위 설정은 덮어쓰지 않는다. 이런 값이 다르면 별도 AI Data를 만들어 `AI Data`에 지정한다.

## 옵션 확장

`UKataSpawnerComponent`의 C++·Blueprint 파생 설정을 만들고 `Spawner Components` 배열의 항목으로 선택한다.
현재 `On Character Spawned` 훅은 스포너·캐릭터·요청 당시 캐릭터 ID를 전달한다. 설정 사본에 실행 상태를 저장하지 않고, 필요한 처리는 전달된 스포너나 캐릭터에 적용한다.
옵션은 배열 순서로 통지받으며, 앞선 콜백에서 작업을 취소하거나 캐릭터를 제거하면 이후 통지를 중단한다.
설정과 행 사본은 배치 실행 객체 `UKataSpawnBatchState`가 참조하며, 현재 콜백이 끝날 때까지 강한 참조로 수명을 유지한다. C++ 보정 구현은 `AKataCharacterSpawner::GetSpawnBatchContext`로 고정 정보를 조회할 수 있다. 반환 포인터는 현재 훅 호출 동안만 사용하고 저장하지 않는다.
위치를 보정하는 옵션은 `Adjust Spawn Transform`을 재정의한다. 스포너는 Spawn Area가 고른 후보를 활성 옵션에 배열 순서로 넘기고, 하나라도 false를 반환하면 후보를 다시 뽑는다.
최대 시도 횟수는 활성 옵션의 `Get Placement Attempts` 중 가장 큰 값이다(기본 1).
On Character Spawned 훅은 캐릭터 BeginPlay 이후에 호출되므로 초기 AI 설정에 쓰지 않는다.
BeginPlay 전에 적용할 값은 C++ 가상 함수 `ModifySpawnRow`에서 이번 배치의 행 사본(`FInstancedStruct`)에 기록한다. 스포너는 Faction Override를 기록하고 중복 설정 검사를 마친 뒤 활성 옵션을 배열 순서로 호출한다.
행 사본은 생성 전 비동기 로드와 캐릭터의 행 적용에 그대로 쓰인다. 새로 지정한 소프트 참조는 행의 `GatherAssetsToLoad`가 수집하는 필드에 있어야 로드된다. Blueprint 파생 옵션에서는 이 함수를 재정의할 수 없다.

## 제한과 문제 해결

- 클래스 선택 항목은 전체 재빌드·에디터 재시작 후 확인한다. 이전 ActorComponent 설정은 인라인 배열로 다시 작성한다.
- Activation 기본값은 Begin Play이며 게임 시작 시 한 번 생성한다. 에디터의 `SphereAreaPreview` 구·`SpawnAreaPreview` 상자와 `CharacterPreview` 메시로 영역과 캐릭터를 미리 보여 준다. 전용 편집 기즈모는 제공하지 않는다.
- 잘못된 Character Id·중복 Spawn Area·중복 AI Override·음수 수량은 false와 `LogKataFramework` 경고로 알린다. 유효하지 않은 후보 Transform은 일반 모드에서는 전체 요청 거절, 분산 모드에서는 개별 실패다.
- 에셋 로드·캐릭터 생성 실패는 개별 실패 이벤트와 전체 완료 결과로 받는다.
- NavMesh 투영은 `Nav Mesh Projection` 옵션으로 제공한다. 지면 맞춤·개체 간격 보장, 이동하는 거리 관리 스포너, 상태를 보존하는 재생성, Roaming은 아직 없다. AI Override는 AI Controller Class와 Home 기준 위치를 바꾸지 않는다.
- 이 작업에서 샘플 Blueprint·레벨 에셋은 만들지 않았다.

## 확인 상태와 근거

GEComponent의 인라인 설정 패턴을 참고해 소스와 사용 절차를 변경했다. 에이전트는 빌드·테스트·별도 검사·UI 실행을 수행하지 않았다.
2026-10-07 분산 모드와 공용 예산·대기 상한·준비 큐를 구현했다. 이 변경의 빌드·실행·UI·프로파일은 아직 확인하지 않았다.
2026-10-07 수동 디스폰·소유 Controller 추적·제거 커서·종료 인계를 추가했다. 이 3단계 변경도 빌드·실행 미확인이다.
2026-10-10 AI Override와 `ModifySpawnRow` 훅을 추가했다([#48](https://github.com/jaykop/Kata/issues/48)). 같은 날 사용자가 빌드 통과와 PIE 확인 완료를 보고했다. 개별 시나리오 결과는 보고되지 않았다.
2026-10-08 Faction Override를 추가했다. 사용자가 빌드 후 팩션 테스트 완료를 보고했다. 개별 시나리오 결과는 보고되지 않았다.
2026-10-07 Distance Activation 항목과 관리자의 거리 평가를 추가했다. 같은 날 사용자가 빌드 후 PIE에서 거리에 따른 생성·제거 동작을 확인했다고 보고했다. 빌드 타깃과 수동 확인 절차의 개별 항목(부분 제거, 재진입 수, 경계 왕복, 외부 제거, 수동 호출 거절) 결과는 보고되지 않았다.
2026-10-07 거리 관리 스포너의 그리드 셀 조회와 활성 스포너 목록을 추가했다. 같은 날 사용자가 빌드 후 거리 생성·제거가 그리드 추가 전과 같게 동작한다고 보고했다. 셀 경계·여러 스포너 배치 시나리오와 성능은 별도로 보고되지 않았다.
2026-10-07 Distance Activation 항목을 스포너의 Activation(Player Distance)으로 옮기고, 이전 설정의 자동 이전과 선택 시 거리 미리보기를 추가했다. 같은 날 사용자가 빌드 후 잘 동작한다고 보고했다. 레벨 재저장 후의 이전 결과와 개별 항목은 별도로 보고되지 않았다.
2026-10-07 사용자가 `LV_TestMap`을 다시 저장해 Player Distance 값이 저장된 것을 파일에서 확인한 뒤 `Distance Activation` 클래스와 이전 코드를 제거했다.
2026-10-09 Activation에 Disabled를 추가했다. 같은 날 사용자가 빌드 후 잘 동작한다고 보고했다.
2026-10-06 배치 실행 분리와 고정 행·NavMesh 기준 연결을 소스에 반영했고, 사용자가 1단계 변경의 빌드 성공을 보고했다. 빌드 타깃은 지정하지 않았으며 생성·실패·취소의 실행 확인은 아직 보고되지 않았다. 이전 사용자 실행 확인 결과는 이번 변경의 실행 검증 결과가 아니다.
2026-10-03 사용자가 현재 구조를 빌드하고 스폰이 정상 동작함을 보고했다. 개별 시나리오의 결과는 별도로 보고되지 않았다.

- [스포너 액터](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataCharacterSpawner.h): 설정 배열과 생성·취소·결과 계약.
- [설정 기반](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataSpawnerComponent.h): 인라인 UObject와 완료 통지.
- [수량·영역 설정](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataSpawnerComponent_SpawnArea.h): Spawn Area와 계산 확장점.
- [AI 설정 덮어쓰기](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataSpawnerComponent_AIOverride.h): AI Override와 행 사본 기록.
- [결정과 구현 기록](../devlog/2026-09-30-Spawner-Component-Design.md).
- [배치 실행 분리 기록](../devlog/2026-10-06-Spawner-Batch-Execution.md).
- [타임슬라이싱 기록](../devlog/2026-10-07-Spawner-Time-Slicing.md).
- [디스폰 수명 기록](../devlog/2026-10-07-Spawner-Despawn-Lifecycle.md).
- [거리 활성화 기록](../devlog/2026-10-07-Spawner-Distance-Activation.md).
- [그리드 조회 기록](../devlog/2026-10-07-Spawner-Distance-Grid.md).
- [생성 방식 통합 기록](../devlog/2026-10-07-Spawner-Activation-Mode.md).
- [AI 설정 덮어쓰기 기록](../devlog/2026-10-10-Spawner-AI-Override.md).
- [작업 상태](https://github.com/jaykop/Kata/issues).

2026-10-03 기존 상태 기록에 남은 사용자 PIE 보고에서는 생성 NPC의 착지와 에디터 영역·캐릭터 미리보기를 확인했다. 이번 문서 이전 작업에서 빌드·PIE를 다시 실행하지 않았다.
