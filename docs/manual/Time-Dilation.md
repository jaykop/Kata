# 시간 배율 요청 사용법

갱신: 2026-10-12  
대상: KataFramework `UKataTimeDilationSubsystem`, `Kata Task: Time Dilation`  
적용 기준: [#65](https://github.com/jaykop/Kata/issues/65) 구현 소스  
확인 상태: 2026-10-12 사용자 Editor 빌드와 실행 확인. 액션 프리뷰 재생·탐색(Self·World), PIE의 몽타주·타임라인 일치와 일시정지 복귀, 기존 액션 회귀를 확인했다고 보고했다.

## 목적과 준비

히트스톱, 슬로모션, 연출처럼 여러 기능이 시간 배율을 바꿀 때 서로 덮어쓰지 않게 한다.
엔진의 시간 배율은 월드 하나(`AWorldSettings`의 Time Dilation)와 액터마다 하나(`AActor::CustomTimeDilation`)뿐이라, 직접 쓰면 마지막에 쓴 쪽이 이긴다.
이 값을 바꾸는 기능은 엔진 값을 직접 쓰지 않고 `UKataTimeDilationSubsystem`에 요청한다.

Subsystem은 게임·PIE 월드와 액션 에디터 프리뷰 월드에서 자동으로 만들어진다. 따로 배치하거나 설정할 것은 없다.
Kata 실행기가 배율을 따르는 규칙은 [런타임 사용법](Runtime-Usage.md)에 적혀 있다.

## 사용 순서

### 액션 타임라인에서

1. 액션 에디터의 Add Task 메뉴에서 Time 묶음의 `Kata Task: Time Dilation`을 추가한다.
2. 배율을 걸 구간을 Start Time과 Duration으로 잡는다. Single Frame이나 Duration 0은 설정 오류다.
3. Target, Time Dilation, Priority를 정한다.
4. 프리뷰에서 재생하면 구간 동안 캐릭터(Self)나 프리뷰 월드 전체(World)가 느려진다.

### C++에서

```cpp
#include "Time/KataTimeDilationSubsystem.h"

// 소스 기준 예제이며 이 코드 자체를 실행해 확인하지는 않았다.
if (UKataTimeDilationSubsystem* TimeDilation = GetWorld()->GetSubsystem<UKataTimeDilationSubsystem>())
{
    // 0.08초 동안 공격자를 거의 멈춘다. 시간이 지나면 스스로 풀린다.
    TimeDilation->RequestActorTimeDilation(Attacker, 0.05f, /*Priority*/ 10, /*Duration*/ 0.08f);

    // 해제할 때까지 월드를 느리게 한다. 받은 핸들을 보관했다가 풀어야 한다.
    SlowMotionHandle = TimeDilation->RequestWorldTimeDilation(0.3f, /*Priority*/ 0);
}
// 나중에
TimeDilation->ReleaseRequest(SlowMotionHandle);
```

Blueprint API는 아직 제공하지 않는다.

## 주요 설정과 동작 규칙

| UI 항목 또는 API | 의미·입력 | 기본값·실패 시 동작 |
|---|---|---|
| `RequestActorTimeDilation` | 대상 액터의 `CustomTimeDilation` 요청. 배율은 0 이상의 유한한 값 | 대상이 없거나 다른 월드 액터이거나 값이 잘못되면 경고를 남기고 `INDEX_NONE` |
| `RequestWorldTimeDilation` | 월드 Time Dilation 요청 | 실제 값은 World Settings의 Min·Max Global Time Dilation으로 잘린다 |
| `Duration` (API) | 0보다 크면 그 시간 뒤 스스로 풀린다 | 0 이하이면 `ReleaseRequest`를 부를 때까지 유지 |
| `ReleaseRequest` | 핸들로 요청을 푼다 | 이미 만료됐거나 `INDEX_NONE`이면 무시 |
| Priority (API·태스크) | 정수. 클수록 우선 | 같으면 나중 요청이 이긴다 |
| Target (태스크) | Self: 실행 기준 액터. World: 월드 전체 | Self |
| Time Dilation (태스크) | 0.01 이상 | 0.5 |
| Duration (태스크) | 요청을 유지할 구간. Kata 시각으로 잰다 | 0.2초 |

- **합성:** 같은 층에서는 이긴 요청 하나만 엔진 값에 쓴다. 월드 층과 액터 층은 엔진이 곱한다. 예를 들어 월드 0.5에 액터 0.1이면 그 액터는 0.05배로 움직인다.
- **원래 값 복원:** 층에 첫 요청이 들어올 때 원래 값을 기억하고, 마지막 요청이 끝나면 그 값으로 되돌린다.
- **만료 시계:** 시간 배율의 영향을 받지 않고, 엔진 일시정지 중에는 멈춘다. 0.1초 요청은 배율과 관계없이 실제로 0.1초 유지되고, 메뉴로 일시정지하면 만료도 멈춘다.
- **정리:** 대상 액터가 파괴되면 그 액터의 요청은 사라진다. 태스크의 요청은 완료·취소·중단·소유자 파괴 등 어떤 종료에서도 풀린다.
- **태스크 구간 길이:** 구간은 Kata 시각으로 재므로 Self·World 대상이면 타임라인도 함께 느려진다. 배율 0.5에 Duration 0.2초인 구간은 실제로 약 0.4초 유지된다. 하한 0.01은 이 때문에 둔다.
- **일시정지:** 요청으로 다루지 않는다. 엔진 일시정지를 쓴다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 배율이 요청과 다르게 돌아간다 | 요청이 있는 동안 다른 코드가 `CustomTimeDilation`이나 월드 Time Dilation을 직접 바꿨다. 그 층의 다음 갱신(요청·해제·만료) 때 덮어쓴다 | 배율을 바꾸는 코드를 모두 요청 API로 옮긴다 |
| 액터 배율 0 요청 뒤 캐릭터가 계속 멈춰 있다 | 해제 때까지 유지하는 요청을 풀지 않았다 | 0 배율에는 Duration을 주거나 핸들로 반드시 해제한다 |
| 프리뷰 탐색이 아주 느린 구간 너머의 시각에 도달하지 못한다 | 탐색은 Kata 시각 단계를 월드 시간으로 늘려 진행하지만 엔진 최대 프레임 시간에 잘린다. 월드·액터 배율을 곱한 값이 약 0.005보다 작으면 단계 수 상한에 걸릴 수 있다 | 재생으로 확인하거나 배율을 높인다 |

## 확인 상태와 근거

2026-10-12 사용자가 Editor 빌드와 항목별 실행 확인(프리뷰 재생·탐색, World 대상, PIE, 일시정지, 회귀)을 마쳤다고 보고했다. Game 타깃 빌드 결과는 전달되지 않았다. 엔진 동작은 UE 5.8 소스에서 확인했다.

- [KataTimeDilationSubsystem.h](../../Plugins/KataFramework/Source/KataFramework/Public/Time/KataTimeDilationSubsystem.h): 요청·합성·복원·만료 규칙.
- [KataTask_TimeDilation.h](../../Plugins/KataFramework/Source/KataFramework/Public/Tasks/KataTask_TimeDilation.h): 태스크 설정과 해제 시점.
- [KataActionInstance.h](../../Plugins/Kata/Source/KataRuntime/Public/Runtime/KataActionInstance.h): `GetTimeDilationActor`, `GetCustomTimeDilation`.
- [시간 배율 요청 결정](../devlog/2026-10-12-Time-Dilation-Requests.md): 배치·합성·만료 기준을 정한 이유.
- [작업 상태 #65](https://github.com/jaykop/Kata/issues/65).
