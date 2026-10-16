# advisors 고문 적용과 PM 고문

작성: 2026-10-14  
갱신: 2026-10-17  
유형: 결정·구현 기록  
대상: `.claude/` 플러그인 지정과 advisors 프로필, AGENTS.md "AI 작업 플러그인"·"작업 추적", 이슈 우선순위 manual, PM 기준 노트 위치  
기준: 12d3085 이후 작업 트리(미커밋). 플러그인은 [jaykop/claude-plugins@ae3ed66](https://github.com/jaykop/claude-plugins/commit/ae3ed66)

## 배경과 결론

사용자가 공용 마켓플레이스 `jaykop/claude-plugins`를 Kata에 적용할 수 있는지 물었다. 그리고 일정을 관리할 PM·PD 에이전트를 만들고, 그 판단 기준으로 사용자가 보관한 게임 제작 수업 자료를 쓰고 싶다고 했다.

`workflow`·`ko-review`는 [이전 전환](2026-10-13-Shared-Agent-Plugins.md)에서 이미 적용돼 있었고, 같은 마켓플레이스의 `advisors`(기획·신기술·시장·최적화 고문)만 꺼져 있었다. 이번에 `advisors`를 켰다. 그리고 다섯 번째 고문 `pm`을 플러그인에 공용판으로 만들었다. 수업 자료 요약은 공개하지 않는 기준 노트로 Kata에 두었다. 다음 작업을 묻는 요청은 PM 고문이 먼저 받도록 지침을 바꿨다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| advisors 플러그인 | 마켓플레이스에 있으나 Kata에서 꺼짐 | `.claude/settings.json`에 `advisors@jaykop` 추가 |
| advisors 프로필 | 없음 | `.claude/advisors/profile.md`. 프로젝트 파일로 확인한 값만 채우고, 결정권자·버전 관리 읽기 명령은 고문을 처음 부를 때 확인한다 |
| `.claude/` 추적 | advisors 폴더 전체가 `.claude/*`에 걸려 제외 | `.gitignore`·`.dvignore`에서 `profile.md`만 예외로 추적. 리포트(`.claude/advisors/reports/`)는 양쪽 모두 제외 |
| PM 고문 | 없음 | 플러그인 `advisors/skills/pm`: 다음 작업, 주간 점검, 마일스톤 계획·회고, 범위 판단, 플레이테스트 계획. 정기 점검표·위험 목록·마일스톤 계획 기록과 계산 스크립트 `pm_calc.py` |
| 기준 노트 | 수업 자료가 Google Drive에만 있음 | `docs/localdocs/pm/`(Git 제외, Diversion 추적)에 요약 노트 3개. 프로필 `pm.knowledge_dir`가 가리킨다 |
| 다음 작업 진입점 | "다음 뭐 해"는 `next-task` | PM 고문이 먼저 받고, 이슈를 추천하면 `next-task`로 넘긴다(AGENTS.md "작업 추적", [이슈 우선순위 사용법](../manual/Issue-Priority.md)) |
| workflow 프로필 대응표 | `.claude/advisors/` 없음 | 문서 대응표에 `.claude/advisors/profile.md` 추가 |

## 주요 결정과 이유

- **PM을 advisors의 고문으로 만든다**: 프로필, 리포트 폴더, 고문 사이 질문함, 예약 점검, 세션 시작 알림 구조를 그대로 쓸 수 있다. "판단과 제안까지만 하고 결정은 사람이 한다"는 고문 원칙도 일정 관리에 맞다. workflow 플러그인에 스킬로 넣는 안은 리포트 비교와 예약 점검 구조를 새로 만들어야 해서, Kata 전용 에이전트로 두는 안은 다른 프로젝트에서 쓸 수 없어서 고르지 않았다(사용자 선택).
- **next-task와 역할을 나눈다**: PM 고문은 이슈, 때가 된 고문 점검, 결정 시점이 다가온 위험, 밀린 회고를 함께 보고 하나를 추천한다. Blocked by까지 반영한 이슈 순서와 이슈 게시는 `next-task`가 계속 맡는다. PM 고문의 이슈 순서는 라벨만 본 초안이다.
- **"다음 뭐 해"는 PM 고문이 먼저 받는다**: 모델이 스킬 설명 문구를 보고 스킬을 고르므로 두 스킬이 같은 요청에 맞으면 어느 쪽이 불릴지 정해지지 않는다. 사용자가 PM 먼저를 골랐고(2026-10-14), 이를 AGENTS.md 규칙으로 고정했다. PM 고문은 실행할 때마다 리포트 파일을 남기므로 `next-task`만 쓰던 때보다 무겁다.
- **다른 고문의 실행 관리**: 성능 주기 점검, 리팩터링 판단, 신기술·시장 점검, 플레이테스트 주기를 정기 점검표 하나로 모은다. 때가 되면 알리고, 사용자가 동의할 때만 그 고문을 부르거나 예약을 만든다. 무인 실행에서는 다른 고문을 부르지 않는다.
- **수업 자료는 공개하지 않는다**: 요약 노트는 Diversion에서만 추적하고 공개 문서에서 링크하지 않는다. 공용 플러그인의 판단 방법 문서는 특정 교재의 예시와 수치 없이 일반 표현으로 쓴다. 첫 판(21d09a0)에 남았던 강의 예시와 세부 수치는 f5222b0에서 일반 표현으로 바꿨다.
- **개인 값은 추적하지 않는다**: 하루 작업 시간은 이미 있는 `.claude/workflow/profile.local.md`를 `pm.capacity_file`로 가리켜 읽고, advisors 프로필에는 옮겨 적지 않는다. 계획 비율 70%만 프로필에 둔다.
- **용량 기준은 마일스톤 설명의 용량 줄이다**: Kata 마일스톤 설명에는 `용량: N시간` 줄이 있고 `issue_priority.py`도 이 값을 쓴다. PM 고문이 일별 시간으로만 계산하면 다른 비율을 내므로, 필수 비율은 이 줄로 판정하고 남은 기간의 일별 계산값은 함께 적게 했다(ae3ed66).

## 근거

- [AGENTS.md](../../AGENTS.md) "AI 작업 플러그인", "작업 추적".
- [advisors 플러그인](https://github.com/jaykop/claude-plugins/tree/main/plugins/advisors): `skills/pm/SKILL.md`, `reference/`, `scripts/pm_calc.py`, 공용 규칙과 프로필 양식.
- [이슈 우선순위 확인 도구 사용법](../manual/Issue-Priority.md): 다음 작업 진입점.
- 이전 결정: [AI 작업 스킬의 공용 플러그인 전환](2026-10-13-Shared-Agent-Plugins.md), [마일스톤과 우선순위 체계](2026-10-12-Milestone-Priority-System.md).

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 프로필의 `[파일]` 값 | 플러그인의 `profile_scan.py` 읽기 전용 실행, 엔진 `Build.version`, GitHub 마일스톤 조회, 에이전트 | 엔진 5.8.3 런처판, git, 측정 폴더 없음, 마일스톤 3개와 용량 줄 확인 | - |
| PM 고문 스크립트 `pm_calc.py` | 실행하지 않음 | - | 네 하위 명령 전부 |
| 플러그인 구조 | 실행하지 않음 | - | `claude plugin validate`, Claude Code 설치와 스킬 호출 |
| 다음 작업 진입점 | 실행하지 않음 | - | 새 세션에서 "다음 뭐 해"가 PM 고문을 부르는지 |

## 남은 제한과 후속 작업

- `/advisors:pm 주간 점검`을 대화 실행으로 한 번 돌려 결정권자, `vcs_read_commands`, 정기 점검표 주기를 확정한 뒤에 주간 예약을 만든다.
- 프로필 `pm.holidays`가 비어 있어 공휴일을 평일로 센다.
- M1·M2 마일스톤 설명의 "순서" 줄이 옮기기 전 도구 경로(`Scripts/IssuePriority.py`)를 가리키고 있었다. 같은 날 사용자 승인을 받아 `next-task`(`issue_priority.py`)로 고쳤다. 목표·완료 기준·용량 줄은 바꾸지 않았다.
- advisors는 Claude Code 전용이다. Codex에서는 PM 고문을 쓸 수 없고 `next-task`를 그대로 쓴다.
- 진입점은 지침과 스킬 설명에 기대므로 다른 스킬이 불릴 수 있다. 확실히 PM 고문을 부르려면 `/advisors:pm 다음 작업`으로 직접 부른다.
- 2026-10-17에 리포트 폴더의 추적 범위를 바꿨다. 위험 목록과 기술 부채 장부 같은 누적 기록을 잃지 않도록, `.claude/advisors/reports/`를 Diversion에서만 추적한다. Git에서는 계속 제외한다. 위 표의 "양쪽 모두 제외"는 당시 판단이다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| 연결 이슈 | 없음. 도구 설정 작업이라 이슈 없이 진행했다 |
| [이슈 우선순위 확인 도구](../manual/Issue-Priority.md) | 다음 작업 요청의 진입점을 PM 고문으로 바꾸고, 범위 결정은 `next-task`로 남겼다 |
| [AI 작업 스킬의 공용 플러그인 전환](2026-10-13-Shared-Agent-Plugins.md) | 당시 판단 보존. 영향 없음 |
