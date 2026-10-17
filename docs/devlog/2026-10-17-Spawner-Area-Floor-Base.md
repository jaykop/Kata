# 스포너 영역을 바닥 기준으로 재정의

작성: 2026-10-17  
갱신: 2026-10-17  
유형: 결정 기록  
대상: KataFramework Spawn Area(`UKataSpawnerComponent_SpawnArea`), 스포너 미리보기, Nav Mesh Projection  
기준: 2026-10-17 작업 트리. 같은 스포너 파일에 있던 다른 작업의 미커밋 변경은 포함하지 않는다.

## 배경과 결론

[스포너 기준점을 발 높이로 바꾼 뒤](2026-10-17-Spawner-Feet-Origin.md) 영역 원점이 바닥에 오면서, 중심 기준인 상자와 구 영역이 바닥 아래로 절반씩 내려가 보였다. 구는 부피 전체에서 후보를 고르므로 땅 아래 후보도 나왔다.
사용자와 상의해 영역을 바닥면 기준으로 다시 정의했다. 상자와 원기둥 모두 영역 원점이 바닥면 중심이고 위로만 쌓인다. 후보 위치, 미리보기 메시, 실제 생성 위치는 모두 바닥면을 기준으로 한다. 구 영역은 원기둥으로 바꿨다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 영역 모양 | Sphere, Box | Cylinder, Box. 열거형 순서와 기본값(첫 값)은 그대로다 |
| Box | 원점이 상자 중심. 부피 안에서 후보를 고름 | 원점이 바닥면 중심. 바닥면에서 X·Y만 고르고 Z Extent는 위로 쌓이는 절반 높이다 |
| 구·원기둥 | 구 부피 안에서 균일하게 후보를 고름 | 바닥 원판 안에서 균일하게 고른다. `CylinderRadius`·`CylinderHalfHeight` |
| 미리보기 | 엔진 구·상자 컴포넌트를 중심 기준으로 그림 | 상자는 Z Extent만큼 올려 그리고, 원기둥은 새 `UKataWireCylinderComponent`가 바닥면부터 그린다 |
| Nav Mesh Projection | 질의 위치는 후보 그대로, 범위는 영역의 가장 긴 크기의 정육면체 | 질의 위치를 영역 높이의 절반만큼 올리고, 범위는 수평 반경과 절반 높이로 나눈다. 기본 범위 최소값은 유지한다 |
| 저장된 데이터 | - | 플러그인 `Config/DefaultKataFramework.ini`의 CoreRedirects가 `Sphere`→`Cylinder`, `SphereRadius`→`CylinderRadius`로 읽는다 |

## 주요 결정과 이유

- 후보는 항상 바닥면에서 고른다. 생성 위치는 캐릭터의 발 위치이므로 부피 안의 높이를 고를 이유가 없고, 높은 곳에서 고르면 떠서 생성된다.
- 높이 값은 엔진 Box·Capsule 컴포넌트처럼 절반 높이로 둔다. 높이는 후보에 영향을 주지 않고, 미리보기와 NavMesh 투영이 영역 안의 계단·경사 위 NavMesh를 찾는 범위로만 쓴다.
- 열거형 값 순서를 유지했다. 기본값 그대로라 직렬화되지 않은 인스턴스가 Box로 바뀌지 않게 하려는 것이다.
- 엔진에는 원기둥 형상 컴포넌트가 없다. Capsule로 그리면 위아래가 둥글어 실제 영역과 모양이 달라지므로, 엔진 `DrawWireCylinder`로 선을 그리는 표시 전용 컴포넌트를 KataFramework에 두었다. 선택했을 때만 그리는 Component Visualizer는 상자 미리보기와 동작이 달라 쓰지 않았다(사용자 확정).
- CoreRedirects는 프로젝트가 아니라 플러그인 설정에 두었다. 옛 값을 가진 데이터는 이 플러그인 타입의 것이므로, 플러그인을 쓰는 다른 프로젝트도 같은 이전을 받는다.

## 근거

- [KataSpawnerComponent_SpawnArea.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataSpawnerComponent_SpawnArea.cpp): `GetSpawnTransform_Implementation`의 바닥면 표본 추출, `GetAreaHalfHeight`·`GetAreaHorizontalRadius`.
- [KataWireCylinderComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataWireCylinderComponent.cpp): 바닥면 기준 원기둥 프록시와 경계.
- [KataCharacterSpawner_Preview.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataCharacterSpawner_Preview.cpp): `UpdateSpawnAreaPreview`.
- [KataSpawnerComponent_NavMeshProjection.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataSpawnerComponent_NavMeshProjection.cpp): 질의 위치와 범위.
- [DefaultKataFramework.ini](../../Plugins/KataFramework/Config/DefaultKataFramework.ini): CoreRedirects.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 소스 | 작성자 소스 읽기 | 구현과 문서 대조 | - |
| Editor 빌드 | 2026-10-17 사용자 보고 | 첫 빌드에서 C4458(프록시 지역 변수 `LocalToWorld`가 엔진 멤버를 가림)이 나와 이름을 바꾼 뒤 통과 | Game 빌드 |
| 저장된 데이터 | MCP로 다시 읽음 | LV_FeatureGallery의 이전 Sphere 스포너 3개가 `Cylinder`, 반지름 200, 절반 높이 0으로 읽힘 | LV_CombatSandbox |
| 미리보기·생성 | 2026-10-17 사용자 PIE 보고 | 원기둥·상자가 바닥면부터 위로 그려지고 높이 변경이 반영됨. 캐릭터가 영역 바닥에 서서 생성되고 경사·계단·턱 위 더미도 생성됨 | LV_CombatSandbox |

## 남은 제한과 후속 작업

- 원기둥 미리보기는 영역 스케일의 X·Y 중 큰 값으로 원을 그린다. X·Y 스케일이 다르면 실제 후보 영역(타원)과 모양이 다르다.
- 이전 Box에서 Z Extent가 0보다 컸던 설정은 영역이 위로 옮겨 그려진다. 샘플 맵은 모두 0으로 옮겨 두었다.
- LV_CombatSandbox의 스포너 3개는 같은 날 바닥 기준으로 옮겼다([발 기준 기록](2026-10-17-Spawner-Feet-Origin.md#기존-배치-처리)). 이 맵은 PIE로 생성 위치를 확인하지 않았다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#128](https://github.com/jaykop/Kata/issues/128) | 구현·확인 상태 |
| [스포너 사용법](../manual/Spawner.md) | 영역 모양·기준점·미리보기, 높이의 의미, 이전 데이터 처리, NavMesh 탐색 범위 |
