# 태스크와 Command 제작

갱신: 2026-09-25  
대상: C++·Blueprint 확장 제작자  
적용 기준: 현재 KataRuntime 공개 API, [#12](https://github.com/jaykop/Kata/issues/12)  
확인 상태: 소스 대조. 아래 예제 절차의 빌드·Blueprint 실행은 미실시.

## 목적과 준비

타임라인 동작은 `UKataTask`에 설정을, `UKataTaskInstance`에 실행별 상태를 둔다.
액션 시작·종료에 한 번 실행할 동작은 `UKataCommand`로 만든다.
기본 제공 태스크의 설정만 필요하면 [런타임 사용법](Runtime-Usage.md#기본-태스크)을 먼저 따른다.

| 동작 | 확장 방법 |
|---|---|
| 특정 시점의 이벤트·GE 적용 | 기본 태스크 설정 |
| 시간 구간, Tick, 비동기 완료, 효과 유지·회수 | Task·TaskInstance 한 쌍 |
| 시작 시 대상 결정, 종료 시 한 번 처리 | Pre Commands·Post Commands |
| 상태를 읽어 실행 가능 여부 판정 | [Condition](Conditions.md) |

재사용 코드는 해당 플러그인에 둔다. C++ 모듈은 공개 타입 사용에 맞춰 KataRuntime 의존성을 추가한다.
추가 엔진 플러그인이 필요한 시스템은 위성·통합 플러그인에 두며, 코어에서 역방향 의존을 만들지 않는다.

## Blueprint 태스크 제작 순서

1. UKataTaskInstance 파생 실행 Blueprint를 만든다.
2. On Task Started에서 Get Task Definition을 설정 BP로 Cast해 읽고, Get Kata Context로 실행자·대상을 얻는다.
3. 지속 진행이 필요하면 On Task Tick을 구현한다. DeltaTime은 0일 수도 있다.
4. On Task Ended에서 타이머·이벤트 구독·생성 객체·효과 등 자신이 획득한 자원을 정리한다.
5. UKataTask 파생 설정 Blueprint를 만들고 실행 중 바뀌지 않는 설정 변수를 둔다.
6. 설정 BP의 Get Task Instance Class를 재정의해 1번 실행 BP를 반환한다.
7. Kata 에디터의 Add Task에서 설정 BP를 선택하고 Start Time·Duration·설정을 지정한다.

기본 반환 클래스와 null 대체 클래스는 UKataTaskInstance다. 연결을 누락하면 사용자 로직 없이 실행될 수 있다.
Add Task에는 추상·폐기 클래스 제외와 미로드 BP 필터가 있다. 새 BP의 실제 메뉴 노출·실행은 사용자가 확인한다.

## C++ 확장점

헤더는 `Action/KataTask.h`, `Runtime/KataTaskInstance.h`를 사용한다.

| 함수 | 역할 |
|---|---|
| GetTaskInstanceClass_Implementation() const | 대응 실행 클래스의 StaticClass 반환 |
| GetConfigurationError() const | Super 오류를 먼저 확인. 유효하면 NAME_None, 잘못된 설정이면 오류 이름 반환 |
| DescribeConfigurationError(FName) const | 자체 오류 설명을 제공하고 나머지는 Super로 전달 |
| OnTaskStarted_Implementation() | 실행 변수 초기화·자원 획득·구독 |
| OnTaskTick_Implementation(float) | 이번 실행 구간 진행 |
| OnTaskEnded_Implementation(EKataTaskEndReason) | 자원·구독 정리 |

설정 검사는 C++ virtual이며 BP 이벤트가 아니다. 현재 BP 전용 설정 검사 확장점은 없다.
구체 예제는 [Send Gameplay Event](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_SendGameplayEvent.cpp)와
[Apply Loose Tag](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_ApplyLooseTag.cpp)를 참고한다.

## 실행 수명과 제한

- 일반 지속 태스크는 Start → Tick들 → End다. Duration 0은 Start → End, Single Frame은 Start → Tick 한 번 → End다.
- FinishTask는 조기 완료다. 시작 콜백에서 호출하면 종료 콜백이 반환 전에 실행될 수 있으므로 이후에 자원을 새로 획득하지 않는다.
- 실행한 태스크의 종료 콜백은 한 번만 호출한다. 시작하지 않고 Skipped가 된 태스크에는 종료 콜백이 없다.
- 루프마다 같은 인스턴스를 재사용한다. `ResetForExecution()`은 기반 상태만 초기화하며 BP 이벤트나 virtual 확장점이 아니다.
  사용자 BP·C++ 변수는 OnTaskStarted에서 초기화한다. Restart On Loop를 끄면 첫 회차 이후 Skipped가 된다.
- 해석된 Task·조건 설정에 시간·히트 목록·ASC·효과 핸들을 저장하지 않는다. 실행 상태는 TaskInstance에 둔다.
  소유하는 UObject 참조는 UPROPERTY로 GC 추적을 유지하고, 외부 대상을 소유하지 않는 참조는 약한 참조로 관리한다.
- Get Kata Context는 사본을 반환한다. Task가 그 값을 바꿔도 액션의 Context는 바뀌지 않는다.

## Command 제작

`Action/KataCommand.h`의 UKataCommand를 상속한다. C++는 Execute_Implementation(UKataActionInstance*), BP는 Execute를 구현한다.
Kata Action Details의 Pre Commands·Post Commands 목록에서 사용한다. 실행 객체는 매 액션 해석에서 복제한다.

- Pre Commands는 시작 판정을 통과해 GAS 활성 상태를 적용한 뒤, 타임라인보다 먼저 선언 순서로 실행한다.
  실행 중에만 Instance의 SetTargetActor로 대상을 변경할 수 있다. 새 대상으로 StartCondition을 다시 평가하지 않는다.
- Post Commands는 태스크 정리 뒤 GAS 활성 상태 회수 전에 실행한다. End Reasons가 비면 모든 종료 사유를 받는다.
- 루프 회차마다 다시 실행하지 않는다. 시작 전에 거절된 액션에서는 Post Commands도 실행하지 않는다.
- Command는 같은 호출 안에 끝내며 Delay·타이머·구독을 남기지 않는다. GetWorld는 Run 동안만 유효하다.
- 현재 코어에는 구체 Command 구현이 없다. 자세한 호출 순서는 [런타임 사용법](Runtime-Usage.md#prepost-command)을 따른다.

## 프리뷰와 확인 상태

프리뷰는 실제 태스크를 실행하지만 호출 Ability와 게임 세션 초기화를 제공하지 않는다.
OwningAbility가 없는 경우와 AttributeSet·이벤트 수신 준비를 고려한다. 뒤로 탐색하면 장면을 재생성하고
처음부터 실행하므로 태스크·Command가 다시 호출된다. 장면 초기화로 되돌릴 수 없는 월드 외부 부작용에 주의한다.
태스크별 프리뷰 실행 정책과 공용 출력 채널은 아직 없다.

2026-09-24 Command의 사용자 빌드·Details 표시 기록은 있으나 런타임 실행은 미확인이다.
이번에는 소스를 읽어 문서만 작성했으며 BP 제작·루프·정리 절차를 실행하지 않았다.

- [KataTask.h](../../Plugins/Kata/Source/KataRuntime/Public/Action/KataTask.h): 설정 확장점.
- [KataTaskInstance.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataTaskInstance.cpp): 완료·초기화 계약.
- [KataCommand.h](../../Plugins/Kata/Source/KataRuntime/Public/Action/KataCommand.h): 동기 실행·월드 수명.
- [KataActionInstance.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionInstance.cpp): 실행 클래스 대체·Command 순서.
- [기존 BP Task 진단](../devlog/Function-Library-and-Blueprint-Task-Diagnosis.md): 도입 당시 판단.
