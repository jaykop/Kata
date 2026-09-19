# Kata

Unreal Engine 5.8과 Gameplay Ability System(GAS)을 기반으로 만드는 싱글플레이 캐릭터 액션 타임라인 플러그인입니다.

이 저장소에는 재사용 가능한 `Plugins/Kata` 플러그인과 개발·검증용 `ProjectKata` 프로젝트가 함께 있습니다. 현재 세 모듈과 기본 조건(Tag, Attribute, Distance, Angle)이 구현되어 있으며 액션 실행, 타임라인 편집, 콤보 및 프리뷰 기능은 아직 구현되지 않았습니다.

## 구조

```text
Plugins/Kata/
  Kata.uplugin
  Source/
    KataConditions/   공용 조건 기반 계층 (Runtime)
    KataRuntime/      액션·태스크·GAS 실행 계층 (Runtime)
    KataEditor/       타임라인·프리뷰 편집 계층 (Editor)
Source/ProjectKata/   개발·검증용 게임 모듈
Config/              프로젝트 공유 설정
Scripts/             빌드 스크립트
docs/                설계 제안 및 현재 구현 상태
```

KataRuntime은 KataConditions에 의존하고, KataEditor는 두 런타임 모듈에 의존합니다. GAS는 필수입니다. KataAI와 멀티플레이 기능은 현재 범위에 없습니다.

## 시작하기

1. Unreal Engine 5.8과 UE C++ 개발 환경을 설치합니다.
2. 이 저장소를 clone하고 Git LFS를 초기화합니다. `git lfs install` 후 에셋이 있는 경우 `git lfs pull`을 실행합니다.
3. `ProjectKata.uproject`에서 프로젝트 파일을 생성하거나 다음 PowerShell 명령으로 빌드합니다.
4. `ProjectKata.uproject`를 열어 개발을 시작합니다. 초기 프로젝트는 엔진 기본 맵을 사용하며 별도 게임플레이 샘플은 없습니다.

```powershell
.\Scripts\Build.ps1 -Target Editor
.\Scripts\Build.ps1 -Target Game
```

엔진 경로는 `-EngineRoot` 또는 `UE_ENGINE_ROOT` 환경 변수로 지정할 수 있습니다. 값을 지정하지 않으면 UE 레지스트리와 기본 설치 경로를 확인합니다. Visual Studio/Rider 프로젝트 파일은 생성 산출물이므로 저장소에 포함하지 않습니다.

다른 UE 5.8 프로젝트에서 사용할 때는 `Plugins/Kata`를 복사하고 Kata를 활성화합니다. 프로젝트 C++ 코드에서 공개 API를 참조하려면 해당 게임 모듈의 Build.cs에 사용하는 Kata 모듈 의존성을 추가합니다.

## 문서

- [다음 작업 계획 — Claude Code 인계](docs/Next-Work-Plan.md).

- [기본 조건 사용법](docs/Conditions.md).

- [공용 에이전트 지침](AGENTS.md): Claude와 Codex가 함께 따르는 작업 규칙.
- [현재 구현 상태](docs/Implementation-Status.md): 실제 구현 범위와 검증 결과.
- [Claude 설계 제안](docs/Kata-Plugin-Design.md).
- [Codex 설계 제안 및 기존안 검토](docs/Kata-Plugin-Design-Codex.md).

설계 문서는 논의 자료이며 현재 구현·확정 사항과 다를 수 있습니다. 현재 범위는 공용 지침과 구현 상태 문서를 기준으로 확인합니다.
