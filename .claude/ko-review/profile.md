---
# ko-review 프로필 — Kata. 양식: jaykop/claude-plugins 의 plugins/ko-review/shared/profile-template.md
profile_version: 1
updated: 2026-10-13

instructions: [AGENTS.md]
doc_roots: [docs/manual, docs/devlog]
doc_templates: [docs/manual/_Template.md, docs/devlog/_Template.md, docs/plan/_Template.md]
code_roots: [Plugins/Kata/Source, Plugins/KataTargeting/Source, Plugins/KataAI/Source, Plugins/KataCamera/Source, Plugins/KataFramework/Source, Plugins/KataGASInspector/Source, Source]
exclude: [Plugins/WindowsDualsense_ds5w, Binaries, Intermediate, Saved, DerivedDataCache, Source/ProjectKata/KataTags.h, Source/ProjectKata/KataTags.cpp]
engine: unreal
comment_language: ko
code_text_language: en        # DisplayName·Category·ToolTip 등 메타데이터, 식별자, 로그 문자열
data_comment_files: [Config/Tags/*.ini, Config/DefaultGameplayTags.ini]

queue_dir: docs/localdocs/threads   # Git 에서 제외, Diversion 에서 추적
writer: Claude
reviewer: Codex
terms_file: .claude/ko-review/terms.json
check_runs: user
---

# 메모

- 설명 변경 목록의 대상은 자체 소스 주석, `docs/manual/`, 태그 ini 의 `DevComment` 다(`AGENTS.md` "설명 변경의 공유 검토").
- 벤더 플러그인 `WindowsDualsense_ds5w` 는 원문 주석을 유지한다. 고친 부분의 기록은 그 플러그인의 `UPSTREAM.md` 에 있다.
- 2026-10-12 이전에 만든 목록은 kind 를 `manual`·`tag-devcomment` 로 적었다. 도구가 그대로 읽으므로 고치지 않는다.
