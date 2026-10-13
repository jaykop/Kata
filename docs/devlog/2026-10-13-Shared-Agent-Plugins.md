# AI 작업 스킬의 공용 플러그인 전환

작성: 2026-10-13  
갱신: 2026-10-13  
유형: 결정·구현 기록  
대상: Claude·Codex 작업 스킬과 훅, `.claude/` 추적 범위, 설명 변경 검토·이슈 우선순위 도구  
기준: b95edd5 이후 작업 트리(미커밋). 플러그인은 [jaykop/claude-plugins@94d8308](https://github.com/jaykop/claude-plugins/commit/94d8308)

## 배경과 결론

Kata의 작업 스킬과 훅은 이 PC의 `.claude/`·`.agents/`·`.codex/`에만 있었고 저장소에서 제외돼 있었다. 사용자가 다른 머신에서 저장소를 clone해도 같은 세팅을 쓰고 싶다고 했고, 이미 공용 플러그인을 올리던 공개 마켓플레이스 `jaykop/claude-plugins`에 함께 올리기로 했다.

진단 결과에 따라 한국어 검수와 작업 흐름 스킬을 프로젝트 값과 분리해 일반화했다. 그 결과를 `ko-review`·`workflow` 두 플러그인으로 올리고, Kata에는 플러그인 지정과 프로필만 남겼다. 외부에서 가져온 스킬은 원본 저장소에서 설치하고, 개인 지침의 게임 팀 제작 스킬은 올리지 않고 로컬에서도 지웠다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 작업 흐름 스킬 | Kata 전용 로컬 스킬(feature-impl, feature-doc-sync, commit-doc-review, context-handoff, next-task)과 훅 2개 | `workflow` 플러그인. 경로·트래커·문서 폴더·빌드 명령·대응표는 `.claude/workflow/profile.md`에서 읽는다 |
| 한국어 검수 | Claude의 doc-polish, Codex 전용 description-delta-review·editor-comment-polish | `ko-review` 플러그인의 `polish`·`record`·`review`. 언리얼 에디터 주석 기준은 참고 파일로 분리 |
| 도구 스크립트 | `Scripts/DescriptionReview.py`, `Scripts/IssuePriority.py` | 플러그인의 `review_queue.py`, `issue_priority.py`로 옮기고 Kata 사본 삭제. 목록 형식(schema 1)은 그대로 |
| Codex 연결 | `.agents/skills`의 진입 파일이 `.claude/skills` 원본을 읽음 | 같은 플러그인에 `.codex-plugin/plugin.json`을 두고 Codex 마켓플레이스로 설치 |
| `.claude/` 추적 | 디렉터리 전체를 Git·Diversion에서 제외 | `settings.json`, 두 플러그인의 공유 프로필, `terms.json`만 추적. 개인 값은 `profile.local.md`(추적 안 함) |
| 외부 스킬 | grammar-checker·humanizer·style-guide·graphify 로컬 사본 | korean-skills는 원본 플러그인으로 설치. graphify는 원본이 Python 도구라 로컬 사본 유지 |
| AGENTS.md | 로컬 스킬 이름과 `Scripts/` 도구 경로 | "AI 작업 플러그인" 단락 추가, 스킬 이름·도구 경로 갱신 |

## 주요 결정과 이유

- **프로필 분리**: 같은 리포의 advisors 플러그인처럼 프로젝트 값은 각 프로젝트의 `.claude/<플러그인>/profile.md`에서 읽는다. 스킬은 규칙을 복사하지 않고 프로필이 가리키는 AGENTS.md 위치를 매번 읽는다. 공개 저장소에 올라가면 안 되는 하루 작업 가능 시간은 추적하지 않는 `profile.local.md`로 나눴다.
- **훅은 프로필로 켠 프로젝트에서만**: 플러그인 훅은 플러그인을 켠 모든 프로젝트에 적용된다. 커밋 차단 훅을 그대로 옮기면 다른 프로젝트의 커밋까지 막으므로, 프로필의 `commit_gate: true`일 때만 동작하게 했다.
- **Claude와 Codex가 같은 스킬 폴더를 쓴다**: 플러그인 폴더에 두 매니페스트를 두면 진입 파일을 따로 유지할 필요가 없다. Codex는 매니페스트의 `version`을 설치 폴더 이름으로 쓰므로, 스킬을 고친 뒤에는 이 값을 올려야 반영된다. Codex 훅은 버전마다 바뀌는 Codex 사본 대신 Claude 마켓플레이스 사본의 고정 경로를 가리킨다.
- **외부 스킬은 복사하지 않는다**: grammar-checker·humanizer·style-guide는 DaleSeo의 MIT 스킬이고 원본에 Claude 마켓플레이스가 있다. graphify는 Graphify-Labs의 Apache-2.0 도구다. 공개 리포에 사본을 두면 출처 관리가 어려워지므로 원본에서 설치한다.
- **회사 파생 내용 제거**: 작업 흐름 스킬은 회사 프로젝트용 비공개 파이프라인에서 시작했다. 회사 정보와 Perforce·Jira·Confluence 전제는 Kata로 옮길 때 이미 뺐고, 공개판에서는 Kata 고유 값도 프로필로 바꿨다.
- **feature-impl의 서브에이전트**: 탐색만 서브에이전트에 맡기고 계획·구현은 메인 세션이 직접 한다. 계획은 대화에서 확정한 결정과 지침 맥락이 품질을 좌우하고, 언리얼 C++ 파일을 따로 구현하면 선언·구현·include가 어긋나 사용자 빌드 시간이 늘어나기 때문이다. 병렬 구현은 서로 참조하지 않는 파일 그룹에만 쓴다.
- **금지어 조정**: `doc_check.py`의 기본 목록은 어느 프로젝트에나 맞는 낱말만 남겼다. "계약"은 Kata manual이 "사용 계약"을 용어로 쓰고 있어 확정 문제에서 확인 필요로 내렸다. 카메라 셰이더 분기 이름이나 탄 발사 표현처럼 Kata 문서에서만 걸리던 항목 5개는 `.claude/ko-review/terms.json`으로 옮겨 Kata에서는 이전과 같게 검사한다.
- **게임 팀 제작 스킬 제외**: 사용자 전역 지침의 game-studio 스킬과 역할 에이전트는 서브에이전트 사용을 강제하려고 만든 것이라 올리지 않았다. 이 변경은 Kata 밖의 개인 설정이라 여기서는 판단만 기록한다.

## 근거

- [ko-review 플러그인](https://github.com/jaykop/claude-plugins/tree/main/plugins/ko-review), [workflow 플러그인](https://github.com/jaykop/claude-plugins/tree/main/plugins/workflow): 스킬, 스크립트, 프로필 양식.
- [AGENTS.md](../../AGENTS.md) "AI 작업 플러그인", "설명 변경의 공유 검토", "작업 추적".
- [설명 변경 검토 사용법](../manual/Description-Review.md), [이슈 우선순위 확인 도구 사용법](../manual/Issue-Priority.md).
- 이전 결정: [설명 검토 파이프라인](2026-10-12-Description-Review-Pipeline.md), [마일스톤과 우선순위 체계](2026-10-12-Milestone-Priority-System.md).

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 플러그인 구조 | `claude plugin validate`, 에이전트 | 마켓플레이스와 두 플러그인 통과. 경고는 의도한 `version` 미기재뿐 | - |
| Claude Code 설치 | 사용자 요청으로 에이전트가 프로젝트 범위 설치 | `claude plugin details`에서 스킬 5·3·3개와 훅 2개 확인. 설치 뒤 세션의 스킬 목록에 플러그인 스킬이 나타남 | 새 세션에서의 스킬 호출 |
| Codex 설치 | 에이전트가 마켓플레이스 추가, app-server `plugin/install` 요청 | `ko-review`·`workflow` 설치·활성 확인 | Codex 세션에서의 스킬 호출, 훅 신뢰 승인 |
| 커밋 차단 훅 | 샘플 입력으로 스크립트 실행, 에이전트 | Kata에서 트레일러 없으면 차단, 있으면 통과, 프로필 없는 프로젝트는 무시. Codex 조건(환경 변수 없음)도 같음 | 실제 세션의 커밋 |
| 설명 변경 목록 호환 | 이전 도구로 시작한 목록을 플러그인 도구로 capture·register | 정상 기록, 미등록 파일 없음 | Codex 쪽 begin·finish |
| `.gitignore` 예외 | `git check-ignore`, 에이전트 | 공유 파일만 추적, `profile.local.md`와 나머지는 제외 | `.dvignore` 예외는 Diversion 커밋 결과로만 확인 |

빌드는 소스 변경이 없어 해당하지 않는다. `issue_priority.py`는 실행하지 않았다.

같은 날 커밋(b34a273) 뒤 사용자가 표의 미확인 범위를 확인했다. Claude Code 새 세션에서 workflow 스킬을 불렀고, 트레일러 없는 커밋 명령(`--dry-run`)이 커밋 문서 훅에 막혔다. Codex에서는 훅을 승인한 뒤 ko-review `review`로 대기 항목 5개를 검토해 begin·finish로 기록했다. 이 검토에서 `begin`이 프로필의 검토자 이름을 읽지 않는 점이 드러나 ko-review 0.1.1에서 프로필 값을 기본으로 쓰게 했다. 0.1.1의 이 동작은 다음 Codex 검토에서 확인한다.

## 남은 제한과 후속 작업

- graphify는 원본 설치(`uv tool install graphifyy`, `graphify install`)를 사용자가 정하면 로컬 사본을 지운다.
- 플러그인 README의 Codex 안내는 실제 절차와 `version` 갱신 규칙으로 고쳤다(jaykop/claude-plugins@bc47fdb). Codex는 `codex plugin marketplace upgrade jaykop`만으로 설치된 사본도 새 버전으로 바뀐다.
- 커밋 문서 훅은 명령 문자열 전체에서 커밋 명령을 찾으므로, 다른 명령의 인자나 문자열에 그 낱말이 들어 있어도 막힌다. 이때는 내용을 파일로 옮겨 실행한다.
- 예전 로컬 스킬·훅은 `~/.claude/backups/`의 이 PC 백업에만 있다. 공개 저장소나 Diversion에는 없다.
- 이전 작업(#64, 설명 검토 파이프라인)의 설명 변경 목록은 삭제한 `Scripts/*.py`를 근거로 가리킨다. 다시 검토하면 위치 오류로 대기가 된다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [연결 이슈 #94](https://github.com/jaykop/Kata/issues/94) | 구현 완료, 사용자 확인 전. 결과 댓글과 `status: needs-verification`은 사용자 확인 뒤 게시 |
| [설명 변경 검토](../manual/Description-Review.md) | 플러그인 기준으로 다시 작성 |
| [이슈 우선순위 확인 도구](../manual/Issue-Priority.md) | 플러그인 기준으로 다시 작성 |
| 이전 devlog 2개 | 당시 판단은 보존하고 후속 줄과 도구 링크만 갱신 |
