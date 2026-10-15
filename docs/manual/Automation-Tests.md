# 자동화 테스트 작성·실행

갱신: 2026-10-15  
대상: Kata 플러그인 모듈의 C++ 자동화 테스트를 작성하거나 실행하는 사람  
적용 기준: [C++ 자동화 테스트 정책](../devlog/2026-10-14-Automation-Test-Policy.md), `KataConditions`·`KataRuntime`의 `Private/Tests/` 소스  
확인 상태: 소스 기준으로 작성했다. `Kata.Runtime.Resolve`는 빌드·실행 미확인이다.

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
| 몽타주·사운드·카메라·VFX·Hit Trace 태스크를 테스트할 수 없다 | 에셋·AnimInstance·PlayerController가 필요하다 | 플레이로 확인한다 |
| 테스트에 GameplayTag가 필요하다 | 테스트용 태그를 어디에 등록할지 아직 정하지 않았다. `Kata.Conditions`는 프로젝트 태그 `Status.LockOn.*`를 쓰고, `Kata.Runtime.Resolve`는 태그가 필요 없는 태스크로 구성했다 | 새 테스트에 태그가 필요하면 연결 이슈에서 등록 위치를 먼저 정한다 |

## 확인 상태와 근거

`Kata.Conditions`의 Distance 두 테스트는 2026-09-24 사용자가 실행해 확인했다([기본 조건](Conditions.md)). `Kata.Runtime.Resolve`는 소스만 작성했고 빌드·실행 확인 전이다.

- [KataConditionTests.cpp](../../Plugins/Kata/Source/KataConditions/Private/Tests/KataConditionTests.cpp): 임시 월드·ASC를 쓰는 테스트 형식.
- [KataResolveTests.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Tests/KataResolveTests.cpp): 월드 없이 transient 객체로 해석 결과를 단언하는 테스트 형식.
- [C++ 자동화 테스트 정책](../devlog/2026-10-14-Automation-Test-Policy.md): 테스트 대상을 고른 이유와 시간 단언 금지.
- [작업 상태](https://github.com/jaykop/Kata/issues/101).
