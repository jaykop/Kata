# C++ 공용 함수 및 Blueprint Task 진단

작성일: 2026-09-20  
갱신일: 2026-09-21

## 진단 범위와 결론

Condition Function Library는 진단 결과를 반영해 실제 `UBlueprintFunctionLibrary`인 `UKataFL_Condition`으로 구현했다.
C++ 조건 UObject와 Blueprint가 같은 부작용 없는 판정 함수를 사용한다.

Task의 Blueprint 제작과 Add Task 목록 노출은 공용 함수 도입과 독립적으로 진단했다.

| 진단 항목 | 결론 |
|---|---|
| Condition 공용 함수 | `CheckAngle`, `CheckDistance`, `CheckTag`, `CompareValue`를 구현하고 기존 조건이 사용하도록 전환했다. |
| Debug Draw 공용 함수 | 분리는 가능하지만 현재 중복 제거 효과는 제한적이다. 사용 범위에 맞춰 결정한다. |
| Task의 BP 제작 | 설정용 Task와 실행용 TaskInstance 모두 소스에 기본 확장 경로가 있다. |
| BP Task의 Add Task 목록 노출 | 파생 클래스 선택 및 생성 경로가 있으며 미로드 BP 필터도 구현되어 있다. 실제 UI 동작은 미검증이다. |

Debug Draw와 Blueprint Task 항목은 진단 결과이며 구현이 확정된 후속 사양을 의미하지 않는다.

## 1. Condition 공용 함수

### 구현 상태

| 대상 | 현재 구현 | 판단 |
|---|---|---|
| 수치 비교 | Distance와 Attribute에서 중복되던 비교 연산자 6개 및 Equal/NotEqual 허용 오차 처리를 `CompareValue`로 공용화했다. | 구현 완료 |
| 비교 설정 검사 | `ValidateComparison`을 Distance와 Attribute가 공유한다. | 구현 완료 |
| Actor/Socket 위치 계산 | Distance의 `.cpp` 안에 `KataDistanceCondition::ResolveLocation`으로 구현되어 있다. | 다른 조건·Task·디버그 표시에서 사용할 때 공개 공용화 |
| 거리 계산 | 두 월드 위치의 2D/3D 거리 판정을 `CheckDistance`로 공용화했다. Actor/Socket 해석은 커스텀 구조체를 노출하지 않도록 조건에 남겼다. | 구현 완료 |
| 각도 계산 | Actor 두 개와 기본 타입 설정을 받는 `CheckAngle`로 공용화했다. | 구현 완료 |
| 태그 판정 | ASC와 `FGameplayTagContainer`를 받는 `CheckTag`로 Any/All 및 Exact 판정을 공용화했다. | 구현 완료 |
| ASC 조회 | Context의 멤버 함수로 이미 재사용한다. | 조회 정책이 다르므로 일괄 통합할 필요는 낮음 |

### 적용한 방향

`KataConditions` 모듈의 `UKataFL_Condition`에 Blueprint 순수 함수를 두고, 조건 UObject는 설정을 보관하며 실제 비교·계산은 공용 함수를 호출하도록 구성했다.

- 기존 `Pass / Fail / Invalid`와 진단 사유를 유지한다.
- Distance와 Attribute의 허용 오차 값과 의미를 보존한다.
- `Invert`는 현재처럼 `UKataCondition::Evaluate()`에서 한 번만 적용한다.
- 공개 함수의 매개변수에는 `FKataConditionContext`, `FKataConditionLocation` 같은 Kata 전용 커스텀 구조체를 사용하지 않는다.
- `OutError`가 `None`이면 bool을 Pass/Fail로 변환하고, 값이 있으면 조건 UObject가 Invalid로 변환한다.
- 외부 모듈과 Blueprint에서 사용할 함수를 Public으로 노출하고 `KATACONDITIONS_API`를 적용한다.

### 근거 파일

- [KataCondition_Distance.cpp](../../Plugins/Kata/Source/KataConditions/Private/Conditions/KataCondition_Distance.cpp)
- [KataCondition_Attribute.cpp](../../Plugins/Kata/Source/KataConditions/Private/Conditions/KataCondition_Attribute.cpp)
- [KataCondition_Angle.cpp](../../Plugins/Kata/Source/KataConditions/Private/Conditions/KataCondition_Angle.cpp)
- [KataCondition_Tag.cpp](../../Plugins/Kata/Source/KataConditions/Private/Conditions/KataCondition_Tag.cpp)
- [KataFL_Condition.h](../../Plugins/Kata/Source/KataConditions/Public/FunctionLibraries/KataFL_Condition.h)
- [KataFL_Condition.cpp](../../Plugins/Kata/Source/KataConditions/Private/FunctionLibraries/KataFL_Condition.cpp)
- [KataCondition.cpp](../../Plugins/Kata/Source/KataConditions/Private/KataCondition.cpp)
- [KataConditionTypes.cpp](../../Plugins/Kata/Source/KataConditions/Private/KataConditionTypes.cpp)
- [KataRuntimeTypes.cpp](../../Plugins/Kata/Source/KataRuntime/Private/KataRuntimeTypes.cpp)

## 2. Debug Draw 공용 함수

### 현재 상태

측정용 Grid·Sphere 그리기는 `FKataPreviewViewportClient::DrawMeasurements()` 한곳에 모여 있다. 현재 분리의 주된 효과는 중복 제거보다 뷰포트 코드 정리와 향후 재사용 준비다.

### 권장 방향

| 사용 목적 | 권장 배치와 역할 |
|---|---|
| 현재 프리뷰 측정 도형 | `KataEditor`의 `Kata::DebugDraw` 같은 공용 함수로 추출 가능 |
| 게임 실행 중 Task의 표시 | 월드 기반 그리기 함수를 런타임 모듈에 별도로 배치 |
| 조건 범위 표시 | 거리의 기준 위치·각도의 기준 방향 계산을 Condition 공용 함수와 공유 |

현재 프리뷰의 PDI 그리기 경로와 게임 월드 그리기를 무조건 하나로 합칠 필요는 없다. 두 경로가 필요해지면 도형 계산을 공유하고 출력 경로를 나누는 방식을 권장한다.

조건 평가 자체에서는 그리기를 호출하지 않는다. 계산과 표시를 분리해 조건 평가의 부작용 없는 계약을 유지한다. 또한 런타임에서 `KataEditor`를 참조하는 역방향 의존성을 만들지 않는다.

### 근거 파일

- [KataPreviewViewportClient.cpp](../../Plugins/Kata/Source/KataEditor/Private/KataPreviewViewportClient.cpp)

## 3. Task의 BP 제작 및 Add Task 목록 노출

### 현재 지원 경로

| 항목 | 소스상 지원 |
|---|---|
| 설정용 Task BP | `UKataTask`에 `Blueprintable` 선언 |
| 실행용 Task BP | `UKataTaskInstance`에 `Blueprintable` 선언 |
| BP 실행 로직 | `OnTaskStarted`, `OnTaskTick`, `OnTaskEnded` 재정의 |
| 조기 완료 | BP에서 `FinishTask()` 호출 |
| 설정과 실행 클래스 연결 | `GetTaskInstanceClass()`를 BP에서 재정의 |
| Add Task 목록 | `UKataTask` 파생 클래스 필터와 미로드 BP용 필터 구현 |
| 선택 후 생성 | 선택한 클래스로 `NewObject<UKataTask>()` 호출 |
| 런타임 인스턴스 생성 | `GetTaskInstanceClass()`의 반환 클래스로 `UKataTaskInstance` 생성 |

### 현재 구조에서의 제작 방식

설정 BP와 실행 BP 두 개를 사용한다.

1. `UKataTaskInstance` 파생 BP에 실행 이벤트를 구현한다.
2. `UKataTask` 파생 BP의 `GetTaskInstanceClass()`에서 실행 BP 클래스를 반환한다.
3. Add Task 목록에서 설정 BP를 선택한다.

소스상 별도의 수동 목록 등록 코드를 추가할 필요는 없어 보인다. 목록의 필터는 추상 클래스, 폐기된 클래스, 새 버전으로 대체된 클래스 등을 제외한다. 실제 BP 생성·목록 노출·선택 동작은 확인하지 않았다.

이 방식은 `UKataAction`을 BP로 바꾸지 않는다. 새 Kata 콘텐츠의 전용 객체 에셋 저작 방식과 설정·실행 상태 분리를 유지한다.

### 보강 후보와 사용상 제한

- **BP 설정 검사 확장:** `GetConfigurationError()`와 `DescribeConfigurationError()`는 현재 C++ virtual이다. BP 전용 설정 검사를 연결하려면 별도의 BP 확장점이 필요하다.
- **실행 클래스 누락 진단:** 현재 기본 반환값 또는 null 대체 경로는 아무 실행 로직도 없는 기본 `UKataTaskInstance`를 사용한다. BP 연결 누락을 명시적으로 진단하는 보강이 유용하다.
- **루프 초기화:** 루프마다 인스턴스를 새로 만들지 않고 재사용한다. `ResetForLoop()`는 기본 실행 상태를 초기화하므로 사용자 BP 변수는 시작 시 초기화하는 등의 처리가 필요하다.
- **자원 정리:** 타이머·이벤트 구독 등 BP가 획득한 자원은 종료 이벤트에서 정리해야 한다. 종료 이벤트가 있다는 것만으로 사용자 자원이 자동 회수되지는 않는다.
- **실행 시간:** Duration이 0인 순간 태스크는 시작 처리 후 자동 완료된다. 비동기 동작을 구현할 때 현재 타임라인의 지속 시간·종료 규칙을 고려해야 한다.

### 근거 파일

- [KataTask.h](../../Plugins/Kata/Source/KataRuntime/Public/Action/KataTask.h)
- [KataTask.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Action/KataTask.cpp)
- [KataTaskInstance.h](../../Plugins/Kata/Source/KataRuntime/Public/Runtime/KataTaskInstance.h)
- [KataTaskInstance.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataTaskInstance.cpp)
- [KataActionInstance.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionInstance.cpp)
- [KataActionEditor.cpp](../../Plugins/Kata/Source/KataEditor/Private/KataActionEditor.cpp)

## 수행 범위 및 검증 여부

Condition Function Library와 기존 조건의 위임 경로를 구현했다. Debug Draw와 Blueprint Task는 소스 구조만 진단했으며 기능 소스는 수정하지 않았다.

Condition Function Library 변경 후 사용자가 빌드 성공을 확인했다. 에이전트는 빌드, UHT, 자동화 테스트를 실행하지 않았고,
BP 노드 노출, BP 생성, Add Task UI 실행은 확인하지 않았다. Blueprint Task 지원 판단은 소스상 경로에 대한 것이며 실행 검증 완료를 의미하지 않는다.
