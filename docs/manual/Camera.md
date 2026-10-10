# 카메라 사용법

갱신: 2026-10-10  
대상: 플레이어 카메라를 설정하는 사용자. KataCamera 모듈, KataFramework의 `AKataPlayerController`  
적용 기준: [#20](https://github.com/jaykop/Kata/issues/20) CAM-1 매니저·파이프라인, CAM-2 Spline 레일 배치, CAM-3 장애물 Shrink, CAM-4 카메라 StateTree와 블렌드, CAM-5 락온 카메라(빌드·실행 미확인)  
확인 상태: CAM-1은 2026-09-29 카메라 에셋·BP 생성과 저장, 2026-09-30 GameplayDebugger 카테고리 표시를 사용자와 확인했다. CAM-2는 2026-09-30 로컬 샘플 설정·저장 후 사용자가 테스트 완료를 보고했다. C++ 빌드·Game 타깃과 개별 추가 시나리오의 결과는 별도 보고되지 않았다. CAM-4 StateTree·블렌드는 2026-10-02 사용자 빌드와 단일 State 샘플 트리의 PIE `running` 표시까지 확인했다. 여러 State 전환과 블렌드는 확인 전이다. CAM-3 Shrink는 2026-10-02 사용자 빌드, 매니저 Features 추가, PIE 벽·천장 당김과 복귀, GameplayDebugger 표시를 확인했다. 피벗이 막힌 경우와 Game 타깃은 확인 전이다. 2026-10-10에 추가한 뷰 타깃-피벗 선행 스윕과 `Pivot Base` 표시는 사용자 Editor 빌드까지 확인했고 PIE 실행은 확인 전이다

## 목적과 준비

카메라 데이터 에셋으로 플레이어 카메라의 배치, FOV, 피치 제한을 정한다. 고정 거리의 `Boom Arm`과 캐릭터 Blueprint에서 편집하는 `Spline Rail`을 제공한다.
카메라는 `AKataPlayerCameraManager`가 계산하며, `AKataPlayerController`는 이 매니저를 기본으로 쓴다.
Status 태그에 따라 카메라 데이터를 바꾸려면 카메라 StateTree를 쓴다. 앞으로 추가될 락온은 [카메라 시스템 계획](../plan/Camera-Plan.md)을 따른다.

준비 조건은 다음과 같다.

- 프로젝트에서 KataCamera 플러그인을 켠다. KataFramework가 KataCamera에 의존하므로 KataFramework를 쓰면 함께 켜야 한다.
- 게임 모드의 Player Controller Class가 `AKataPlayerController` 또는 그 파생이어야 한다.
- 카메라가 따라갈 뷰 타깃은 폰이어야 한다.

## 사용 순서

1. 콘텐츠 브라우저에서 Data Asset을 만들고 클래스로 `Kata Camera Data`를 고른다.
2. 에셋의 Placement에서 `Boom Arm`을 고르고 Distance와 Pivot Offset을 정한다. 필요하면 Field Of View, Pitch Min·Max를 조정한다.
3. `Kata Player Camera Manager`를 부모로 하는 Blueprint를 만들고 Class Defaults의 Default Camera Data에 위 에셋을 지정한다.
4. `AKataPlayerController`를 부모로 하는 Blueprint를 만들고 Player Camera Manager Class를 3의 Blueprint로 바꾼다.
5. 게임 모드의 Player Controller Class를 4의 Blueprint로 지정하고 PIE를 실행한다.
6. 시점 입력으로 카메라가 피벗 주위를 도는지 확인한다. 게임 중 `'` 키로 GameplayDebugger를 켜고 `KataCamera` 카테고리에서 적용 데이터와 값을 확인한다. 카테고리는 카메라 정보를 텍스트로 표시하고 피벗 위치에 노란 점을 그린다. 오프셋 적용 전 기준인 뷰 타깃 위치에는 흰 점(`Pivot Base`)을 그리고 피벗까지 노란 선으로 잇는다. Spline 배치는 작은 2D 레일 패널도 표시한다.

`AKataPlayerController` 자체의 기본 매니저는 C++ 클래스라서 Default Camera Data가 비어 있다. 3~4를 하지 않으면 엔진 기본 카메라가 나온다.
테스트용 예시는 로컬 `Content/KataSample/Camera`에 있다. `Content/`는 저장소에 포함되지 않는다.

2026-09-30에 저장한 로컬 샘플은 다음 설정을 사용한다. 조준점·FOV 커브는 비워서 레일 이동을 확인하기 쉽게 했다.

| 샘플 | 설정 |
|---|---|
| `BP_SamplePC`의 `KataCameraRail` | 열린 5점 곡선, 원점 Z 60cm, Rail Tag `Camera.Rail.Backview` |
| `DA_KataTestCamera_Default` | Spline Rail, 같은 태그, FOV 85°, Pitch -70°~60°, Fallback Distance 400cm |
| `BP_KataTestCameraManager` | 위 데이터를 Default Camera Data로 사용 |

### Spline 레일 설정

위 매니저·컨트롤러 연결을 유지하고 다음과 같이 배치를 설정한다.

1. 플레이어 캐릭터 Blueprint에 `Kata Camera Rail` 컴포넌트를 추가한다. 컴포넌트 원점을 원하는 피벗 위치에 놓는다. 예를 들어 상대 위치 Z를 60으로 둔다.
2. 프로젝트에 카메라 레일용 GameplayTag를 등록하고 컴포넌트의 Rail Tag에 지정한다. 예: `Camera.Rail.Backview`. 이 태그는 사용자가 등록하며 플러그인이 기본 태그를 추가하지 않는다.
3. Blueprint 뷰포트의 Spline 편집 도구로 곡선을 만든다. `Kata Camera Rail`의 C++ 생성자는 Closed Loop를 false로 지정한다. 기존 Blueprint가 true를 저장했다면 해당 값을 false로 바꾼다. 상대 회전이 0이면 X 앞, Y 오른쪽, Z 위쪽이다. 캐릭터 뒤의 레일은 주로 X가 음수인 위치에 둔다.
4. 카메라 데이터의 Placement를 `Spline Rail`로 바꾸고 Rail Tag에 같은 태그를 지정한다. 현재 뷰 타깃 폰에 그 태그를 가진 레일이 정확히 하나 있어야 한다.
5. Pitch Min·Max를 지정한다. Min 입력은 Spline 시작점, Max 입력은 끝점이다. 예를 들어 내려다보는 시작점을 피벗보다 높게, 올려다보는 끝점을 낮게 놓는다. 범위 중간의 입력은 Spline 길이에 비례한 위치를 사용한다.
6. 필요하면 Aim Offset Curve의 External Curve에 `Curve Vector` 에셋을 지정하고, Field Of View Curve에는 `Curve Float` 에셋을 지정하거나 커브 키를 편집한다. 두 커브의 가로축은 정규화된 Pitch 0~1이다. Aim 값은 cm, FOV 값은 도다. 비어 있으면 조준점 오프셋 0과 데이터의 Field Of View를 사용한다.
7. Fallback Distance를 정한다. 레일 설정에 문제가 있으면 이 거리의 Boom Arm을 사용한다. 출력 로그와 GameplayDebugger의 Rail 진단을 확인한다.

정상 Spline 배치의 피벗은 **컴포넌트 원점**이다. Pivot Offset은 Boom Arm 배치에만 있으며, Spline 실패 시 대체 배치는 오프셋 없이 폰 위치를 피벗으로 쓴다.
Spline 로컬 위치에 컴포넌트의 상대 스케일·회전을 적용하고 카메라 Yaw로 돌려 피벗에 더한다. 캐릭터의 월드 회전은 레일 모양을 돌리는 데 사용하지 않는다.
컴포넌트 원점의 월드 위치는 캐릭터와 함께 이동한다. 레일을 캐릭터 루트 바로 아래에 두면 피벗 배치와 상대 변환을 관리하기 쉽다.

카메라는 Spline 접선 대신 피벗 + Aim Offset을 바라본다. Aim Offset의 X·Y는 카메라 Yaw 기준이고 Z는 월드 위쪽이다.
입력 Pitch가 레일 위치를 정하므로 최종 시선 Pitch는 입력과 다를 수 있다. FOV 커브의 결과는 5~170도로 제한한다.

### GameplayDebugger 카메라 데이터 표시

`KataCamera` 카테고리는 활성 카메라 데이터의 FOV·Pitch 범위, 피벗 래그 설정과 현재 래그 거리, 배치별 값(Boom Arm의 Distance·Pivot Offset, Spline의 태그·Fallback Distance)을 매 수집마다 에셋에서 다시 읽어 표시한다.
PIE 중 Details에서 값을 바꾸면 다음 갱신에 반영된다. 락온 중이거나 해제 블렌드 중이면 상태(active·releasing), 설정 출처(Default Lock On Settings 또는 Lock On Data 이름), 가중치, 좌우 값과 정렬·옆 오프셋·화면 위치·조준선 비율·회전 래그·Blend In 값을 함께 표시한다.
락온 중에는 월드에 피벗에서 락온 초점(빨강)까지의 주황 선과 Look At Alpha 위치의 조준점(청록, `Aim t=`)을 그리고, 피벗–초점·피벗–조준점 거리를 텍스트로 표시한다. Max Look Distance로 잘리면 조준점이 선 위에서 피벗 쪽으로 당겨진다.

### GameplayDebugger 레일 패널

정상 Spline 배치일 때 선택된 태그, Rail Alpha(0~1), Spline 거리·전체 길이와 피벗 기준 XYZ 위치를 표시한다.
300×170 크기의 패널에는 카메라 Yaw 공간의 XZ 투영을 그린다. X는 화면 오른쪽, Z는 위쪽이며 0·1은 레일의 시작·끝이다.
흰 곡선은 레일, 노란 십자는 피벗, 초록 십자는 배치된 카메라, 청록 십자는 조준점이다. Y 위치는 패널 위의 XYZ 표시값으로 확인한다.
피벗과 조준점이 같으면 두 표시가 겹친다. 패널은 Placement 결과를 표시하며 이후 Feature·Camera Modifier 보정은 포함하지 않는다.

이 패널은 Canvas에 그리므로 월드 DebugDraw 표시 여부에 의존하지 않는다. 표시값은 매니저가 마지막으로 계산한 결과다.
Unpossess·디버그 카메라처럼 원래 플레이어 카메라 계산이 멈춘 동안에는 마지막 결과를 보여줄 수 있다. 매니저 조회가 불가능하거나 현재 배치가 Spline이 아니면 패널을 표시하지 않는다.

### 카메라 StateTree로 상태별 데이터 적용

카메라 StateTree는 "어떤 Status일 때 어떤 카메라 데이터를 쓰는가"만 정한다. 위치 계산과 블렌드는 카메라 매니저가 한다. 락온은 이 트리에서 다루지 않는다.

1. StateTree 에셋을 만들고 Schema로 `Kata Camera`를 고른다. 컨텍스트로 `CameraManager`, `Pawn`, `AbilitySystem`이 제공된다.
2. Evaluators에 `Kata Camera Status Tag Watcher`를 추가한다. Watched Tags에 상태를 가르는 Status 태그를 정확한 이름으로 넣고, Reselect Event Tag에 재선택 이벤트 태그(샘플: `StateTree.Event.Camera.Reselect`)를 넣는다.
3. 루트 아래에 상태를 우선순위 순서로 둔다(예: 전투 → 탐색). 루트의 자식 선택은 위에서부터 Enter Condition을 검사하는 방식이어야 한다.
4. 조건이 필요한 상태에 Enter Condition으로 `Kata Camera Has Status Tag`를 넣는다. 마지막 상태(탐색)는 조건 없이 두어 기본값으로 쓴다.
5. 각 상태에 `Kata Camera Apply Data` 태스크를 넣고 Camera Data, Blend Time, Blend Curve, Offset Blend를 정한다.
6. 루트에 Reselect Event Tag 이벤트를 받으면 루트로 가는 전이를 추가한다. 이 전이가 Status 태그가 바뀔 때마다 상태를 다시 고른다.
7. 카메라 매니저 Blueprint의 Camera State Tree에 이 에셋을 지정한다. Default Camera Data는 트리를 쓸 수 없을 때와 첫 요청 전의 대체 데이터로 남긴다.

트리는 폰에 ASC가 있을 때만 시작하고, 이벤트가 없으면 갱신하지 않는다. 폰이나 ASC가 바뀌면 트리를 다시 시작해 구독과 선택을 새로 한다.
첫 요청은 블렌드 없이 적용하고, 이후 전환은 Apply Camera Data의 블렌드 설정을 따른다.

## 주요 설정과 실행 규칙

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| Boom Arm > Pivot Offset | Boom Arm 피벗 오프셋. X·Y는 카메라 Yaw 기준, Z는 월드 위쪽이다 | (0, 0, 60). Spline 배치에는 없다 |
| Kata Camera Data > Field Of View | 수평 시야각(5~170도) | 90 |
| Kata Camera Data > Pitch Min·Pitch Max | 입력 한계(도). 데이터가 바뀔 때 View Pitch Min·Max에 반영한다. Spline은 이 범위를 시작~끝에 대응시킨다 | -70, 60. 유한한 값이어야 하며 Min > Max는 오류다. Spline은 Min < Max가 필요하다 |
| Kata Camera Data > Placement | `Boom Arm` 또는 `Spline Rail` | 비어 있으면 엔진 기본 카메라를 쓰고 경고를 남긴다. 데이터 검증에서도 오류다 |
| Kata Camera Data > Pivot Lag Time Horizontal·Vertical | 피벗이 수평·수직으로 따라잡는 대략의 시간(초) | 0.12·0.05. 폰 위치를 임계 감쇠로 따라가고 그 차이만큼 블렌드된 피벗과 카메라를 옮겨 루트 모션의 짧은 흔들림을 걸러 낸다. Pivot Offset의 시점 회전에 따른 움직임에는 래그를 걸지 않는다. 0이면 즉시 따른다. 활성 카메라 데이터의 값을 쓴다 |
| Kata Camera Data > Pivot Lag Max Distance | 래그로 피벗이 실제 위치에서 떨어질 수 있는 최대 거리(cm) | 150. 0이면 제한하지 않는다. 폰이 바뀌면 래그 없이 다시 붙는다 |
| Boom Arm > Distance | 피벗에서 시선 반대 방향으로 떨어진 거리(cm). 충돌 처리는 하지 않는다 | 400 |
| Kata Camera Rail > Rail Tag / Spline Rail > Rail Tag | 컴포넌트 식별 태그와 배치가 찾는 태그 | 비어 있으면 잘못된 설정이다. 부모·자식 태그는 매칭하지 않는다 |
| Spline Rail > Fallback Distance | 잘못된 레일을 대체할 Boom Arm 거리(cm) | 400. 대체 배치는 폰 위치 피벗과 기본 FOV를 사용한다 |
| Spline Rail > Aim Offset Curve | 정규화된 Pitch에 따른 조준점 오프셋(cm) | 빈 커브는 (0, 0, 0) |
| Spline Rail > Field Of View Curve | 정규화된 Pitch에 따른 수평 FOV(도) | 빈 커브는 Data.FieldOfView |
| Kata Player Camera Manager > Camera State Tree | Status 태그로 카메라 데이터를 고르는 `Kata Camera` 스키마 StateTree | 비어 있으면 Default Camera Data만 쓴다 |
| Status Tag Watcher > Watched Tags·Reselect Event Tag | 추가·제거를 감시할 Status 태그와 그때 보낼 재선택 이벤트 | 태그 개수가 0↔1로 바뀔 때만 보낸다. 부모 태그를 넣으면 자식 태그 변화에도 반응한다 |
| Has Status Tag > Tag·Include Child Tags·Invert | ASC가 태그를 가졌는지 판정 | 자식 포함이 기본값이다(`State.Combat`이 `State.Combat.Aim`을 포함) |
| Apply Camera Data > Blend Time·Blend Curve | 이전 데이터에서 넘어오는 시간(초)과 가중치 곡선 | 0.4, EaseInOut. 곡선은 단조 증가하는 Linear·EaseIn·EaseOut·EaseInOut만 있다 |
| Apply Camera Data > Offset Blend | 궤도 오프셋을 섞는 방법 | Linear(캐릭터 기준 직선 경로). 직선이 피벗을 스치는 전환에만 Direction Slerp를 쓴다 |
| Kata Player Camera Manager > Default Camera Data | 적용할 카메라 데이터 | 비어 있으면 엔진 기본 카메라를 쓴다 |
| Kata Player Camera Manager > Features | 파이프라인의 각 단계에서 적용할 기능 목록. 플레이어마다 인스턴스가 따로 생긴다 | 비어 있음. 현재 `Shrink`를 제공한다 |
| Shrink > Probe Radius·Probe Channel | 스윕하는 구의 반지름(cm)과 충돌 채널. 뷰 타깃 위치에서 피벗까지 먼저 스윕해 피벗이 장애물 안이나 너머에 있으면 시작점을 당기고, 그 지점에서 카메라까지 다시 스윕한다 | 12, `Camera`. 뷰 타깃 폰·컨트롤러·카메라 매니저는 무시한다 |
| Shrink > Min Distance | 당겨도 피벗과 카메라 사이에 남길 거리(cm) | 10 |
| Shrink > Pull In Interp Speed·Recover Interp Speed | 당길 때와 복귀할 때의 보간 속도. 0이면 즉시 | 0(즉시 당김), 4. 당김에 속도를 주면 그동안 장애물 너머가 보일 수 있다 |
| `GetActiveCameraData()` | 이번 프레임에 적용하는 카메라 데이터(Blueprint Pure) | 현재는 Default Camera Data를 돌려준다 |

매니저는 매 프레임 컨트롤 회전 → Rotation Feature → 피벗 계산 → Placement → Framing·Constraint·Reaction Feature 순으로 포즈를 만든다.
흔들림 같은 Camera Modifier는 그 뒤에 엔진이 적용한다. 카메라 액터를 뷰 타깃으로 쓰거나 디버그 카메라 스타일을 켜면 엔진 처리가 우선한다.
카메라 상태는 플레이어에 속하므로 빙의나 리스폰으로 폰이 바뀌어도 매니저가 유지되고, 다음 갱신부터 새 폰을 피벗으로 쓴다.

## 락온 카메라

`AKataPlayerController`가 폰의 락온 변경을 받아 `SetLockOnFocus`로 카메라에 전달한다.
회전·구도 Feature는 매니저의 기본 서브오브젝트이므로 Features 배열에 따로 추가하지 않는다.
기존 Boom Arm·Spline Rail 배치를 계속 사용하며, 락온 동안 수동 시점 입력은 무시한다. 이동·공격 입력은 유지한다.

1. 플레이어 컨트롤러와 카메라 매니저를 Kata 파생으로 설정하고 유효한 Default Camera Data·Placement를 지정한다.
2. [입력 사용법](Input.md#락온-입력)에 따라 세 입력을 연결하고, 대상에 활성 `TargetPoint.LockOn` 지점을 둔다.
3. 기본값은 Camera Manager → Kata|Camera|Lock On의 Default Lock On Settings에서 조정한다.
4. 부위별로 바꾸려면 `Kata Lock On Data` 에셋을 만들어 아래 설정 전체를 정한다. 대상의 Kata Target Point에서 Lock On Camera Data 체크박스를 켜고 에셋을 지정한다.
   체크박스가 꺼졌거나 에셋이 비면 매니저 기본값을 쓰고, 켜져 있으면 그 에셋의 설정을 그대로 쓴다. 두 설정을 항목별로 섞지 않는다.

| 설정 | 의미·단위 | 실행 계약 |
|---|---|---|
| Alignment | 플레이어 기준 타겟을 둘 쪽. Right, Left, Auto(기본) | Right면 카메라가 플레이어→타겟 선의 오른쪽으로 비켜 타겟이 플레이어 오른쪽에 보인다 |
| Side Offset | 플레이어→타겟 선에서 카메라가 비키는 최종 거리(cm). 기본 60 | 피벗 오프셋·레일이 이미 비킨 거리를 보정해 이 값에 맞춘다 |
| Auto Switch Angle | Auto 전환 임계 각도(도). 기본 30. Alignment가 Auto일 때만 보인다 | 아래 Auto 판정을 따른다 |
| Target Screen Position | 조준점을 둘 화면 좌표. 왼쪽 위 (0, 0), 오른쪽 아래 (1, 1). 기본 (0.5, 0.4) | 카메라 회전만 보정한다. X는 Right 기준이며 Left에서는 좌우를 뒤집는다. 각 축 0.05~0.95 |
| Look At Alpha | 조준점 = Lerp(피벗, 락온 지점, 값). 기본 1 | 1이면 타겟, 0.5면 플레이어와 타겟의 중간을 화면 위치에 둔다 |
| Max Look Distance | 피벗에서 조준점까지의 상한(cm). 0이면 무제한. 실제 t가 1보다 작을 때만 쓴다 | 타겟이 멀어져도 플레이어가 화면 밖으로 밀리지 않게 한다 |
| Rotation Lag Time | 블렌드가 끝난 뒤 지점 방향을 따라잡는 대략의 시간(초). 기본 0.15 | 임계 감쇠로 따라가며 0이면 즉시 따른다. 블렌드 중에는 쓰지 않는다 |
| Pitch Offset By Distance | 플레이어–락온 지점 거리(cm)에 따른 Pitch 오프셋(도) 곡선. 양수면 카메라가 올라가 내려다본다 | 비면 0. 락온 회전의 목표 Pitch에 더하고 카메라 데이터 Pitch 범위로 제한한다. 타겟 변경 중에는 이전·새 값을 섞는다 |
| Look At Alpha By Distance | 같은 거리에 따른 조준점 비율 t(0~1) 곡선 | 데이터가 있으면 Look At Alpha 대신 쓴다. 디버거의 `Aim t=`가 실제 값이다 |
| Boom Distance Scale By Distance | 같은 거리에 따른 Boom Arm 거리 배율 곡선 | 비면 1. Boom Arm에만 적용하며 Spline에는 쓰지 않는다. 락온 가중치만큼 적용한다 |
| Blend In Duration | 획득·타겟 변경·좌우 전환 시간(초). 기본 0.5 | 해제도 같은 시간 동안 Ease In-Out으로 블렌드한다. 0이면 즉시 |
| Blend In Curve | 0~1 진행도를 가중치로 바꾸는 Float Curve 에셋. 비면 Ease In-Out | 단조 증가해야 한다. 데이터 검증이 비단조이거나 0→0, 1→1이 아닌 곡선을 경고한다. 해제에는 쓰지 않는다 |
| Lock On Data → Camera Data | 락온 중 사용할 배치 데이터 | 비면 StateTree가 고른 배치를 유지한다. 교체 블렌드도 Blend In 시간·곡선을 쓴다 |

거리와 Pitch 범위는 락온 설정에 없다. 락온 중 다르게 하려면 Lock On Data의 Camera Data로 배치 데이터를 교체한다. 락온 회전의 Pitch는 활성 카메라 데이터의 Pitch 범위로 제한한다.

### 블렌드

- 획득: 획득 순간의 시점에서 타겟 방향으로 회전하고, 좌우 오프셋·화면 위치 보정이 0에서 1로 들어온다. 모두 같은 Blend In 시간과 곡선을 따른다.
- 타겟 변경: 보정 강도는 유지한 채 초점·회전·구도 값·좌우가 이전 값에서 새 타겟으로 같은 시간과 곡선으로 이동한다. 마커는 새 지점을 즉시 표시한다.
- 해제: 보정이 같은 시간 동안 Ease In-Out으로 빠지고, 시점 입력은 마지막 락온 회전에서 이어진다. 해제 중 보정은 시점에 붙은 오프셋으로 줄어들므로 마우스로 시점을 돌려도 화면이 출렁이지 않는다.
- 블렌드 중에 다시 획득·변경·해제하면 진행 중인 현재 값에서 이어서 시작한다.
- 별도 Camera Data를 지정한 락온 중에도 StateTree는 계속 최신 상태를 고르고, 해제하면 그 데이터로 블렌드해 돌아간다.

### Auto 정렬 판정

- 획득·타겟 변경 시 현재 시점 기준으로 타겟이 오른쪽에 있으면 Right, 왼쪽이면 Left를 고른다.
- 블렌드가 끝난 뒤에는 카메라 Yaw와 플레이어→타겟 Yaw의 차이가 반대쪽으로 Auto Switch Angle을 넘으면 좌우를 바꾼다.
- 카메라는 Rotation Lag Time만큼 늦게 타겟을 따라가므로, 전환은 타겟이 플레이어 가까이를 빠르게 가로지르는 등 시선이 임계 각도 이상 뒤처질 때 일어난다. Rotation Lag Time이 0이면 락온 중에는 전환하지 않는다.

컨트롤 회전에는 최종 시선이 아니라 궤도 회전을 유지한다. Spline의 최종 시선 Pitch를 레일 입력에 다시 넣어 해제 순간 위치가 바뀌는 일을 피하기 위해서다.

Shrink는 구도 보정 뒤 위치만 당긴다. 장애물에 당겨진 동안에는 지정한 화면 위치와 실제 대상 위치가 어긋날 수 있다.
화면 좌표 계산은 일반 원근 뷰포트를 기준으로 하며, 강제 종횡비·레터박스 설정과 직교 카메라는 이번 범위에서 보장하지 않는다.
2026-10-05 재작성한 CAM-5는 사용자가 DebugGame 에디터 PIE에서 블렌드, 해제 중 시점 입력, 피벗 래그, 타겟 주위 회전, 디버거 조준선과 거리 곡선 적용을 확인했다. Game 타깃과 Spline 배치에서의 락온은 확인 전이다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 엔진 기본 카메라가 나온다 | 매니저의 Default Camera Data 또는 데이터의 Placement가 비었거나, 컨트롤러가 다른 매니저를 쓴다 | 사용 순서 3~5를 확인한다. 출력 로그의 `LogKataCamera` 경고와 GameplayDebugger 표시를 본다 |
| Spline을 선택했는데 Boom Arm으로 표시된다 | 태그 누락·불일치·중복, 닫힌 Spline, 길이 없음, 잘못된 Pitch 범위 또는 유한하지 않은 평가값 | Rail 진단과 `LogKataCamera`를 확인한다. 오류 대상·태그·사유가 바뀔 때 경고하며 임의의 첫 레일은 선택하지 않는다 |
| 캐릭터 Blueprint의 기본 Spline을 찾지 못한다 | 기본 `Spline`의 Component Tags는 FName이다 | `Kata Camera Rail` 컴포넌트의 Rail Tag를 사용한다 |
| 2D 패널에서 레일 일부가 겹친다 | XZ 투영이므로 Y 방향 변화는 겹쳐 보일 수 있다 | 현재 Y 값을 텍스트로 확인하고 전체 형태는 캐릭터 Blueprint에서 편집한다 |
| 카메라가 벽을 뚫는다 | 배치는 충돌을 처리하지 않으며 매니저 Features에 `Shrink`가 없다 | 카메라 매니저 BP의 Features에 `Shrink`를 추가한다. 채널을 바꿨다면 장애물이 그 채널을 Block하는지 확인한다 |
| 카메라가 캐릭터 몸 근처로 붙는다 | 피벗(Spline Rail은 레일 원점)이 지형이나 다른 물체 안에 있어 스윕이 시작부터 막혔다 | 피벗 위치를 확인한다. 이 경우 Min Distance까지 당겨지는 것이 의도된 동작이다 |
| 상태가 바뀌어도 카메라 데이터가 그대로다 | 트리가 실행되지 않았거나(폰에 ASC 없음, 스키마 불일치), 감시 태그·재선택 전이·Enter Condition 순서가 맞지 않는다 | GameplayDebugger의 State Tree 줄과 Layer 목록, `LogKataCamera` 경고를 확인한다 |
| 블렌드 중 카메라가 잠깐 멈칫한다 | 블렌드가 끝나기 전에 전환이 네 번 이상 겹쳐 가장 아래의 두 레이어가 고정된 결과로 합쳐졌다 | 디버거의 frozen 레이어를 확인한다. 합친 순간의 출력은 바뀌지 않으며 고정 레이어는 매 프레임 다시 계산하지 않는다 |
| Rewind Debugger에 카메라 StateTree가 보이지 않는다 | 트리는 시작과 재선택 때만 이벤트를 내고 평소에는 갱신하지 않는다. PIE 시작 뒤에 녹화를 켜면 기록할 이벤트가 없다(추정) | PIE 시작 전에 녹화를 켜거나 Status 태그를 바꿔 재선택을 일으킨다. 트리 소유자는 카메라 매니저이므로 그 액터 아래에서 찾는다 |
| 락온 구도가 없다 | 타게팅 획득 실패, 다른 컨트롤러·매니저 또는 배치 데이터 누락 | 위 락온 설정과 타게팅 디버거를 확인한다. 새 코드 빌드 후 PIE를 다시 시작한다 |
| Unpossess 후 피벗 점이 보이지 않는다 | GameplayDebugger 월드 DebugDraw가 표시되지 않는 알려진 현상이다 | 텍스트 정보와 Canvas 레일 패널을 활용한다. 관전 모드에서의 새 패널 동작은 사용자 확인 전이다 |
| Shipping 빌드에 디버그 카테고리가 없다 | `WITH_GAMEPLAY_DEBUGGER`가 꺼진 대상에서는 빠진다 | 의도된 동작이다 |

## 확인 상태와 근거

로컬 샘플은 엔진의 `SetSplinePoints` API로 설정했고 Blueprint 생성 클래스 갱신 후 세 에셋을 저장했다. 2026-09-30 사용자가 이 설정의 테스트 완료를 보고했다. 에이전트는 C++ 빌드·테스트·별도 검사를 실행하지 않았다.
Pitch·Yaw·GameplayDebugger의 세부 관찰 결과, 조준점·FOV 커브, 태그 누락·중복의 대체 배치, 폰 교체와 Unpossess·관전 모드의 개별 결과는 별도 보고되지 않았다. 샘플 테스트 완료를 모든 경계 조건이나 Game 타깃의 확인으로 확대하지 않는다.

- [레일 컴포넌트](../../Plugins/KataCamera/Source/KataCamera/Public/KataCameraRailComponent.h): RailTag와 로컬 위치 변환.
- [배치 구현](../../Plugins/KataCamera/Source/KataCamera/Private/KataCameraPlacement.cpp): Pitch 정규화와 조준점·FOV 평가.
- [Spline 레일 결정 기록](../devlog/2026-09-30-Camera-Spline-Rail.md).
- [작업 상태](https://github.com/jaykop/Kata/issues).
