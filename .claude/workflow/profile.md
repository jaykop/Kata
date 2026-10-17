---
# workflow 프로필 — Kata. 양식: jaykop/claude-plugins 의 plugins/workflow/shared/profile-template.md
# 개인 값(하루 작업 가능 시간)은 같은 폴더의 profile.local.md 에 둔다. 그 파일은 버전 관리에서 제외한다.
profile_version: 1
updated: 2026-10-13
project_name: Kata

# ── 지침과 범위 ─────────────────────────────────────
instructions: [AGENTS.md]
scope_sections: ["AGENTS.md 프로젝트와 현재 범위", "AGENTS.md 모듈 경계", "docs/devlog/2026-10-10-Plugin-Modularization-Decisions.md"]
coding_rules: ["AGENTS.md 구현 원칙", "이 프로필의 코딩 규칙 보충"]
source_roots: [Plugins/*/Source, Source/ProjectKata, Source/ProjectKataEditor]

# ── 이슈 트래커 ─────────────────────────────────────
tracker: github
tracker_repo: jaykop/Kata
tracker_public: true
issue_template: .github/ISSUE_TEMPLATE/work.md
status_labels:
  in_progress: "status: in-progress"
  needs_verification: "status: needs-verification"
  blocked: "status: blocked"
priority_scheme: grade        # 규칙 원문은 AGENTS.md 작업 추적
backlog_goal: Kata 범용 기반 목표
milestone_view_url: https://github.com/jaykop/Kata/issues/views

# ── 버전 관리와 커밋 ────────────────────────────────
vcs: git
commit_policy: "AGENTS.md 커밋과 저장소별 추적 범위"   # "커밋해"는 GitHub 와 Diversion 양쪽
commit_gate: true
commit_trailer: "Docs-Reviewed:"

# ── 문서 ────────────────────────────────────────────
docs:
  index: docs/README.md
  manual: docs/manual
  devlog: docs/devlog
  plan: docs/plan
  templates: [docs/manual/_Template.md, docs/devlog/_Template.md, docs/plan/_Template.md]
threads_dir: docs/localdocs/threads
archive_dir: docs/localdocs/archive

# ── 검증 ────────────────────────────────────────────
verification: user
build_commands: [".\\Scripts\\Build.ps1 -Target Editor", ".\\Scripts\\Build.ps1 -Target Game"]
test_setup_mcp: unreal-mcp

# ── 연동 ────────────────────────────────────────────
description_review: true
handoff_reminder_turns: 25
---

# 변경 → 문서 대응표

| 변경 경로 | 검토할 문서 |
|---|---|
| 코드 전체 (`Plugins/*/Source/**`, `Source/**`) | 연결 이슈의 구현·확인 상태와 변경에 영향을 받는 manual·devlog |
| `Plugins/Kata/Source/KataConditions/**` | `docs/manual/Conditions.md` |
| `Plugins/Kata/Source/KataRuntime/**` | `docs/manual/Runtime-Usage.md` |
| `Plugins/Kata/Source/KataEditor/**` | `docs/manual/Editor-Usage.md` |
| `Plugins/Kata/Source/KataGraph*/**` | 그래프를 다루는 manual 단락 (`Editor-Usage.md`, `Runtime-Usage.md`) |
| `Plugins/KataAI/**` | `docs/manual/AI.md`, `docs/plan/AI-Plan.md`, `docs/plan/AI-Graph-Trigger-Plan.md` |
| `Plugins/KataCamera/**` | `docs/manual/Camera.md`, `docs/plan/Camera-Plan.md` |
| `Plugins/KataTargeting/**` | `docs/manual/Targeting.md`, `docs/manual/Target-Tilt.md`, `docs/manual/Factions.md` |
| `Plugins/KataWorld/**` | `docs/manual/Time-Of-Day.md`, `docs/manual/Day-Night.md`, `docs/plan/Level-Event-Plan.md` |
| `Plugins/KataFramework/**/Spawning/**` | `docs/manual/Spawner.md` |
| `Plugins/KataFramework/**/HitTrace/**`, `**/HitReaction/**` | `docs/manual/Hit-Trace.md`, `docs/manual/Hit-Reaction.md`, `docs/plan/Hit-Reaction-Plan.md` |
| `Plugins/KataFramework/**/Character/**`, `**/Attributes/**`, `**/Death/**`, `**/Data/**` | `docs/manual/Character-Data.md`, `docs/manual/Attributes.md`, `docs/manual/Death.md`, `docs/manual/Gameplay-Data.md` |
| `Plugins/KataFramework/**/Equipment/**` | `docs/manual/Equipment.md`, `docs/plan/Equipment-Plan.md` |
| `Plugins/KataFramework/**/Animation/**`, `**/Movement/**` | `docs/manual/Animation-Layers.md`, `docs/manual/Root-Motion-Curve.md`, `docs/plan/Animation-System-Plan.md` |
| `Plugins/KataFramework/**/Input/**`, `**/Player/**`, `**/UI/**` | `docs/manual/Input.md`, `docs/manual/HUD.md` |
| `Plugins/KataFramework/**/Tasks/**`, `**/Time/**`, `**/Cheats/**` | `docs/manual/FX-Tasks.md`, `docs/manual/Time-Dilation.md`, `docs/manual/Cheats.md` |
| `Config/Tags/**`, 태그 생성 스크립트 | `docs/manual/Gameplay-Tags.md` |
| `*.uplugin`, `*.Build.cs`, `*.uproject`, 새 플러그인·모듈 | `AGENTS.md` "프로젝트와 현재 범위"·"모듈 경계", `docs/devlog/2026-10-10-Plugin-Modularization-Decisions.md` 의 종료 시점 구성 |
| `Config/*.ini` 의 Redirect, 직렬화 필드 변경 | manual 의 기존 에셋 처리 안내와 관련 devlog |
| `Source/ProjectKata*/**`, `Content/KataSample/**` | 샘플 전용 도구 manual (`Debug-Tab.md`, `Cheats.md`, `Runtime-Spawner.md` 등)과 관련 이슈 |
| `docs/**` 추가·삭제·이름 변경 | `docs/README.md` 목록, 그 문서를 가리키는 링크 |
| `.claude/settings.json`, `.claude/workflow/**`, `.claude/ko-review/**`, `.claude/advisors/profile.md` | `AGENTS.md` "AI 작업 플러그인", 관련 manual |
| 연결 이슈가 있는 모든 변경 | 이슈에서 링크한 `docs/plan/*.md` 의 확정·미확정 사항, 이슈 체크리스트 |
| 공개 API·UI·설정·실행 계약 변경 | 관련 manual (`AGENTS.md` "문서 작성과 동기화") |
| 설계 결정·반복될 수 있는 문제의 해결 | 주제별 devlog 필요 여부 |
| 결함 수정(이슈 종류와 무관하게 버그를 고친 코드 변경) | 관련 devlog 의 `결함:` 줄(`docs/devlog/_Template.md`). 줄이 없으면 갱신 필요로 판정한다. TD 고문이 이 줄로 되풀이된 버그를 센다 |

# 설계 점검 항목

- 에셋 설정과 캐릭터별 실행 상태를 분리했는가. 공유 에셋·조건 객체에 실행 중 시간·대상 기록·이펙트 핸들을 저장하지 않는가.
- 조건 평가에 부작용이 없는가. 비용 지불·GE 적용·입력 소비는 실행 처리에서 하는가.
- GAS 의 비용·쿨다운·Attribute 를 중복 구현하지 않는가.
- 실행 태스크가 완료·취소·중단·소유자 파괴 때 자원과 이벤트 구독을 정리하는가.
- UObject 참조 수명과 GC 추적을 검토했는가.
- 에셋 경로·모듈명·반영 타입명을 바꾼다면 직렬화된 에셋과 Redirect 를 확인했는가.
- 플러그인 의존 방향(위성·통합 → 코어)과 모듈 경계를 지키는가. 싱글플레이 전용이며 복제·RPC 를 추가하지 않는가.
- 이슈가 닿는 시스템의 manual·plan 서술이 지금 코드와 맞는가. 그 시스템의 문서만 대조하고, 어긋나면 구현 전에 사용자에게 알린다(요청형 대조).

# 코딩 규칙 보충

`AGENTS.md` "구현 원칙"에 없는 Kata 소스 관례.

- BOM 없는 UTF-8. 저작권 헤더를 붙이지 않는다.
- Allman 스타일, 들여쓰기 4칸.
- `#include` 순서: 자기 헤더가 맨 위, `*.generated.h` 가 맨 아래.
- 공용 함수는 `KataFL` 네임스페이스에 두고, 분류는 네임스페이스가 아니라 `KataFL_<주제>` 파일명으로 한다. 파일은 그 주제 폴더(`Public|Private/<주제>/`)에 두고 `FunctionLibraries/` 폴더는 만들지 않는다.

# 메모

- 인계 메모와 설명 변경 목록은 `docs/localdocs/threads/` 에 둔다. 이 폴더는 Git 에서 제외하고 Diversion 에서 추적한다.
- 커밋은 GitHub 와 Diversion 양쪽이다. 범위는 `AGENTS.md` 를 따른다.
