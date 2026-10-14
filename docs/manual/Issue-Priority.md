# 이슈 우선순위 확인 도구 사용법

갱신: 2026-10-14  
대상: 이슈를 만들고 작업 순서를 정하는 사용자와 에이전트  
적용 기준: [workflow 플러그인](https://github.com/jaykop/claude-plugins/tree/main/plugins/workflow)(`next-task`, `issue_priority.py`), `.claude/workflow/profile.md`, AGENTS.md "작업 추적"  
확인 상태: 플러그인 도구는 아직 실행하지 않았다. 계산 규칙은 2026-10-12에 실행한 이전 Kata 도구와 같다.

## 목적과 준비

열린 이슈의 중요도·규모 라벨과 Blocked by 관계를 읽어, 마일스톤마다 지금 손댈 수 있는 이슈를 순서대로 보여 준다. 마일스톤 용량 대비 계획량과 규칙 위반도 함께 알려 준다.

계산은 workflow 플러그인의 `issue_priority.py`가 하고, 보통은 `next-task` 스킬이 실행한다("이거 이슈로 만들어", "우선순위 다시 매겨"). "다음 뭐 해" 같은 다음 작업 요청은 advisors PM 고문(`/advisors:pm 다음 작업`)이 먼저 받는다. PM 고문은 이슈와 함께 때가 된 고문 점검과 결정 시점이 다가온 위험을 보고 하나를 추천하며, 이슈를 추천하면 순서 확정과 오늘 범위를 `next-task`로 넘긴다. 설치는 AGENTS.md "AI 작업 플러그인"을 따른다. Python 3.8 이상과 로그인된 `gh` CLI가 필요하며, 프로젝트 루트에서 실행한다. 저장소는 `--repo`를 지정하면 그 값을 쓰고, 생략하면 프로필의 `tracker_repo`(`jaykop/Kata`), 현재 프로젝트의 `gh` 저장소 순으로 찾는다.

등급 규칙, 출력 읽는 법, 경고의 원문은 플러그인의 [등급 체계](https://github.com/jaykop/claude-plugins/blob/main/plugins/workflow/shared/priority-scheme.md)다. 이 문서는 Kata에서 쓰는 값과 절차만 적는다. 규칙이 겹치면 AGENTS.md "작업 추적"이 우선한다.

## 사용 순서

1. 이슈에 중요도 라벨(`priority: must`·`should`·`could`)과 규모 라벨(`size: S`·`M`·`L`·`XL`)을 붙이고, 선행 작업은 GitHub의 Blocked by로 연결한다. 종류(`type:`)와 대상 플러그인(`plugin:`) 라벨은 AGENTS.md를 따른다.
2. 마일스톤 설명에 `용량: N시간` 줄을 둔다. 없으면 계획량만 보이고 용량 경고는 나오지 않는다.
3. `next-task`로 순서를 본다. 스킬은 마일스톤별 순서표와 경고를 읽어 오늘 시간(개인 프로필의 평일·주말 시간)에 맞는 이슈를 고른다. PM 고문이 이슈를 추천한 뒤에도 이 단계로 범위를 정한다.
4. grade 라벨을 자체 등급에 맞출 때만 `--apply`를 쓴다. 공개 저장소의 라벨을 바꾸는 게시 작업이므로 에이전트는 사용자 확인을 받은 뒤 실행한다.

## 주요 설정과 실행 계약

| 항목 | Kata 값·의미 |
|---|---|
| 중요도 a | 이슈가 속한 마일스톤의 목표 기준. 백로그는 Kata 범용 기반 목표(프로필 `backlog_goal`) 기준 |
| 규모 b | 사용자가 직접 손대는 시간. S 1시간 이하, M 3시간 이하, L 8시간 이하, XL은 하위 이슈로 나눈다 |
| 자체·유효 등급 | a×b(A1~C2), 선행 작업은 자기가 막는 이슈의 가장 높은 등급을 물려받는다. grade 라벨은 자체 등급만 기록한다 |
| 하루 작업 시간 | `.claude/workflow/profile.local.md`의 `time_budget_weekday`·`time_budget_holiday`. 추적하지 않는 개인 파일이다 |
| 마일스톤 뷰 | 마일스톤을 시작할 때 저장소 Issues → Views의 "현재 마일스톤" 검색어(`milestone:"…"`)를 새 이름으로 바꾼다 |

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| `Could not detect the repository` | 프로필에서 `tracker_repo`를 얻지 못했고 `gh`의 저장소 감지도 실패했다 | `.claude/workflow/profile.md`의 `tracker_repo`를 확인하거나 `--repo jaykop/Kata`를 지정한다 |
| `gh … failed` | `gh` 로그인 만료, 권한 부족, 네트워크 오류 | `gh auth status`로 확인한다 |
| 머리줄에 `no capacity line` | 마일스톤 설명에 용량 줄이 없거나 용량 값이 0이다 | 마일스톤 설명에 양수인 `용량: N시간` 줄을 적는다 |
| Windows 콘솔에서 제목 글자가 깨진다 | 도구의 UTF-8 출력과 콘솔의 문자 해석이 맞지 않는다 | 도구는 UTF-8로 출력한다. 콘솔이나 출력을 읽는 프로그램의 인코딩을 UTF-8로 맞춘다 |

- 이슈 하나에서 라벨과 선행 이슈는 각각 50개까지만 읽는다. `--milestone`은 출력 범위만 줄이고 경고는 전체 열린 이슈를 기준으로 낸다.

## 확인 상태와 근거

- [workflow 플러그인](https://github.com/jaykop/claude-plugins/tree/main/plugins/workflow): `issue_priority.py`, `next-task` 스킬, 등급 체계 문서.
- [마일스톤과 우선순위 체계 결정](../devlog/2026-10-12-Milestone-Priority-System.md).
- [작업 상태](https://github.com/jaykop/Kata/issues/94). 처음 도입은 [#64](https://github.com/jaykop/Kata/issues/64).
- 2026-10-12 사용자 요청으로 이전 Kata 도구(`Scripts/IssuePriority.py`)를 읽기 전용으로 실행해 순서표, 용량, 대기 표시, 유효 등급 상속, 경고 출력을 확인했다(열린 이슈 45개, 경고 0건). `--apply`와 순환·라벨 누락 경고는 실행으로 확인하지 않았다.
- #94에서 도구를 플러그인으로 옮기며 저장소 감지(프로필 `tracker_repo` 우선)와 용량 줄 표기(`capacity: Nh`도 인정)만 바꿨다. 플러그인 도구는 아직 실행하지 않았다.
