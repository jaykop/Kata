---
# advisors 프로필 — Kata. 양식: jaykop/claude-plugins 의 plugins/advisors/shared/profile-template.md
# 값 옆 주석에 출처를 적는다: [사용자] 사용자가 확정 / [파일] 프로젝트 파일에서 읽음 / [추정] 고문이 추정, 확인 필요
# 이 파일은 Git·Diversion 에서 추적한다. 하루 작업 시간 같은 개인 값은 pm.capacity_file 이 가리키는 추적하지 않는 파일에 둔다.
profile_version: 1
updated: 2026-10-15

# ── 공통 ─────────────────────────────────────────────
project_name: Kata                      # [파일] AGENTS.md
genre:                                  # 미정. Kata 는 장르를 정하지 않은 범용 기반이고 게임은 샘플에서 만든다
play_mode: 싱글                          # [파일] AGENTS.md 프로젝트와 현재 범위
ip_based: false
platforms: [Win64]                      # [사용자] 2026-10-14
target_spec:                            # [사용자] 모름
regions: []
business_model:
release_window:
dev_stage: 프리프로덕션                  # [사용자] 2026-10-14
next_milestone: M1 전투 샌드박스 2026-11-15   # [파일] GitHub Milestone (2026-10-14 조회). PM 고문은 트래커 마일스톤을 우선 쓴다
team_size: 1                            # [파일] .claude/workflow/profile.local.md user_role "1인 개발"
feature_cutoff:                         # [사용자] 2026-10-14 마감 없음

engine:
  name: Unreal Engine
  version: 5.8.3                        # [파일] C:/Program Files/Epic Games/UE_5.8/Engine/Build/Build.version
  source_build: false                   # [파일] EngineAssociation '5.8' (런처판)
  root: C:/Program Files/Epic Games/UE_5.8   # [파일]

source_roots: [Plugins/*/Source, Source/ProjectKata, Source/ProjectKataEditor]   # [파일] .claude/workflow/profile.md
project_file: ProjectKata.uproject      # [파일]

vcs: git                                # [파일]
vcs_read_commands: [git rev-parse, git log, git show]   # [사용자] 2026-10-14

decision_owners:
  design: 사용자                        # [사용자] 2026-10-14
  business:
  tech: 사용자                          # [사용자] 2026-10-14
  schedule: 사용자                      # [사용자] 2026-10-14 1인 개발
user_role: 1인 개발                       # [파일] .claude/workflow/profile.local.md

report_dir: .claude/advisors/reports    # Git에서 제외하고 Diversion에서만 추적한다
share_scope: personal
stale_after_days: 90
code_change_entry: workflow:feature-impl   # [파일] AGENTS.md AI 작업 플러그인
search_banned_terms: []                 # [사용자] 2026-10-14 없음
search_banned_combos: []                # [사용자] 2026-10-14 없음

# ── 기획 고문 ─────────────────────────────────────────
design:
  roots: [docs/design/]                 # [사용자] 2026-10-14. Git 제외, Diversion 추적
  exclude: []                           # [사용자] 2026-10-14 제외 없음
  summaries:
  principles:                           # [사용자] 아직 안 정함 — G1 장르 결정(#81)에서 함께 정한다
  principles_status:
  playtest_records: [docs/design/playtest/]   # [사용자] Git 제외, Diversion 추적
  data_paths: []
  data_status: 임시값                   # [사용자] 2026-10-14
  principles_confirmed:
  archive_dirs: []
  deprecated_marker: []
  glossary:
  area_file_patterns: {}
  existing_tools: []
  knowledge_dir:

# ── 기술 동향 고문 ─────────────────────────────────────────
radar:
  engine_mod_marker:                    # [파일] 런처판 엔진(engine.source_build false)이라 엔진 수정 블록 없음
  engine_mod_end_marker:
  engine_mod_tag_re:
  engine_mod_dirs: []
  interests: []
  excluded: []
  ai_tool_policy:
  community_sources:
    auto: [qiita, geeknews, tencent-gameinstitute]
    manual: [reddit, zhihu]
  knowledge_dir:

# ── 시장 동향 고문 ─────────────────────────────────────
market:
  competitors_seed: []
  target_player:
  paid_data_files: []
  price_band:
  check_interval: quarterly
  knowledge_dir:

# ── 최적화 고문 ───────────────────────────────────────
perf:
  budgets: {}                           # [사용자] 아직 안 정함 — 장르·플랫폼 결정 뒤
  scenarios:                            # [사용자] 2026-10-14
    - {id: S01, name: "LV_CombatSandbox 흑기사 1:1과 잡몹 소수 전투 (M1)"}
  profiling_dir: Saved/Profiling        # [사용자] 엔진 기본 출력 위치. 2026-10-14 기준 폴더 없음
  measured_by: 사용자                     # [파일] AGENTS.md 테스트·검사 담당 원칙
  test_command: "UnrealEditor-Cmd.exe ProjectKata.uproject -unattended -nullrhi -nosplash -nosound -nop4 '-ExecCmds=Automation RunTests Kata' '-TestExit=Automation Test Queue Empty'"   # [파일] docs/manual/Conditions.md headless 실행 명령을 Kata 전체로 넓힘. 2026-10-14 사용자 확인
  test_convention: "대상 모듈 Private/Tests/, WITH_DEV_AUTOMATION_TESTS, IMPLEMENT_SIMPLE_AUTOMATION_TEST, 플래그 EditorContext|EngineFilter, 이름 Kata.<모듈 주제>.<대상>.<경우>"   # [파일] KataConditions/Private/Tests/KataConditionTests.cpp:6,25,97. 2026-10-14 사용자 확인
  edit_scope: tests_only
  knowledge_dir:

# ── PM 고문 ───────────────────────────────────────────
pm:
  knowledge_dir: docs/localdocs/pm      # [사용자] 강의 요약 노트. Git 제외, Diversion 추적
  tracker: github                       # [파일] AGENTS.md 작업 추적
  tracker_repo: jaykop/Kata             # [파일]
  size_hours: {"size: S": 1, "size: M": 2, "size: L": 5.5}   # [파일] AGENTS.md 규모 합산(칸의 가운데 값)
  must_label: "priority: must"          # [파일]
  blocked_label: "status: blocked"      # [파일]
  progress_label: "status: in-progress" # [파일]
  order_labels: ["grade: A1", "grade: A2", "grade: B1", "grade: B2", "grade: C1", "grade: C2"]   # [파일] AGENTS.md 등급
  next_task_entry: workflow:next-task   # [파일] AGENTS.md AI 작업 플러그인
  capacity: {plan_ratio: 0.7}           # [사용자] 계획은 가능 시간의 70%
  capacity_file: .claude/workflow/profile.local.md   # [파일] 평일·휴일 하루 작업 시간(추적하지 않는 개인 값)
  capacity_keys: {weekday_hours: time_budget_weekday, holiday_hours: time_budget_holiday}
  holidays: []                          # 비어 있으면 공휴일을 평일로 센다
  must_ratio_limit: 0.6                 # [파일] AGENTS.md 작업 추적
  week_start: 월

# ── TD 고문 ─────────────────────────────────────────
td:
  rule_docs: [AGENTS.md]                # [사용자] 2026-10-15 TD 고문 첫 실행에서 확인. 루트 CLAUDE.md 는 AGENTS.md 참조뿐이라 넣지 않음
  record_roots: [docs/devlog, docs/plan]   # [추정] 결정 이유는 devlog, 설계안은 plan. TD 고문 첫 실행 때 확인
  extra_roots: []
  vanilla_engine_root: C:/Program Files/Epic Games/UE_5.8   # [파일] 런처판이라 engine.root 자체가 순정 엔진
  engine_base_ref:                      # 엔진 소스를 버전 관리하지 않는다
  knowledge_dir:
---

# 프로필 메모

- Kata 는 장르 미정의 범용 기반이다. 게임 기획·플레이테스트 대상은 `Content/KataSample` 의 샘플 게임이다.
- 이슈 순서의 원문 규칙은 `AGENTS.md` 작업 추적과 workflow 플러그인의 `issue_priority.py` 다. PM 고문의 라벨 순서는 초안이고, 선행 관계까지 반영한 순서는 `workflow:next-task` 를 따른다.
- 빌드·테스트·측정은 사용자가 직접 한다. 고문은 계획과 점검 항목까지만 낸다.
- 비어 있는 결정권자(`decision_owners`)와 `vcs_read_commands` 는 고문을 처음 부를 때 확인한다.
