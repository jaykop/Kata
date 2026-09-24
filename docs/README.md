# Kata 문서

갱신: 2026-09-24

현재 문서는 `devlog`, `manual`, `plan` 세 카테고리로 관리한다. 이 README는 문서 안내이며 네 번째 카테고리는 아니다.
작성 규칙은 [AGENTS.md](../AGENTS.md)의 문서 작성과 동기화를 따른다.

## 분류와 템플릿

| 위치 | 기록할 내용 | 템플릿 |
|---|---|---|
| devlog | 실제 변경, 결정 이유, 진단과 확인 결과 | [기록 템플릿](devlog/_Template.md) |
| manual | 현재 기능의 사용 순서, API·설정 계약, 제한 | [사용법 템플릿](manual/_Template.md) |
| plan | 후속 작업, 확정·미확정 사항, 항목별 상태와 완료 조건 | [계획 템플릿](plan/_Template.md) |

현재 상태는 [Implementation-Status.md](devlog/Implementation-Status.md), 후속 작업은
[Next-Work-Plan.md](plan/Next-Work-Plan.md)를 먼저 읽는다. 계획이나 과거 기록을 현재 구현 사양으로 사용하지 않는다.
기능 변경 시 구현 상태와 관련 사용법·계획을 함께 갱신하고, 중요한 결정 이유는 devlog에 연결한다.
아래 기존 문서에는 갱신 누락이 남아 있으며, 구체적인 차이는 문서 부채 진단에 기록했다.

## Manual

- [에디터 사용법](manual/Editor-Usage.md)
- [런타임 사용법](manual/Runtime-Usage.md)
- [기본 조건](manual/Conditions.md)
- [게임플레이 태그 사용법](manual/Gameplay-Tags.md): 태그 ini 구조와 `KataTag` 코드 생성.

## Devlog

- [현재 구현 상태](devlog/Implementation-Status.md)
- [C++ 공용 함수 및 Blueprint Task 진단](devlog/Function-Library-and-Blueprint-Task-Diagnosis.md)
- [문서 부채와 분류 진단](devlog/2026-09-24-Documentation-Diagnosis.md): 불일치 근거와 정비 우선순위.
- [Play Montage 포즈 탐색 진단](devlog/2026-09-24-Montage-Scrub-Diagnosis.md): 현재 스크럽과 UE 5.8 몽타주 에디터 경로 비교.
- [Play Montage 포즈 탐색 구현](devlog/2026-09-24-Montage-Scrub-Implementation.md): 직접 포즈 평가와 일반 재생 전환.
- [프리뷰 시간 탐색의 실행 시뮬레이션 전환](devlog/2026-09-24-Preview-Scrub-Simulation.md): 최종 탐색 방식과 한 프레임 동기 진행의 엔진 제약.
- [게임플레이 태그 코드 생성 구현](devlog/2026-09-24-Gameplay-Tag-Generation.md): Native ini 기반 생성과 `+` 접두사 문제.
- [모듈 구조와 AKataCharacter 진단](devlog/2026-09-24-Module-Structure-Diagnosis.md): 플러그인 분리·캐릭터 이동·KataAI 범위 결정.

구현 전 단계의 Claude/Codex 설계 제안과 이전 Blueprint 중심 사용법은 현재 구조와 충돌해 폐기했다.

## Plan

- [다음 작업 계획](plan/Next-Work-Plan.md)
- [플러그인 분리 모듈화 계획](plan/Plugin-Modularization-Plan.md): 코어·위성·통합 플러그인 구성과 단계. PM-1 구현, 빌드 미확인.
- [액션 게임 기반 시스템 계획](plan/Action-Game-Systems-Plan.md): 카메라·인풋·타게팅·퍼셉션·스포너 후보. 제안.
- [Play Montage 포즈 탐색 수정 계획](plan/Montage-Scrub-Plan.md): 실행 시뮬레이션 방식으로 완료, 사용자 확인 완료.
- [게임플레이 태그 체계와 컴포넌트 태그 계획](plan/Gameplay-Tag-Plan.md): 네이티브 태그 생성 완료, 컴포넌트 태그 작업 보류. Hit Trace 선행 작업.
- [기본 태스크 확장 계획](plan/Base-Task-Plan.md): 제안 당시의 전제와 현재 구현을 구분해 갱신해야 한다.
- [Kata 공통 요청 메모](plan/Kata.md)
- [KataAction 요청 메모](plan/KataAction.md)
- [KataGraph 요청 메모](plan/KataGraph.md)

다음 작업 계획에는 이미 구현된 후보의 동기화가 필요하다. 요청 메모는 원래 질문을 보존한 문서이며
모든 항목이 미구현이라는 뜻은 아니다. 결과를 확인해 완료·보류·미확정 상태로 연결한다.
