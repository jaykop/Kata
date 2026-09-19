# Kata 공용 작업 지침

이 파일은 Codex와 Claude를 포함한 모든 코딩 에이전트의 공용 지침이다. `CLAUDE.md`에는 이 문서의 참조만 유지한다. 지침의 변경은 이 파일에서 수행한다.

## 프로젝트와 현재 범위

- Unreal Engine **5.8**, C++ 프로젝트 `ProjectKata`, 플러그인 **Kata**.
- **싱글플레이 전용이며 GAS가 필수**다. 복제, RPC, 클라이언트 예측, NetScope를 추가하지 않는다.
- 플러그인 코드는 `Plugins/Kata/Source`에, 프로젝트별 실험·샘플은 `Source/ProjectKata` 및 프로젝트 `Content`에 둔다.
- 현재 모듈은 `KataConditions`, `KataRuntime`, `KataEditor` 세 개다.
- **KataAI는 현재 범위에서 제외**한다. BT/StateTree 모듈 의존성과 어댑터를 먼저 추가하지 않는다.
- 현재 구현은 모듈 골격이다. 타임라인 UI, 실행기, 조건 객체, 콤보, 프리뷰는 별도 후속 구현이며 이미 제공되는 기능처럼 문서화하지 않는다.

## 설계 판단의 우선순위

1. 사용자의 현재 요청과 명시적으로 확정한 결정.
2. 이 파일과 `docs/Implementation-Status.md`에 정리된 현재 구조·구현 상태.
3. `docs/Kata-Plugin-Design.md` 및 `docs/Kata-Plugin-Design-Codex.md`의 설계 제안.

두 설계 문서는 검토 자료이며 모든 제안이 승인된 사양은 아니다. 특히 기존 문서의 네트워크, 선택형 GAS, KataAI, 콤보 전이 소유권 관련 내용은 현재 범위와 다르거나 아직 논의 중이다. 기술 변경 시 실제 구현 상태 문서를 함께 갱신한다.

## 모듈 경계

- `KataConditions`: 공용 조건·Context를 위한 런타임 계층. GAS를 사용할 수 있지만 KataRuntime과 에디터에 의존하지 않는다.
- `KataRuntime`: 액션·콤보·태스크·실행기·GAS 연결을 위한 런타임 계층. KataConditions에 의존한다.
- `KataEditor`: 편집기·타임라인·그래프·프리뷰를 위한 Editor 전용 계층. 런타임 계층을 참조한다.
- 의존 방향을 역전시키거나 순환 의존을 만들지 않는다. UnrealEd, AssetTools, Slate 편집 기능 등 에디터 전용 코드를 런타임에 넣지 않는다.
- Public 헤더에서 필요한 의존성만 Public으로 노출하고 구현 전용 의존성은 Private에 둔다. 신규 의존성과 플러그인은 실제 기능에 필요할 때 추가한다.

## 구현 원칙

- Unreal 명명 규칙과 타입 접두사 U/A/F/E/I/S를 사용하고 공개 심볼에는 Kata 접두사를 사용한다.
- 에셋의 설정과 캐릭터별 실행 상태를 분리한다. 공유 에셋·조건 객체에 실행 중 시간, 대상 기록, 이펙트 핸들을 저장하지 않는다.
- 조건 평가는 부작용 없이 수행한다. Attribute 비용 지불, Gameplay Effect 적용, 입력 소비는 실행 처리에서 수행한다.
- GAS의 비용·쿨다운·Attribute 시스템을 중복 구현하지 않는다.
- 실행 태스크는 완료·취소·중단·소유자 파괴 시 획득한 자원과 이벤트 구독을 정리하도록 설계한다.
- C++ virtual과 Blueprint 확장 지점을 구분한다. UObject 참조 수명과 GC 추적을 검토한다.
- 에셋 경로·모듈명·반영 타입명을 변경할 때 직렬화된 에셋과 필요한 Redirect를 확인한다.
- 엔진 소스나 생성 파일을 수정해 프로젝트 문제를 우회하지 않는다.

## 함께 작업할 때

- 시작 전에 `git status --short`와 관련 파일의 현재 내용을 확인한다. 다른 작업자의 미커밋 변경을 덮어쓰거나 되돌리지 않는다.
- 요청 범위에 필요한 변경만 수행하고, 같은 파일을 변경하는 작업이 발견되면 현재 상태를 확인한 뒤 이어서 작업한다.
- 문서와 완료 보고는 기본적으로 한국어로 작성한다. 코드 식별자는 영어를 사용한다.
- 사용자 요청에 없는 기능이나 미확정 설계 결정을 구현으로 확정하지 않는다.
- 커밋에는 변경 이유와 검증 결과를 추적할 수 있는 설명을 사용한다. 공개 저장소에는 비밀 값과 개인 로컬 설정을 추가하지 않는다.

## 빌드와 검증

프로젝트 루트에서 PowerShell로 실행한다. 엔진 경로가 자동 탐색되지 않으면 `-EngineRoot`로 엔진 설치 루트를 지정한다.

```powershell
.\Scripts\Build.ps1 -Target Editor
.\Scripts\Build.ps1 -Target Game
# 예: .\Scripts\Build.ps1 -Target Editor -EngineRoot 'D:\Epic Games\UE_5.8'
```

- 모듈·Build.cs·플러그인 구조 변경 후 Editor와 Game Development 빌드를 확인한다. Game 빌드로 에디터 의존성 유출도 확인한다.
- 동작 변경에는 관련 기능을 확인하는 테스트나 재현 절차를 적용한다. 골격·문서 변경만을 위한 의미 없는 테스트는 추가하지 않는다.
- 실행 중인 에디터가 DLL을 잠그거나 Live Coding이 빌드를 막으면 사용자 작업을 강제로 종료하지 않는다. 원인과 검증 한계를 보고한다.
- 빌드 성공과 에디터 실행·패키징·게임 동작 검증을 구분한다. 실행하지 않은 검증을 성공했다고 보고하지 않는다.
- `Binaries`, `Intermediate`, `Saved`, DDC, 생성 솔루션 파일은 커밋하지 않는다. `.uasset`·`.umap`을 추가할 때 Git LFS 설정을 사용한다.

## 구현 상태 기록

작업 완료 시 `docs/Implementation-Status.md`에서 구현 범위, 주요 결정, 확인한 검증, 남은 제한을 실제 상태와 맞춘다. 사용자의 새로운 결정이 이 지침과 충돌하면 지침과 상태 문서도 함께 수정한다.
