# 자동화 테스트 작성·실행

갱신: 2026-10-15  
대상: Kata 플러그인 모듈의 C++ 자동화 테스트를 작성하거나 실행하는 사람  
적용 기준: [C++ 자동화 테스트 정책](../devlog/2026-10-14-Automation-Test-Policy.md), `KataConditions`·`KataRuntime`·`KataGraph`의 `Private/Tests/` 소스  
확인 상태: `Kata.Runtime.Resolve`·`Kata.Runtime.Cleanup`·`Kata.Graph.Transition` 18건을 2026-10-15 사용자가 Editor에서 실행해 모두 통과했다. headless 실행 명령은 이 세 그룹으로 실행하지 않았다.

## 목적과 준비

C++ 자동화 테스트는 두 가지에 쓴다. 하나는 입력과 결과로 확인할 수 있는 로직(유닛 테스트)이고, 다른 하나는 시작·취소·중단을 반복해도 자원과 구독이 남지 않는다는 불변식(반복 테스트)이다. 시간을 단언하는 성능·부하 테스트는 쓰지 않는다. 성능은 측정으로, 블루프린트·에셋·UI·연출은 플레이로 확인한다. 테스트는 이슈로 정한 대상이나 사용자 요청이 있을 때만 추가한다([AGENTS.md](../../AGENTS.md) 테스트·검사 담당 원칙).

실행하려면 Editor 빌드(`.\Scripts\Build.ps1 -Target Editor`)를 먼저 마쳐야 한다. 테스트는 `WITH_DEV_AUTOMATION_TESTS`가 켜진 빌드에만 들어가고 Shipping 빌드에는 포함되지 않는다.

## 테스트 작성

1. 테스트 대상 모듈의 `Private/Tests/`에 `.cpp` 파일을 둔다. 별도 테스트 모듈은 만들지 않는다.
2. 테스트 대상 헤더를 include한 뒤 테스트 코드 전체를 `#if WITH_DEV_AUTOMATION_TESTS`로 감싼다.
3. `IMPLEMENT_SIMPLE_AUTOMATION_TEST`로 테스트를 선언하고 플래그는 `EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter`를 쓴다.
4. 단언 설명 문자열은 영어로, 주석은 한국어로 쓴다.
5. 프로젝트 에셋에 기대지 않는다. 월드가 필요하면 테스트 안에서 임시 월드를 만들고(`KataConditionTests.cpp`의 `FFixture`), 필요 없으면 transient 패키지 아래에 객체를 만든다.
6. 시간이 아니라 개수와 상태(태그 카운트, 활성 태스크 수, 인스턴스 상태, 진단 코드)를 단언한다.
7. GameplayTag가 필요하면 ProjectKata의 `Config/Tags`에 등록된 태그를 빌려 쓴다. 플러그인은 태그를 정의하지 않으므로 테스트 전용 태그도 만들지 않는다. 이런 테스트는 ProjectKata에서만 통과한다. ASC의 태그 수를 단언할 때는 자식 태그가 없는 태그를 고른다. 부모 태그의 수에는 자식 태그의 수가 합쳐진다.

```cpp
#include "Action/KataAction.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataExampleTest, "Kata.Runtime.Resolve.Example",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKataExampleTest::RunTest(const FString& Parameters)
{
    // 에셋을 만들지 않고 transient 패키지 아래에서 준비한다.
    UKataAction* Action = NewObject<UKataAction>(GetTransientPackage());
    TestFalse(TEXT("Empty action resolves without errors"), Action->Resolve(GetTransientPackage())->HasErrors());
    return true;
}

#endif
```

예제는 형식을 보이기 위한 것이며 실행 확인 대상이 아니다.

## 이름 규칙과 그룹

테스트 이름은 `Kata.<모듈>.<영역>.<경우>`이다. 실행할 때는 `Kata.<모듈>.<영역>`까지를 그룹 이름으로 넘긴다. `KataConditions` 모듈만 기존 이름 `Kata.Conditions.<조건>.<경우>`를 유지한다.

| 그룹 | 모듈 | 소스 | 대상 |
|---|---|---|---|
| `Kata.Conditions` | KataConditions | `Private/Tests/KataConditionTests.cpp` | Tag·Attribute·Distance·Angle 조건 판정, Invert·Invalid, 에셋 검증 |
| `Kata.Runtime.Resolve` | KataRuntime | `Private/Tests/KataResolveTests.cpp` | 액션 상속 해석, 태스크 오버라이드, 진단 코드, 시작 판정용 해석, `KataPropertyOverride::CopyOverriddenProperty` |
| `Kata.Runtime.Cleanup` | KataRuntime | `Private/Tests/KataCleanupTests.cpp` | 종료 경로 4개(StopKata, TryCancelKata, CancelAbilitiesAndKata, 교체 Play)와 반복·무작위 순서에서 Loose Tag·Active Granted Tags·Blocked Ability Tags·전이·캔슬 창 회수, 캔슬 창 판정. ProjectKata 태그 필요 |
| `Kata.Graph.Transition` | KataGraph | `Private/Tests/KataGraphTransitionTests.cpp` | 트리거 전이(Immediate·예약·보관·우선순위), 게임플레이 거절과 계약 오류 구분, Any State Alias, 펼친 SubGraph 진입 노드, 동기 전이 한도. 그래프 자산은 코드로 만든다. ProjectKata 태그 필요 |

## 실행

Editor에서는 Session Frontend의 Automation 탭에서 그룹을 골라 실행한다. headless로 실행할 때는 프로젝트 루트에서 다음 명령을 쓰고, `$group`에 위 표의 그룹 이름을 넣는다.

```powershell
$engineRoot = 'C:\Program Files\Epic Games\UE_5.8'
$group = 'Kata.Runtime.Resolve'
& "$engineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "$PWD\ProjectKata.uproject" -unattended -nullrhi -nosplash -nosound -nop4 `
  "-ExecCmds=Automation RunTests $group" `
  '-TestExit=Automation Test Queue Empty' `
  "-ReportExportPath=$PWD\Saved\Automation\$group"
```

결과는 `-ReportExportPath`에 지정한 폴더의 보고서와 `Saved/Logs`의 로그에서 확인한다. 그룹 이름이 틀려도 오류 없이 0건 실행으로 끝날 수 있다. 실행한 테스트 수가 0이 아니고 모두 통과했는지 함께 확인한다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 테스트 목록에 Kata 테스트가 없다 | Editor 빌드 전이거나 개발용 자동화 테스트가 빠진 빌드다 | Editor 빌드를 마친 뒤 다시 실행한다 |
| 실행한 테스트 수가 0이다 | 그룹 이름이 틀렸다 | 위 표의 그룹 이름을 확인한다 |
| 태스크 완료, 루프 경계, 보관 트리거 재평가를 단언할 수 없다 | 시간 진행이 필요하다. 런타임 인스턴스는 같은 프레임 번호에서 두 번째 Tick을 무시한다 | 플레이로 확인한다 |
| 그래프 테스트가 SubGraph 펼침 자체는 확인하지 않는다 | SubGraph 펼침은 에디터가 저장할 때 하는 단계다. 테스트는 펼친 결과의 모양을 코드로 만들어 런타임 규칙만 확인한다 | 펼침은 에디터에서 그래프를 저장해 확인한다 |
| 테스트가 엔진 오류 로그 때문에 실패한다 | 자동화 테스트는 실행 중 Error 로그가 나오면 실패로 본다. 계약 오류처럼 일부러 만든 실패는 Kata가 Error 로그를 남긴다 | 테스트에서 `AddExpectedErrorPlain`으로 기대한 로그와 횟수를 등록한다 |
| 몽타주·사운드·카메라·VFX·Hit Trace 태스크를 테스트할 수 없다 | 에셋·AnimInstance·PlayerController가 필요하다 | 플레이로 확인한다 |
| ProjectKata가 아닌 프로젝트에서 `Kata.Conditions`·`Kata.Runtime.Cleanup`·`Kata.Graph.Transition`이 실패한다 | 세 그룹은 ProjectKata의 태그(`Status.*`, `Window.*`, `Event.Hit`, `Trigger.*`)를 쓴다. `Kata.Runtime.Resolve`는 태그가 필요 없는 태스크로 구성했다 | ProjectKata에서 실행한다 |
| 전이·캔슬 창이 몇 번 열렸는지 단언할 수 없다 | `UKataActionInstance`는 창이 열려 있는지만 태그별로 알려 주고 열린 수는 공개하지 않는다. ASC의 Blocked Ability Tags 수도 공개되지 않는다 | 열림 여부와 차단 여부로 단언한다 |

## 확인 상태와 근거

`Kata.Conditions`의 Distance 두 테스트는 2026-09-24 사용자가 실행해 확인했다([기본 조건](Conditions.md)). 2026-10-15 사용자가 Editor에서 `Kata.Runtime.Resolve` 8건, `Kata.Runtime.Cleanup` 5건, `Kata.Graph.Transition` 5건을 실행해 모두 통과했다. 같은 날 에이전트가 세 그룹이 들어간 Editor 빌드를 확인했다. 위 headless 실행 명령으로 이 세 그룹을 끝까지 돌린 기록은 없다. 같은 날 시도는 엔진 설치 문제로 엔진 시작 단계에서 멈췄다.

- [KataConditionTests.cpp](../../Plugins/Kata/Source/KataConditions/Private/Tests/KataConditionTests.cpp): 임시 월드·ASC를 쓰는 테스트 형식.
- [KataResolveTests.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Tests/KataResolveTests.cpp): 월드 없이 transient 객체로 해석 결과를 단언하는 테스트 형식.
- [KataCleanupTests.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Tests/KataCleanupTests.cpp): 임시 월드의 액션 컴포넌트로 종료 경로별 자원 회수를 개수로 단언하는 테스트 형식.
- [KataGraphTransitionTests.cpp](../../Plugins/Kata/Source/KataGraph/Private/Tests/KataGraphTransitionTests.cpp): 런타임 그래프 모양을 코드로 만들어 전이 결과를 단언하는 테스트 형식.
- [C++ 자동화 테스트 정책](../devlog/2026-10-14-Automation-Test-Policy.md): 테스트 대상을 고른 이유와 시간 단언 금지.
- [작업 상태](https://github.com/jaykop/Kata/issues/101).
