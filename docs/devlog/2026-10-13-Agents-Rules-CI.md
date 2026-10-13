# AGENTS.md 규칙 CI 검사 도입

작성: 2026-10-13  
갱신: 2026-10-13  
유형: 결정 기록, 구현 기록  
대상: `.github/workflows/agents-rules.yml`, `Scripts/CheckAgentsRules.py`, AGENTS.md "테스트·검사 담당 원칙"  
기준: 7069c2a 이후 작업 트리(미커밋). 다른 작업의 미커밋 변경(KataFramework HUD MVVM)이 있는 상태에서 대조·실행했다.

## 배경과 결론

저장소에 GitHub Actions를 처음 도입했다. 클라우드 에이전트(Claude·Copilot·Codex)가 PR을 올리는 경우를 대비해 안전장치를 둔다.
GitHub 호스트 러너에는 Unreal Engine이 없어 컴파일 CI를 할 수 없다. 그래서 AGENTS.md 규칙 가운데 빌드 없이 텍스트로 판정할 수 있는 것만 push·pull_request마다 검사한다.

결론은 다음과 같다.

- 규칙 8개를 검사하는 표준 라이브러리 전용 Python 스크립트와 읽기 전용 워크플로를 만들었다.
- AGENTS.md "테스트·검사 담당 원칙"에 CI가 자동으로 검사하는 범위와 에이전트의 로컬 실행 조건을 적었다.
- 사용법과 판정 한계는 [AGENTS.md 규칙 검사](../manual/Agents-Rules-Check.md)에 정리했다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| CI | 없음. `.github/`에는 이슈 템플릿만 있었다 | 워크플로 `AGENTS rules`. 모든 브랜치 push·pull_request·수동 실행, `contents: read`, ubuntu-latest |
| 규칙 검사 도구 | 없음 | `Scripts/CheckAgentsRules.py`. 규칙 8개, 위반 시 파일·줄·AGENTS.md 항목 출력과 Actions annotation, 종료 코드 0·1·2 |
| AGENTS.md | 테스트·검사는 모두 사용자 담당 | CI 자동 검사 범위, CI가 사용자 확인을 대신하지 않는다는 점, 에이전트의 로컬 실행 조건, 규칙 변경 시 갱신 위치를 추가 |
| 문서 | 없음 | manual 신규, docs/README.md 목록 추가 |

## 주요 결정과 이유

- **컴파일 CI와 셀프 호스트 러너를 쓰지 않는다.** 호스트 러너에는 엔진이 없다. 셀프 호스트 러너를 공개 저장소에 붙이면 fork PR의 코드가 러너 머신에서 실행될 수 있다.
- **텍스트 규칙부터 검사한다.** 엔진 없이도 결과가 결정적이고, 플러그인 경계 위반처럼 되돌리는 비용이 큰 실수를 PR 단계에서 드러낼 수 있다.
- **규칙 8개(2026-10-13 사용자 선택).** 처음 제안한 후보 다섯 개(코어 → 위성·통합 금지, 위성 간 의존 제한, 런타임 → 에디터 금지, 샘플 전용 플러그인 금지, 복제·RPC 금지)를 모두 채택했다. AGENTS.md 본문에서 찾은 세 개(코어 플러그인 허용 목록, GAS Inspector 참조 금지, 샘플 모듈 참조 금지)도 추가했다.
- **복제 검사 범위는 Kata 플러그인 여섯 개(KataGASInspector 포함)로 하고 벤더 WindowsDualsense_ds5w는 뺐다.** 벤더 코드는 원문을 유지하므로 참조 대상으로만 판정한다.
- **`if (Target.bBuildEditor)` 안의 에디터 의존도 런타임 모듈에서는 위반이다.** AGENTS.md는 에디터 전용 코드를 런타임 모듈에 넣지 않게 한다. 조건문을 해석하지 않고 문자열 리터럴로 판정하므로 결과가 단순하다. 현재 Runtime 모듈에는 해당 사례가 없다. UncookedOnly인 KataFrameworkAnimGraph는 대상이 아니다.
- **복제 선언은 단어 검색이 아니라 지정자 단위로 판정한다.** 단어로 검색하면 KataEditor 뷰포트의 `Client` 변수가 걸린다. 그래서 주석과 문자열을 지우고 `UPROPERTY`·`UFUNCTION` 괄호의 최상위 지정자만 본다.
- **플러그인 구성이 바뀌면 구성 오류(종료 코드 2)로 실패한다.** 계획에는 없었지만 구현 중에 추가했다. 새 `Kata*` 플러그인이 생겼는데 규칙 표를 갱신하지 않으면 검사 대상에서 조용히 빠지기 때문이다.
- **워크플로 보안.** `actions/checkout`을 v7.0.1의 커밋 SHA로 고정하고 `persist-credentials: false`, `contents: read`로 실행한다. `pull_request_target`은 쓰지 않는다.
- **필수 검사(브랜치 보호)로 지정하지 않는다.** 이슈 범위에서 제외했다. 필요하면 첫 실행을 확인한 뒤 따로 정한다.
- **채택하지 않은 대안.** 엔진 에디터 모듈의 전체 목록은 엔진 소스가 있어야 만들 수 있어서, 명시 목록과 이름 규칙으로 대신했다. `FPredictionKey` 같은 GAS 예측 API는 엔진 가상 함수의 시그니처에도 쓰여 텍스트로 구분할 수 없으므로 검사하지 않는다.

## 근거

- [AGENTS.md](../../AGENTS.md): "모듈 경계", "프로젝트와 현재 범위", "테스트·검사 담당 원칙".
- [플러그인 분리 결정](2026-10-10-Plugin-Modularization-Decisions.md): 코어·위성·통합 구성과 의존 방향.
- [검사 스크립트](../../Scripts/CheckAgentsRules.py), [워크플로](../../.github/workflows/agents-rules.yml).

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 현재 소스의 규칙 준수 | 2026-10-13 Claude가 .uplugin 7개와 Build.cs 13개를 읽고 대조 | 위반 없음 | — |
| 스크립트 실행 | 2026-10-13 사용자 요청으로 Claude가 로컬에서 실행 | 위반 없음, 종료 코드 0. .uplugin 6개, Build.cs 12개, C++ 456개, 규칙 8개. `--list-rules` 출력 | 로컬에서 위반을 넣은 실행(위반 검출은 Actions에서 확인) |
| GitHub Actions | 2026-10-13 main push와 임시 브랜치 push의 실행 기록 확인 | main(4b0843b·b95edd5·b34a273) 통과. 임시 브랜치(위반 2개 삽입, 병합하지 않고 삭제)는 종료 코드 1로 실패했고 `core-no-satellite`·`no-replication` annotation이 파일·줄에 표시됨 | PR Files changed 표시, fork PR 동작 |

## 남은 제한과 후속 작업

- PR의 Files changed 화면 표시와 fork PR 동작은 확인하지 않았다.
- 실행 기록에 `ubuntu-latest`가 2026-10-19부터 Ubuntu 26으로 바뀐다는 러너 안내가 있었다. 표준 라이브러리만 쓰므로 영향은 작다고 보지만 확인하지 않았다.
- 텍스트 판정의 한계(문자열 리터럴 대조, 에디터 모듈 목록, 전처리 미해석)는 [manual](../manual/Agents-Rules-Check.md)에 적었다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#93](https://github.com/jaykop/Kata/issues/93) | 결과 댓글 게시, 위반 검출 확인 뒤 닫음 |
| [AGENTS.md 규칙 검사](../manual/Agents-Rules-Check.md) | 신규 |
| [AGENTS.md](../../AGENTS.md) | "테스트·검사 담당 원칙"에 항목 추가(2026-10-13 사용자 확인) |
| [문서 목록](../README.md) | Manual·Devlog 항목 추가 |
| plan | 해당 없음 |
