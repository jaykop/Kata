# 카메라 사용법

갱신: 2026-09-30  
대상: 플레이어 카메라를 설정하는 사용자. KataCamera 모듈, KataFramework의 `AKataPlayerController`  
적용 기준: [#20](https://github.com/jaykop/Kata/issues/20) CAM-1 카메라 매니저·단계 파이프라인·카메라 데이터·Boom Arm 배치  
확인 상태: 2026-09-29 사용자 Editor 빌드 후 에디터에서 카메라 데이터·매니저 BP·컨트롤러 BP 생성과 저장 확인. 2026-09-30 GameplayDebugger 카테고리 표시 확인. PIE 카메라 동작, 피치 제한, Game 타깃은 미확인

## 목적과 준비

카메라 데이터 에셋 하나로 플레이어 카메라의 피벗, 거리, FOV, 피치 제한을 정한다.
카메라는 `AKataPlayerCameraManager`가 계산하며, `AKataPlayerController`는 이 매니저를 기본으로 쓴다.
설계와 앞으로 추가될 궤도 트랙·Shrink·StateTree·락온은 [카메라 시스템 계획](../plan/Camera-Plan.md)을 따른다.

준비 조건은 다음과 같다.

- 프로젝트에서 KataCamera 플러그인을 켠다. KataFramework가 KataCamera에 의존하므로 KataFramework를 쓰면 함께 켜야 한다.
- 게임 모드의 Player Controller Class가 `AKataPlayerController` 또는 그 파생이어야 한다.
- 카메라가 따라갈 뷰 타깃은 폰이어야 한다.

## 사용 순서

1. 콘텐츠 브라우저에서 Data Asset을 만들고 클래스로 `Kata Camera Data`를 고른다.
2. 에셋의 Placement에서 `Boom Arm`을 고르고 Distance를 정한다. 필요하면 Pivot Offset, Field Of View, Pitch Min·Max를 조정한다.
3. `Kata Player Camera Manager`를 부모로 하는 Blueprint를 만들고 Class Defaults의 Default Camera Data에 위 에셋을 지정한다.
4. `AKataPlayerController`를 부모로 하는 Blueprint를 만들고 Player Camera Manager Class를 3의 Blueprint로 바꾼다.
5. 게임 모드의 Player Controller Class를 4의 Blueprint로 지정하고 PIE를 실행한다.
6. 시점 입력으로 카메라가 피벗 주위를 도는지 확인한다. 게임 중 `'` 키로 GameplayDebugger를 켜고 `KataCamera` 카테고리에서 적용 데이터와 값을 확인한다. 카테고리는 글만 표시하고 월드 도형은 그리지 않는다.

`AKataPlayerController` 자체의 기본 매니저는 C++ 클래스라서 Default Camera Data가 비어 있다. 3~4를 하지 않으면 엔진 기본 카메라가 나온다.
테스트용 예시는 로컬 `Content/KataTest/Camera`에 있다. `Content/`는 저장소에 포함되지 않는다.

## 주요 설정과 실행 규칙

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| Kata Camera Data > Pivot Offset | 뷰 타깃 위치에서 피벗까지의 오프셋. X·Y는 카메라 Yaw 기준(X 앞, Y 오른쪽), Z는 월드 위쪽이다 | (0, 0, 60). 어깨 너머 오프셋은 Y로 준다 |
| Kata Camera Data > Field Of View | 수평 시야각(5~170도) | 90 |
| Kata Camera Data > Pitch Min·Pitch Max | 시점 입력으로 내려보고 올려볼 수 있는 한계(도). 적용 데이터가 바뀔 때 매니저의 View Pitch Min·Max에 반영된다 | -70, 60. Min이 Max보다 크면 데이터 검증에서 오류다 |
| Kata Camera Data > Placement | 카메라를 어디에 둘지 정하는 배치 방식. 현재 `Boom Arm`만 있다 | 비어 있으면 엔진 기본 카메라를 쓰고 경고 로그를 한 번 남긴다. 데이터 검증에서도 오류다 |
| Boom Arm > Distance | 피벗에서 시선 반대 방향으로 떨어진 거리(cm). 충돌 처리는 하지 않는다 | 400 |
| Kata Player Camera Manager > Default Camera Data | 적용할 카메라 데이터 | 비어 있으면 엔진 기본 카메라를 쓴다 |
| Kata Player Camera Manager > Features | 파이프라인 단계에 끼는 기능 목록. 플레이어마다 인스턴스가 따로 생긴다 | 비어 있음. 현재 제공하는 구체 Feature는 없다 |
| `GetActiveCameraData()` | 이번 프레임에 적용하는 카메라 데이터(Blueprint Pure) | 현재는 Default Camera Data를 돌려준다 |

매니저는 매 프레임 컨트롤 회전 → Rotation Feature → 피벗 계산 → Placement → Framing·Constraint·Reaction Feature 순으로 포즈를 만든다.
흔들림 같은 Camera Modifier는 그 뒤에 엔진이 적용한다. 카메라 액터를 뷰 타깃으로 쓰거나 디버그 카메라 스타일을 켜면 엔진 처리가 우선한다.
카메라 상태는 플레이어에 속하므로 빙의나 리스폰으로 폰이 바뀌어도 매니저가 유지되고, 다음 갱신부터 새 폰을 피벗으로 쓴다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 엔진 기본 카메라가 나온다 | 매니저의 Default Camera Data 또는 데이터의 Placement가 비었거나, 컨트롤러가 다른 매니저를 쓴다 | 사용 순서 3~5를 확인한다. 출력 로그의 `LogKataCamera` 경고와 GameplayDebugger 표시를 본다 |
| 카메라가 벽을 뚫는다 | Boom Arm은 충돌을 처리하지 않는다 | 장애물 Shrink(CAM-3) 전까지의 제한이다 |
| 상태별 카메라, 궤도 트랙, 락온 구도가 없다 | CAM-2 이후 작업이다 | [#20](https://github.com/jaykop/Kata/issues/20) 진행을 따른다 |
| Shipping 빌드에 디버그 카테고리가 없다 | `WITH_GAMEPLAY_DEBUGGER`가 꺼진 대상에서는 빠진다 | 의도된 동작이다 |
