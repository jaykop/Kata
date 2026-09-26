# Kata 문서

갱신: 2026-09-25

현재 문서는 `devlog`, `manual`, `plan` 세 카테고리로 관리한다. 이 README는 문서 안내이며 네 번째 카테고리는 아니다.
작성 규칙은 [AGENTS.md](../AGENTS.md)의 문서 작성과 동기화를 따른다.

## 분류와 템플릿

| 위치 | 기록할 내용 | 템플릿 |
|---|---|---|
| devlog | 실제 변경, 결정 이유, 진단과 확인 결과 | [기록 템플릿](devlog/_Template.md) |
| manual | 현재 기능의 사용 순서, API·설정 계약, 제한 | [사용법 템플릿](manual/_Template.md) |
| plan | Large 이슈에 딸린 설계 문서. 설계안, 확정·미확정 사항, 작업 순서와 완료 조건 | [계획 템플릿](plan/_Template.md) |

현재 상태는 [Implementation-Status.md](devlog/Implementation-Status.md), 진행 중인 작업과 후속 작업은
[GitHub 이슈](https://github.com/jaykop/Kata/issues)를 먼저 읽는다. 진행 상태는 이슈에서만 관리하고, plan은 이슈에서 링크하는 설계 문서로 쓴다. 계획이나 과거 기록을 현재 구현 사양으로 사용하지 않는다.
기능 변경 시 구현 상태와 관련 사용법·계획을 함께 갱신하고, 중요한 결정 이유는 devlog에 연결한다.
2026-09-25 기존 설명서와 결정 기록을 현행 코드에 맞췄다. 소스 반영과 실제 실행 확인은 구분하며,
문서 정비 결과와 남은 실행 확인은 [현행화 기록](devlog/2026-09-25-Documentation-Maintenance.md)을 따른다.

## Manual

- [에디터 사용법](manual/Editor-Usage.md)
- [런타임 사용법](manual/Runtime-Usage.md)
- [태스크·Command 제작](manual/Task-Authoring.md): C++·BP 확장과 실행별 자원 수명.
- [기존 에셋·API 이전](manual/Asset-Migration.md): 자동 처리·수동 재설정·Redirect 범위.
- [기본 조건](manual/Conditions.md)
- [게임플레이 태그 사용법](manual/Gameplay-Tags.md): 태그 ini 구조와 `KataTag` 코드 생성.
- [팩션 사용법](manual/Factions.md): Kata Factions 설정과 `UKataFL_Faction` 관계 판정.
- [타게팅 사용법](manual/Targeting.md): 타게팅 컴포넌트, 소프트 타겟·락온, Preset 확장 태스크.

## Devlog

- [현재 구현 상태](devlog/Implementation-Status.md)
- [액션 에셋 모델과 상속](devlog/2026-09-25-Action-Asset-Model.md)
- [실행 순서와 태스크 수명](devlog/2026-09-25-Execution-Lifecycle.md)
- [GAS 책임과 기본 태스크](devlog/2026-09-25-GAS-and-Tasks.md)
- [그래프 전이와 대상 유지](devlog/2026-09-25-Graph-Transition.md)
- [설명서·결정 기록 현행화](devlog/2026-09-25-Documentation-Maintenance.md)
- [C++ 공용 함수 및 Blueprint Task 진단](devlog/Function-Library-and-Blueprint-Task-Diagnosis.md)
- [문서 부채와 분류 진단](devlog/2026-09-24-Documentation-Diagnosis.md): 불일치 근거와 정비 우선순위.
- [Play Montage 포즈 탐색 진단](devlog/2026-09-24-Montage-Scrub-Diagnosis.md): 현재 스크럽과 UE 5.8 몽타주 에디터 경로 비교.
- [Play Montage 포즈 탐색 구현](devlog/2026-09-24-Montage-Scrub-Implementation.md): 직접 포즈 평가와 일반 재생 전환.
- [프리뷰 시간 탐색의 실행 시뮬레이션 전환](devlog/2026-09-24-Preview-Scrub-Simulation.md): 최종 탐색 방식과 한 프레임 동기 진행의 엔진 제약.
- [게임플레이 태그 코드 생성 구현](devlog/2026-09-24-Gameplay-Tag-Generation.md): Native ini 기반 생성과 `+` 접두사 문제.
- [모듈 구조와 AKataCharacter 진단](devlog/2026-09-24-Module-Structure-Diagnosis.md): 플러그인 분리·캐릭터 이동·KataAI 범위 결정.
- [전역 메시지 라우터 도입 검토](devlog/2026-09-24-Gameplay-Message-Router-Review.md): GameplayMessageRouter 보류 결정과 재검토 조건.
- [UKataComponent 이름 변경](devlog/2026-09-26-KataActionComponent-Rename.md): UKataActionComponent로 변경과 Redirect.
- [KataFramework 캐릭터 조합](devlog/2026-09-26-KataFramework-Character-Composition.md): AKataCharacter 컴포넌트 구성, 팀 인터페이스, AKataPlayerCharacter.

구현 전 단계의 Claude/Codex 설계 제안과 이전 Blueprint 중심 사용법은 현재 구조와 충돌해 폐기했다.

## Plan

- [플러그인 분리 모듈화 계획](plan/Plugin-Modularization-Plan.md): 코어·위성·통합 플러그인 구성과 단계. [#1](https://github.com/jaykop/Kata/issues/1).
- [타게팅 시스템 설계](plan/Targeting-Plan.md): KataTargeting의 컴포넌트·Preset·팩션 설계. [#13](https://github.com/jaykop/Kata/issues/13).
- [액션 게임 기반 시스템 계획](plan/Action-Game-Systems-Plan.md): 카메라·인풋·타게팅·퍼셉션·스포너 후보. 연결 이슈 없음(제안).
- [기본 태스크 확장 계획](plan/Base-Task-Plan.md): [#6](https://github.com/jaykop/Kata/issues/6)·[#7](https://github.com/jaykop/Kata/issues/7)의 설계 참고. 당시 제안과 현재 구현 전제를 구분한다.
- [Hit Trace 계획](plan/Hit-Trace-Plan.md): HitBox 프리셋·컴포넌트, 서브스텝 보정, Hit Subsystem·Handler, Preset 필터. [#6](https://github.com/jaykop/Kata/issues/6).
- [액션 비용 정책 계획](plan/Cost-Policy-Plan.md): [#9](https://github.com/jaykop/Kata/issues/9).
- [프리뷰 멀티 타겟 배치 계획](plan/Preview-Multi-Target-Plan.md): [#10](https://github.com/jaykop/Kata/issues/10).

2026-09-24 작업 추적을 GitHub 이슈로 옮기면서 다음 작업 계획(Next-Work-Plan)을 삭제했다. 우선순위와 보류 항목은 이슈 #3~#12로,
Cost 정책과 멀티 타겟 배치 설계는 별도 plan으로 옮겼다. Kata·KataAction·KataGraph 요청 메모와 완료된 Play Montage 포즈 탐색 계획도
같은 날 삭제했으며 결과는 관련 devlog와 이슈에 남아 있다.
