# 스포너 생성 방식을 Activation 하나로 통합

작성: 2026-10-07  
갱신: 2026-10-09  
유형: 구현 기록·결정 기록  
대상: KataFramework 스포너  
기준: `2cd4c15` 이후 미커밋 작업 트리. 다른 작업자의 Config·AI 문서 변경은 포함하지 않음

## 배경과 결론

[거리 활성화](2026-10-07-Spawner-Distance-Activation.md)는 사용자 요청대로 스포너 설정 항목(`Distance Activation`)으로 구현했다. 그 뒤 사용자가 설정 항목의 역할 분리와 이름을 진단해 달라고 요청했다. 진단해 보니 다른 항목은 생성 과정의 한 단계를 보정하는데, `Distance Activation`만 스포너 전체의 실행 방식을 바꾸는 모드였다. 그 결과 액터의 `bSpawnOnBeginPlay`·`bUseTimeSlicing`을 말없이 무시했다.

사용자가 생성 방식을 액터의 선택지 하나로 모으는 안(B)과 이름 `Player Distance`를 승인했다. 스포너에 `Activation`(Begin Play·Manual·Player Distance)과 `SpawnDistance`·`DespawnDistance`를 두고, 기존 설정은 로드 때 자동으로 옮긴다. 함께 논의한 대로 거리 범위는 스포너를 선택했을 때만 보이게 했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 생성 시작 방식 | `bSpawnOnBeginPlay`와 `Distance Activation` 항목에 나뉨 | `Activation` 하나. Player Distance일 때만 거리 값 표시 |
| 시간 분산 옵션 | Player Distance에서도 보였지만 무시됨 | Player Distance에서 숨김 |
| 이전 `bSpawnOnBeginPlay=false` | 직접 호출 방식 | 로드 시 `Manual`로 이전 |
| 이전 `Distance Activation` 항목 | 거리 설정 담당 | 로드 시 첫 활성 항목의 값을 옮기고 배열에서 제거. 클래스는 읽기용으로 숨김 |
| 거리 미리보기 | 없음 | 선택 시에만 SpawnDistance 초록·DespawnDistance 빨강 와이어 구 |
| Spawn Area 미리보기 | 항상 표시 | 유지 |

## 주요 결정과 이유

- 생성 시점의 선택을 한 곳에 모아, 무시되는 설정이 Details에 남지 않게 했다. 컴포넌트가 아니므로 항목 이름 문제도 함께 해소된다.
- 기존 값은 `bSpawnOnBeginPlay_DEPRECATED`로 읽는다. 엔진은 `_DEPRECATED`를 C++ 이름에만 붙이고 저장 이름은 유지하므로(`FProperty::GetNameCPP`) Redirect 없이 이전 값을 읽는다. 기본값 true는 저장되지 않으므로 false일 때만 Manual로 옮긴다.
- `Distance Activation` 클래스를 바로 지우면 기존 레벨의 항목이 클래스를 찾지 못한다. 그래서 `HideDropdown`으로 새로 추가할 수 없게 하고 PostLoad에서 옮긴다. 이 레벨을 모두 다시 저장한 뒤 클래스를 지울 수 있다.
- 거리 범위는 수십 m라 항상 그리면 여러 스포너의 구가 겹쳐 영역 미리보기를 가린다. 엔진 `UShapeComponent::bDrawOnlyIfSelected`로 선택 시에만 그리며, 별도 Editor 모듈 코드는 추가하지 않았다. 판정이 3D 거리이므로 수평 원 대신 구로 그리고, 액터 스케일과 무관한 절대 반지름을 쓴다.
- Spawn Area 미리보기는 배치 확인에 필요하고 크기가 작아 항상 표시를 유지했다.

## 후속 변경 (2026-10-09)

사용자가 레벨에 배치한 스포너를 끄는 옵션이 없다고 지적했다. Manual도 자동 생성은 하지 않지만 Blueprint 호출로 생성되고 이름만으로 꺼짐이 드러나지 않는다. 별도 `Enabled` 체크박스는 생성 시점 설정을 다시 두 곳으로 나누므로, 사용자 승인에 따라 `Activation`에 `Disabled`를 추가했다. Disabled는 BeginPlay에서 아무것도 하지 않고 수동 생성·취소·제거 호출을 거절하며, Details에서 거리 값과 Use Time Slicing을 숨긴다. 열거형 값은 이름으로 저장되므로 끝에 추가해도 기존 레벨에 영향이 없다. 같은 날 사용자가 빌드 후 잘 동작한다고 보고했다.

## 근거

- [스포너](../../Plugins/KataFramework/Source/KataFramework/Private/Spawning/KataCharacterSpawner.cpp): `PostLoad` 이전, `BeginPlay` 분기, `InitializeDistanceManagement`, `UpdateDistancePreview`.
- [이전 항목](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataSpawnerComponent_DistanceActivation.h): 읽기 전용으로 남긴 클래스.
- 엔진 `Property.cpp`의 `FProperty::GetNameCPP`, `ShapeComponent.h`의 `bDrawOnlyIfSelected`와 `SphereComponent.cpp`의 선택 판정.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 설계 선택 | 사용자 대화 | B안·이름 Player Distance 승인 | 없음 |
| 이전 값 읽기 방식 | 엔진 소스 읽기 | `_DEPRECATED`는 C++ 이름에만 적용 | 실제 레벨 로드 결과 |
| 구현 | 에이전트 소스 작성 | 열거형·거리 속성·이전·미리보기 | 없음 |
| 실행 | 사용자 빌드·확인 보고 (2026-10-07) | 잘 동작함 | 레벨 재저장 후 이전 결과, 개별 항목 |

에이전트는 빌드·테스트·별도 검사를 실행하지 않았다.

## 남은 제한과 후속 작업

- 같은 날 사용자가 `LV_TestMap`을 다시 저장했다. 저장 파일에 이전 항목 참조가 없고 Player Distance 값이 들어간 것을 확인한 뒤 `UKataSpawnerComponent_DistanceActivation`과 PostLoad의 항목 이전 코드를 제거했다. `bSpawnOnBeginPlay` 이전은 다른 레벨을 위해 유지한다.
- PostLoad 이전은 레벨을 수정 상태로 만들지 않으므로, 액터를 조금 수정한 뒤 저장해야 파일에 반영된다.
- Blueprint에서 `bSpawnOnBeginPlay`를 읽던 노드가 있으면 `Activation`으로 바꿔야 한다. 현재 소스에는 사용처가 없다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| 연결 이슈 | 미게시 |
| [스포너 사용법](../manual/Spawner.md) | Activation·거리 설정, 자동 이전, 선택 시 미리보기 |
| [전체 계획](../plan/Spawner-Scheduling-And-Grid-Plan.md) | S4 항목 이름을 Player Distance로 갱신 |
| [문서 목록](../README.md) | 기록 링크 추가 |
