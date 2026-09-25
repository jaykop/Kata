# Kata

Unreal Engine 5.8과 Gameplay Ability System(GAS)을 기반으로 만드는 싱글플레이 캐릭터 액션 타임라인 플러그인입니다.

이 저장소에는 코어 `Kata`, 통합 `KataFramework`, 위성 `KataTargeting` 플러그인과 샘플 전용 `ProjectKata` 프로젝트가 있습니다.
액션 에셋·조건·태스크·GAS 실행, 콤보 그래프와 에디터, 팩션 설정이 구현되어 있습니다.
실제 타게팅 컴포넌트와 AI·카메라는 후속 범위입니다. 기능별 구현·사용자 확인 범위는
[현재 구현 상태](docs/devlog/Implementation-Status.md)를 따릅니다.

## 구조

```text
Plugins/Kata/
  Kata.uplugin
  Source/
    KataConditions/   공용 조건 기반 계층 (Runtime)
    KataRuntime/      액션·태스크·GAS 실행 계층 (Runtime)
    KataGraph/        콤보 그래프와 전이 (Runtime)
    KataEditor/       타임라인·프리뷰 편집 계층 (Editor)
    KataGraphEditor/  그래프 편집 계층 (Editor)
Plugins/KataFramework/ 통합 캐릭터 기반 (Runtime)
Plugins/KataTargeting/ 팩션 설정·판정, 타게팅 확장 기반 (Runtime)
Source/ProjectKata/   샘플 게임·프로젝트 태그
Source/ProjectKataTesting/ 개발 하네스 (DeveloperTool)
Config/              프로젝트 공유 설정
Scripts/             빌드 스크립트
docs/                설명서·결정 기록·설계 문서
```

KataRuntime은 KataConditions에, KataGraph는 KataRuntime에 의존하며 편집 코드는 Editor 모듈에 둡니다.
코어는 위성·통합 플러그인을 참조하지 않습니다. KataAI는 StateTree 기반으로 계획되어 있으며 BT·네트워크·예측은 도입하지 않습니다.

## 시작하기

1. Unreal Engine 5.8과 UE C++ 개발 환경을 설치합니다.
2. 이 저장소를 clone하고 Git LFS를 초기화합니다. `git lfs install` 후 에셋이 있는 경우 `git lfs pull`을 실행합니다.
3. `ProjectKata.uproject`에서 프로젝트 파일을 생성하거나 다음 PowerShell 명령으로 빌드합니다.
4. `ProjectKata.uproject`를 열어 개발을 시작합니다. Content Browser에서 Kata 에셋을 생성하면 전용 에디터로 편집할 수 있습니다.

```powershell
.\Scripts\Build.ps1 -Target Editor
.\Scripts\Build.ps1 -Target Game
```

엔진 경로는 `-EngineRoot` 또는 `UE_ENGINE_ROOT` 환경 변수로 지정할 수 있습니다. 값을 지정하지 않으면 UE 레지스트리와 기본 설치 경로를 확인합니다. Visual Studio/Rider 프로젝트 파일은 생성 산출물이므로 저장소에 포함하지 않습니다.

다른 UE 5.8 프로젝트에서 사용할 때는 `Plugins/Kata`를 복사하고 Kata를 활성화합니다. 프로젝트 C++ 코드에서 공개 API를 참조하려면 해당 게임 모듈의 Build.cs에 사용하는 Kata 모듈 의존성을 추가합니다.
AKataCharacter를 사용하면 KataFramework도, 팩션 설정·판정이 필요하면 KataTargeting도 함께 추가합니다.
샘플의 태그 생성 단계와 기존 에셋 Redirect는 프로젝트별 설정입니다. 자세한 내용은
[게임플레이 태그](docs/manual/Gameplay-Tags.md)와 [이전 안내](docs/manual/Asset-Migration.md)를 참고합니다.

## 문서

- [문서 목록](docs/README.md): Manual, Devlog, Plan 분류.
- [공용 에이전트 지침](AGENTS.md): Claude와 Codex가 함께 따르는 작업 규칙.
- [GitHub 이슈](https://github.com/jaykop/Kata/issues): 진행 중인 작업과 미확정 기능.
