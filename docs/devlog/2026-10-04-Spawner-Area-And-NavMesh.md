# 스포너 영역·수량·NavMesh 확장과 #21 종료

작성: 2026-10-04  
갱신: 2026-10-04  
유형: 결정 기록  
대상: KataFramework 스포너(`AKataCharacterSpawner`, `UKataSpawnerComponent`, `UKataSpawnerComponent_SpawnArea`, `UKataSpawnerComponent_NavMeshProjection`)  
기준: 커밋 `67a8690`. [#21](https://github.com/jaykop/Kata/issues/21) 종료 시점의 상태

## 배경과 결론

2026-09-30 기본 스포너([결정 기록](2026-09-30-Spawner-Component-Design.md)) 뒤에 사용자가 영역 모양, 수량 범위, NavMesh 위 생성을 요청했다.
같은 시기 [#31](https://github.com/jaykop/Kata/issues/31)에서 캐릭터 지정이 행 ID로 바뀌었다. 기본 설정 컴포넌트의 이름을 Spawn Area로 바꾸고, 위치 보정 확장 지점과 NavMesh 투영 설정을 더했다.
사용자가 빌드와 실행으로 확인했고 #21을 닫았다. 이 문서는 #21 계획 문서에서 옮긴 결정도 함께 남긴다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 기본 설정 이름 | `UKataSpawnerComponent_SpawnSettings`("Spawn Settings") | `UKataSpawnerComponent_SpawnArea`("Spawn Area"). Redirect 없이 사용자가 테스트 맵 항목을 다시 추가했다 |
| 영역 모양 | Box만 | `AreaShape` Sphere(기본, `SphereRadius` 200)·Box. 구 내부 부피에서 균일하게 고른다. 에디터 구 미리보기(`SphereAreaPreview`) 추가 |
| 수량 | `SpawnCount`(0이면 생성 없이 완료) | `MinSpawnCount`·`MaxSpawnCount`(둘 다 기본 1, 1 이상, Min≤Max). 다르면 요청마다 사이에서 무작위로 정한다. Details에서 한쪽을 넘기면 다른 쪽이 따라간다 |
| 위치 보정 | 없음 | `UKataSpawnerComponent::AdjustSpawnTransform`·`GetPlacementAttempts` 확장 지점. 스포너가 Spawn Area의 후보를 활성 설정에 배열 순서로 넘기고, 거절되면 다시 뽑는다 |
| NavMesh | 범위 밖 | `Nav Mesh Projection` 설정. 탐색 범위는 Spawn Area 크기를 따르고, 시도가 모두 실패한 개체는 실패 이벤트로 알린다 |
| 캐릭터 지정 | `FDataTableRowHandle CharacterRow` | `Source Table`(컬렉션 NPC 목록 중 하나) + `FKataCharacterId CharacterId`. #31 결정 기록 참고 |

## 주요 결정과 이유

- **Spawn Area라는 이름.** 다른 옵션(Nav Mesh Projection)이 생기자 "Settings"로는 기본 설정과 옵션을 구분할 수 없었다. 이 설정의 주된 역할은 "이 영역에서 몇 개를 뽑을지"라서 Spawn Area로 정했다. 개수와 영역을 두 설정으로 나누는 안은 구조 변경이 커서 택하지 않았다.
- **Sphere 기본값.** 사용자 결정이다. 구는 원점 아래도 후보가 되므로 영역 높이와 반지름을 바닥에 맞춰 정해야 한다(manual에 안내).
- **Min·Max 해석.** 사용자 표현 "Max는 Main보다 낮아야"를 "Min은 Max보다 클 수 없다"로 해석해 구현했고, 사용자가 실행으로 확인했다. Details 밖에서 잘못된 범위가 들어오면 경고 후 요청을 거절한다.
- **NavMesh 투영을 옵션 설정으로.** 스포너 클래스 옵션은 위치 보정이 늘 때마다 클래스가 커지고, Spawn Area 옵션은 위치 선택을 재정의한 파생 설정에서 빠질 수 있다. 어떤 위치 선택 뒤에도 적용되고 지면 맞춤·간격 보장 같은 후속 보정이 같은 방식을 따르도록 별도 설정으로 만들었다.
- **실패 처리.** 사용자 결정으로 후보를 다시 뽑고(b), 끝내 실패한 개체만 실패로 알린다(a). 나머지 개체는 생성된다. 후보를 낼 수 없는 Spawn Area 오류는 설정 오류로 보고 작업 전체를 거절한다.
- **탐색 범위.** 사용자 결정으로 Spawn Area가 있으면 Sphere는 반지름, Box는 가장 긴 Extent를 쓰고 영역·스포너의 가장 큰 축 스케일을 곱한다. 없으면 NavMesh 기본 범위를 쓴다.
- **Height Offset.** 투영 위치는 바닥 표면이라 캐릭터 중심이 묻힌다. 기본 0은 Collision Handling에 맡기고, 캡슐 절반 높이를 넣으면 바로 바닥 위에 놓이게 했다.

## 계획 문서에서 옮긴 결정 (2026-09-30~10-03)

- 테이블·Row 선택은 스포너 본체에 둔다. 옵션은 GEComponent처럼 인라인 UObject 배열로 추가한다.
- `bSpawnOnBeginPlay`(기본 true)가 켜져 있으면 BeginPlay에서 한 번 생성한다. 스포너를 배치하고 게임에 들어갔는데 생성되지 않는다는 사용자 보고로 추가했다.
- 에디터 전용 영역·캐릭터 미리보기를 둔다. 사용자 보고와 요청으로 추가했다.
- 활성 Spawn Area가 없으면 액터 Transform에서 1개를 생성하고, 둘 이상이면 요청을 거절한다.
- 한 스포너는 한 번에 한 작업만 받는다. 요청 설정과 후보 위치는 시작 시 복사하고, 전체 요청을 먼저 예약해 즉시 실패 콜백이 작업을 중간에 완료시키지 않게 한다.
- 취소·제거는 생성 완료 NPC를 제거하거나 다시 생성하지 않는다. 파괴 이벤트를 자동 재생성 기준으로 삼지 않는다.

## 근거

- [KataCharacterSpawner.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataCharacterSpawner.cpp): 후보 재추출과 실패 처리.
- [KataSpawnerComponent_SpawnArea.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataSpawnerComponent_SpawnArea.cpp): 영역 표본 추출, 수량 범위.
- [KataSpawnerComponent_NavMeshProjection.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataSpawnerComponent_NavMeshProjection.cpp): 탐색 범위와 투영.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| Spawn Area 구·상자, Min/Max, Nav Mesh Projection, Source Table | 2026-10-04 사용자 Editor 빌드·실행 보고 | 확인 | 개별 시나리오(재시도 횟수, 경계 영역)의 결과는 따로 보고되지 않음. Game 대상 빌드 |

## 남은 제한과 후속 작업

#21 본문의 초기 범위 중 아래는 구현하지 않고 이슈를 닫았다. 필요해지면 별도 이슈로 만든다.

- 조우 볼륨 발동, 동시 생존 수 상한, 재스폰 규칙.
- 스폰 시 팩션·StateTree·Sense 등 설정 주입. 행동 시작 전 적용 시점과 추가 로딩은 그 옵션을 만들 때 정한다. 현재 완료 훅은 캐릭터 BeginPlay 이후다.
- 지면 맞춤, 개체 간격 보장, Roaming.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [#21](https://github.com/jaykop/Kata/issues/21) | 결과 댓글과 닫기 |
| [Spawner](../manual/Spawner.md) | 현재 사용법 |
| [스포너 컴포넌트 결정 기록](2026-09-30-Spawner-Component-Design.md) | 당시 기록 보존. 삭제한 계획 링크를 이 문서로 바꿨다 |
| 최소 스포너 계획 | 이슈 종료에 따라 삭제. 결정은 이 문서로 옮겼다 |
