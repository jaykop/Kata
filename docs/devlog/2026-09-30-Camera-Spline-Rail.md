# 카메라 Spline 레일 구현

작성: 2026-09-30  
갱신: 2026-10-03  
유형: 구현·결정 기록  
대상: KataCamera, [#20](https://github.com/jaykop/Kata/issues/20) CAM-2  
기준: 2026-09-30 CAM-2 소스·문서와 로컬 샘플 설정. 같은 날 사용자 샘플 테스트 완료 보고

## 배경과 결론

사용자는 플레이어 캐릭터 Blueprint에 Spline 컴포넌트를 붙여 카메라 레일을 편집하고,
CameraData의 GameplayTag로 사용할 레일을 선택하기로 했다. Spline 자체로 피벗 오프셋을 표현하므로 컴포넌트 원점을 피벗으로 정했다.
이 결정을 바탕으로 `UKataCameraRailComponent`와 `UKataCameraPlacement_Spline`을 구현했다.

카메라 연결 선이 시야를 가렸고, Unpossess 후 GameplayDebugger의 월드 DebugDraw가 표시되지 않는 기존 현상이 있었다.
피벗 점은 사용자 요청대로 유지하고, 레일 상태는 Canvas의 작은 2D 패널로 표시한다. 관전 모드의 새 패널 동작은 아직 사용자 확인 전이다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 레일 저작 | 고정 거리 Boom Arm만 제공 | 캐릭터 Blueprint에서 편집하는 태그 지정 Spline 컴포넌트 |
| 배치 | 컨트롤 회전의 반대 방향으로 고정 거리 | Pitch를 레일 시작~끝에 대응하고 카메라 Yaw로 궤도 회전 |
| 프로필 | 데이터의 고정 FOV | 정규화된 Pitch에 따른 조준점·FOV 커브. 빈 커브는 기본값 사용 |
| 오류 처리 | Placement 누락 시 엔진 기본 계산 | 잘못된 Spline은 경고와 설정 거리의 Boom Arm으로 대체 |
| 디버그 | 글과 피벗 점 | 태그·입력·거리·위치 진단과 XZ 투영 레일 패널 추가 |
| 평가 결과 | 최종 위치·회전 | FOV, 정규화 입력, 피벗 기준 위치·조준점 오프셋도 제공 |

## 주요 결정과 이유

사용자가 확정한 설계는 [카메라 시스템 계획](../plan/Camera-Plan.md)의 CAM-2 관련 결정에 기록했다.

- `ComponentTags`는 FName이므로 GameplayTag를 가진 Spline 파생 컴포넌트를 제공한다. 기존 Spline 편집 도구를 이용하며 별도 에디터 모듈을 추가하지 않는다.
- 정상 Spline은 컴포넌트 원점을 피벗으로 쓴다. Data.PivotOffset을 다시 더하면 같은 오프셋이 중복되므로 Boom Arm과 오류 대체 경로에만 적용한다.
- 로컬 곡선에 컴포넌트의 상대 스케일·회전을 적용한 뒤 카메라 Yaw로 회전한다. 캐릭터의 몸 회전과 시점 궤도 방향을 분리한다.
- 열린 Spline의 길이를 입력 Pitch에 대응한다. Pitch는 최종 시선 각도가 아니라 레일 위치 매개변수다. 시선은 피벗과 조준점 오프셋으로 계산하며 접선을 사용하지 않는다.
- 레일 컴포넌트의 C++ 생성자에서 Closed Loop의 기본값을 false로 명시한다. 기존 Blueprint가 저장한 true 값은 생성자 변경으로 자동 해제되지 않으므로 별도로 바꾼다.
- 조준점·FOV 커브는 CameraData가 소유한 Spline Placement의 설정이다. 레일 참조·진단은 매니저에 두고 프레임별 결과만 Context로 전달한다. 공유 에셋에 실행 상태를 저장하지 않는다.
- 태그는 정확히 일치해야 하며 중복되면 어느 것도 선택하지 않는다. 매 갱신 현재 폰의 컴포넌트를 검색해 폰 교체·컴포넌트 제거·태그 변경을 반영한다. 경고는 폰·배치·태그·실패 사유가 달라졌을 때 남긴다.
- 레일 누락·중복·닫힘·길이 없음·잘못된 Pitch 범위·유한하지 않은 평가값은 Boom Arm으로 대체한다. 오류 경로도 기본 FOV와 기존 피벗 규칙을 유지한다.
- GameplayTags는 공개 레일·배치 타입의 FGameplayTag 때문에 Public 의존성으로 추가한다. 코어 Kata·GAS·StateTree 의존은 이번 단계에서 추가하지 않는다.

## 근거

- [레일 컴포넌트](../../Plugins/KataCamera/Source/KataCamera/Public/KataCameraRailComponent.h): RailTag, 피벗 기준 궤도 위치 변환.
- [배치 구현](../../Plugins/KataCamera/Source/KataCamera/Private/KataCameraPlacement.cpp): Pitch 정규화, 조준점·FOV, Boom Arm 대체.
- [카메라 매니저](../../Plugins/KataCamera/Source/KataCamera/Private/KataPlayerCameraManager.cpp): 현재 폰의 레일 검색, 진단과 값 스냅숏.
- [GameplayDebugger](../../Plugins/KataCamera/Source/KataCamera/Private/Debug/GameplayDebuggerCategory_KataCamera.cpp): Canvas 레일 패널과 피벗 점.

## 확인 범위와 결과

| 대상 | 수행 내용 | 결과 | 미확인 범위 |
|---|---|---|---|
| CAM-2 | 확정 설계를 소스·문서에 반영. 설정 후 사용자가 샘플 테스트 완료 보고 | 구현과 샘플 테스트 완료 | C++ 빌드·Game 타깃, 커브·오류 처리·폰 교체 등 개별 시나리오의 결과는 별도 보고되지 않음 |
| 로컬 샘플 설정 | 에디터 재실행 후 `SetSplinePoints`로 레일 설정, Blueprint 생성 클래스 갱신과 에셋 저장 | 열린 5점 레일, 원점 Z 60cm, `Camera.Rail.Backview` 태그. Spline Rail·FOV 85°·Pitch -70°~60°·대체 거리 400cm. 캐릭터 BP·데이터·매니저 BP 저장 완료 | 수동 Spline 편집의 개별 결과는 별도 보고되지 않음 |
| GameplayDebugger | DrawData의 Canvas 패널 구현 | 월드 도형과 독립된 표시 경로 작성 | 실제 패널 표시, 크기·가독성, Unpossess·관전 모드에서 매니저 조회와 결과 갱신 |

2026-09-30 사용자가 샘플 설정 후 테스트 완료를 보고했다. 개별 관찰 결과나 빌드 로그는 별도로 제공하지 않았다.
에이전트는 빌드·테스트·자동 문서 검사·별도 코드 검사를 사용자 담당 원칙에 따라 실행하지 않았다.
과거 CAM-1의 사용자 확인 결과를 이번 CAM-2의 검증 결과로 사용하지 않는다.

## 남은 제한과 후속 작업

두 배치 모두 충돌을 처리하지 않으므로 다음 단계는 CAM-3 장애물 Shrink다.
CAM-4 블렌딩 구현은 아직 없으며 궤도 공간 결과는 이후 설계를 위한 입력으로 제공한다.
XZ 투영은 Y 방향 변화를 평면에서 구분하지 못하므로 현재 Y 값을 글로 표시하고 전체 형태는 Blueprint에서 편집한다.
패널은 Placement 결과이며 이후 Feature·Modifier 보정은 포함하지 않는다. 플레이어 카메라 계산이 멈추면 표시값은 마지막 결과다.

샘플 에셋 설정 중 SplineCurves의 하위 배열을 차례로 수정하자 에디터 재구성이 중간 상태를 읽었다.
사용자가 보낸 디버거 화면에서 위치·회전·스케일 곡선의 점 개수가 같아야 한다는 `FSplineCurves::UpdateSpline`의 check 중단을 확인했다.
레일 점 설정은 내부 배열의 개별 변경 대신 `SetSplinePoints`처럼 세 곡선을 함께 구성하는 엔진 API를 사용해야 한다.
사용자가 에디터를 재실행한 뒤 `SetSplinePoints`로 설정을 다시 수행했다. 위치·회전·스케일의 점 개수가 모두 5개인 설정을 읽어 확인했고 샘플 레일·데이터·매니저를 저장했다. 조준점·FOV 커브는 비워 초기 레일 이동 확인에 집중할 수 있게 했다. 이후 사용자가 샘플 테스트 완료를 보고했다. 엔진의 check나 소스는 수정하지 않았다.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [폐지 전 상태 기록](../localdocs/Implementation-Status-Archive-2026-10-03.md) | CAM-2 범위, GameplayTags 의존, 사용자 샘플 테스트 보고와 남은 확인 범위 |
| [카메라 사용법](../manual/Camera.md) | BP 레일·태그·커브 설정, 피벗·좌표 규칙, 오류 진단과 2D 패널 |
| [카메라 시스템 계획](../plan/Camera-Plan.md) | 현재 제공 범위·평가 계약과 이 기록 연결. 후속 설계 유지 |
| [문서 목록](../README.md) | 이 기록 추가, 카메라 설명서 소개 갱신 |

새 구현 범위는 #20에 대응한다. CAM-3 이후가 남아 있으므로 전체 카메라 이슈와 계획 문서는 유지한다. 공개 이슈 결과 게시·체크리스트·라벨 변경은 별도 게시 승인 전이며 이번 작업에서 수행하지 않았다.


> 2026-10-03 이후 전체 상태 요약 문서는 폐지했다. 위 상태 기록 링크는 당시 기록 보존용 로컬 자료다. 현재 작업 상태는 [GitHub Issue](https://github.com/jaykop/Kata/issues)를 따른다.
