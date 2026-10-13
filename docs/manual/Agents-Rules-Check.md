# AGENTS.md 규칙 검사 사용법

갱신: 2026-10-13  
대상: 저장소에 변경을 올리는 사용자와 코딩 에이전트(Claude·Codex·Copilot)  
적용 기준: `Scripts/CheckAgentsRules.py`, `.github/workflows/agents-rules.yml`, AGENTS.md "모듈 경계"·"프로젝트와 현재 범위"  
확인 상태: 2026-10-13 로컬 실행과 GitHub Actions에서 통과를, Actions에서 위반 검출(실패와 파일·줄 annotation)을 확인했다. PR 화면 표시와 fork PR 동작은 확인하지 않았다.

## 목적과 준비

GitHub Actions가 push·pull_request마다 AGENTS.md 규칙 가운데 빌드 없이 파일 텍스트로 판정할 수 있는 것을 검사한다.
플러그인 의존 방향, 런타임 모듈의 에디터 의존, 샘플 전용 플러그인 참조, 복제·RPC 선언이 PR 단계에서 드러난다.
컴파일·쿠킹·자동화 테스트는 하지 않는다. GitHub 호스트 러너에는 Unreal Engine이 없고, 공개 저장소이므로 셀프 호스트 러너를 쓰지 않는다.

로컬에서 실행하려면 Python 3.10 이상이 필요하다. 외부 패키지는 쓰지 않으며 파일을 읽기만 한다.

## 사용 순서

### 로컬에서 실행

프로젝트 루트에서 실행한다.

```powershell
python Scripts/CheckAgentsRules.py
python Scripts/CheckAgentsRules.py --list-rules
```

종료 코드는 위반이 없으면 0, 위반이 있으면 1, 아래 구성 오류면 2다. 위반은 `경로:줄: [규칙 ID] 내용`과 AGENTS.md의 항목 제목·규칙 요약으로 출력한다.
다음은 형식을 보여 주는 예시이며 실제 실행 결과가 아니다.

```text
Plugins/Kata/Kata.uplugin:40: [core-plugin-allowlist] core plugin Kata depends on plugin StateTree (.uplugin Plugins)
    AGENTS.md "모듈 경계": The core plugin depends only on the engine and GAS; its .uplugin may list only GameplayAbilities.
```

### GitHub에서 결과 보기

1. 워크플로 `AGENTS rules`의 작업 `Module boundaries and single-player scope`가 모든 브랜치의 push, pull_request, Actions 탭의 수동 실행(Run workflow) 때 돈다.
2. 위반이 있으면 작업이 실패한다. 위반한 파일·줄은 실행 요약의 annotation에 나오며, PR에서 바뀐 줄이면 Files changed의 해당 줄에도 표시된다.
3. 같은 ref에 새 실행이 시작되면 진행 중인 이전 실행을 취소한다.

필수 검사(브랜치 보호)로는 지정하지 않았다. 실패해도 병합이 막히지 않으므로 병합 전에 결과를 확인한다.

### 위반을 고칠 때

1. 출력의 규칙 ID와 AGENTS.md 항목을 읽고 의존 방향을 바꾼다. 예를 들어 코어가 위성 기능을 써야 하면 코어에는 확장 지점만 두고 위성이나 통합 플러그인에서 구현한다.
2. 규칙 자체가 바뀌어야 하면 AGENTS.md와 스크립트의 규칙·플러그인 표, 이 문서를 같은 작업에서 고친다. 스크립트만 고쳐 검사를 통과시키지 않는다.

## 주요 설정과 실행 규칙

"Kata 플러그인"은 `Kata`·`KataTargeting`·`KataAI`·`KataCamera`·`KataFramework`·`KataGASInspector` 여섯 개다.
벤더 플러그인 `WindowsDualsense_ds5w`는 원문을 유지하는 코드이므로 검사하지 않고 참조 대상으로만 판정한다.

| 규칙 ID | 검사 대상 | 위반으로 보는 것 | AGENTS.md 근거 |
|---|---|---|---|
| `core-no-satellite` | `Plugins/Kata`의 .uplugin·Build.cs | KataTargeting·KataAI·KataCamera·KataFramework 플러그인이나 그 모듈 참조 | 모듈 경계 |
| `satellite-deps` | 위성 세 개의 .uplugin·Build.cs | KataAI → KataTargeting 외의 위성 간 참조, KataFramework 참조 | 모듈 경계 |
| `core-plugin-allowlist` | `Kata.uplugin`의 `Plugins` | GameplayAbilities 외의 플러그인 | 모듈 경계 |
| `runtime-no-editor` | Kata 플러그인의 Runtime 계열 모듈 Build.cs | 에디터 전용 모듈 의존. `if (Target.bBuildEditor)` 안의 의존도 포함 | 모듈 경계 |
| `no-gas-inspector-ref` | 코어·위성·통합의 .uplugin·Build.cs | KataGASInspector 플러그인이나 KataGASInspectorEditor 모듈 참조 | 프로젝트와 현재 범위 |
| `sample-only-plugins` | Kata 플러그인의 .uplugin·Build.cs | 이름이 `WindowsDualsense`·`ChaosClothAsset`·`SlateIM`으로 시작하는 플러그인·모듈 참조 | 프로젝트와 현재 범위 |
| `no-sample-module-ref` | Kata 플러그인의 Build.cs | `.uproject`에 선언한 샘플 모듈(ProjectKata·ProjectKataEditor) 의존 | 프로젝트와 현재 범위 |
| `no-replication` | Kata 플러그인의 C++ 소스(.h·.hpp·.cpp·.inl) | 아래 복제·RPC 선언 | 프로젝트와 현재 범위 |

`no-replication`이 위반으로 보는 선언은 다음과 같다. 지정자는 대소문자를 구분하지 않는다.

- `UPROPERTY`의 `Replicated`·`ReplicatedUsing` 지정자
- `UFUNCTION`의 `Server`·`Client`·`NetMulticast`·`Reliable`·`Unreliable`·`WithValidation`·`ServiceRequest`·`ServiceResponse` 지정자
- `GetLifetimeReplicatedProps`, `DOREPLIFETIME`으로 시작하는 매크로
- `bReplicates`·`bReplicateMovement`에 true 대입, `SetIsReplicated`·`SetIsReplicatedByDefault`·`SetReplicates`·`SetReplicateMovement`에 true 전달

판정 방식은 다음과 같다.

- Build.cs는 주석을 건너뛰고 문자열 리터럴만 모듈 이름과 대조한다. 조건문 위치는 보지 않으므로 조건부 의존도 의존으로 본다.
- .uplugin은 `Plugins[].Name`을 읽는다. 모듈과 플러그인의 소속은 각 .uplugin의 `Modules`에서 만든다.
- C++ 소스는 주석과 문자열을 지운 뒤 `UPROPERTY(...)`·`UFUNCTION(...)` 괄호의 최상위 지정자만 본다. `meta=(...)` 안과 툴팁 문자열은 판정하지 않는다.
- 한 참조에는 규칙 하나만 보고한다. 플러그인 관계 규칙을 먼저 보고, 해당하지 않을 때 `runtime-no-editor`를 본다.

에디터 전용 모듈과 Runtime 계열 모듈은 다음과 같이 정한다.

| 구분 | 기준 |
|---|---|
| 에디터 전용 모듈 | 스크립트의 `ENGINE_EDITOR_MODULES` 목록(UnrealEd, AssetTools, AssetDefinition, Kismet, BlueprintGraph, AnimGraph 등), 이름이 `Editor`로 시작하거나 `Editor`·`EditorModule`로 끝나는 모듈, Kata 플러그인 .uplugin과 .uproject에서 Type이 Editor·EditorNoCommandlet·EditorAndProgram·UncookedOnly인 모듈 |
| Runtime 계열 모듈 | .uplugin의 Type이 Runtime·RuntimeNoCommandlet·RuntimeAndProgram·CookedOnly·ClientOnly·ClientOnlyNoCommandlet·ServerOnly인 모듈 |

다음은 구성 오류로 보고 종료 코드 2로 끝낸다. 플러그인 구성이 바뀌었는데 검사가 조용히 통과하는 일을 막기 위한 처리다.

- Kata 플러그인 여섯 개 중 하나가 `Plugins/` 아래에 없다.
- 표에 없는 `Kata`로 시작하는 플러그인이 생겼다.
- Build.cs의 모듈이 소속 .uplugin의 `Modules`에 선언되지 않았다.
- .uplugin·.uproject의 JSON을 읽지 못했다.

워크플로는 `permissions: contents: read`로 저장소를 읽기만 하고 ubuntu-latest에서 5분 제한으로 실행한다.
`actions/checkout`은 v7.0.1의 커밋 SHA로 고정하고 `persist-credentials: false`를 지정하며 LFS 파일은 받지 않는다. 검사는 러너에 설치된 `python3`로 실행한다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 할 일 |
|---|---|---|
| 컴파일·include 오류를 잡지 못한다 | 텍스트로만 검사한다 | 사용자가 `Scripts/Build.ps1`로 빌드해 확인한다 |
| Build.cs에서 변수나 메서드로 만든 모듈 이름을 놓친다 | 문자열 리터럴만 모듈 이름과 대조한다 | 의존 모듈 이름은 문자열로 적는다 |
| 의존이 아닌 용도의 문자열도 모듈 이름과 같으면 위반으로 본다 | 리터럴의 용도를 해석하지 않는다 | 규칙에 맞는 구조로 바꾼다. 예외가 필요하면 AGENTS.md·스크립트·이 문서를 함께 고친다 |
| 목록에 없는 엔진 에디터 모듈 의존을 놓친다 | 엔진 에디터 모듈 전체 목록을 갖고 있지 않다 | 발견하면 `ENGINE_EDITOR_MODULES`에 더한다 |
| 샘플 전용 `MVVMToolset` 참조를 놓친다 | 현재 `SAMPLE_ONLY_PREFIXES`에 `MVVMToolset`이 없다 | AGENTS.md의 샘플 전용 범위를 직접 대조한다. 검사 목록 보완은 별도 코드 수정이 필요하다 |
| `#if 0` 안의 선언도 위반으로 본다. 매크로로 감싼 지정자는 놓친다 | 전처리를 해석하지 않는다 | 쓰지 않는 선언은 지운다 |
| `FPredictionKey` 같은 GAS 예측 API는 검사하지 않는다 | 엔진 가상 함수 시그니처에도 쓰여 텍스트로 구분할 수 없다 | 리뷰에서 확인한다 |
| 종료 코드 2 | 위 구성 오류 | AGENTS.md의 플러그인 구성과 스크립트 상단의 플러그인 표를 함께 갱신한다 |
| Windows 콘솔에서 한국어 항목 제목이 `\u` 형식으로 보인다 | 콘솔 인코딩이 표현하지 못한 문자를 이스케이프했다 | `python -X utf8 Scripts/CheckAgentsRules.py`로 실행한다 |

## 확인 상태와 근거

- 2026-10-13 작성 시점에 .uplugin과 Build.cs를 읽어 규칙과 대조했고 위반을 찾지 못했다.
- 2026-10-13 사용자 요청으로 Claude가 로컬에서 실행했다. 다른 작업의 미커밋 변경이 있는 작업 트리에서 .uplugin 6개, Build.cs 12개, C++ 파일 456개를 검사했고 위반 없이 종료 코드 0으로 끝났다.
- 2026-10-13 GitHub Actions의 main push(4b0843b)에서 위반 없이 통과했다. 임시 브랜치(병합하지 않고 삭제)에 코어 → 위성 참조(`KataRuntime.Build.cs`의 `KataAI`)와 `SetReplicates(true)`를 넣은 push에서는 종료 코드 1로 실패했고, 두 위반이 `core-no-satellite`·`no-replication` annotation으로 파일·줄에 표시됐다.
- PR의 Files changed 화면 표시와 fork PR 동작은 확인하지 않았다.
- [도입 결정 기록](../devlog/2026-10-13-Agents-Rules-CI.md).
- [검사 스크립트](../../Scripts/CheckAgentsRules.py): 플러그인 표, `RULES`, `Checker`.
- [워크플로](../../.github/workflows/agents-rules.yml).
- [AGENTS.md](../../AGENTS.md): "모듈 경계", "프로젝트와 현재 범위".
- [작업 상태 #93](https://github.com/jaykop/Kata/issues/93).
