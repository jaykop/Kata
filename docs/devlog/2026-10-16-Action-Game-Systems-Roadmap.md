# 액션 게임 기반 시스템 로드맵 종료

작성: 2026-10-16  
갱신: 2026-10-16  
유형: 결정 기록  
대상: 카메라·인풋·타게팅·퍼셉션·스포너와 전투 기반 시스템의 로드맵  
기준: [#24](https://github.com/jaykop/Kata/issues/24) 종료(2026-10-11) 시점. [#76](https://github.com/jaykop/Kata/issues/76)에서 삭제한 `docs/plan/Action-Game-Systems-Plan.md`(2026-09-24 작성, 2026-10-12 갱신)의 결정과 남은 후보를 옮긴 기록

## 배경과 결론

Kata가 액션 에셋·에디터·그래프 전이를 갖춘 뒤, 실제 액션 게임으로 동작하는 데 필요한 기반 시스템을 한 계획에 모았다. 사용자가 우선순위로 꼽은 카메라, 인풋, 타게팅, AI 퍼셉션, 스포너와 함께 필요한 히트 판정·피격 반응·타격감·사망·HUD를 후보로 두었다.

단계별 이슈(#19, #6, #13, #17, #20, #21, #22, #23)로 나눠 진행했고 #24는 2026-10-11에 닫혔다. 계획 문서의 결정은 대부분 이미 주제별 기록으로 옮겨져 있었다. 이 기록은 그 위치를 모으고, 구현하지 않은 후보가 어느 이슈로 이어지는지 남긴다.

## 주요 결정과 이유

| 항목 | 결정 | 기록 |
|---|---|---|
| 입력과 그래프의 연결 | KataGraph는 입력이 아니라 트리거 이벤트 태그로 전이를 받는다 | [입력 계층 결정](2026-10-10-Input-Layer.md) |
| 입력 버퍼 | 전이 창이 닫혀 막힌 마지막 트리거를 보관하고 창의 PreAcceptSeconds를 보관 폭으로 쓴다. 캔슬 입력은 보관하지 않는다(2026-10-10 사용자 결정) | [그래프 트리거 버퍼](2026-10-10-Trigger-Buffer.md) |
| Context의 대상 수 | `FKataContext::TargetActor` 하나를 유지한다 | [프리뷰 멀티 타겟 보류](2026-10-10-Preview-Multi-Target-Waived.md) |
| 퍼셉션·AI 판단 위치 | StateTree 기반 `KataAI` 플러그인(2026-09-24 사용자 결정) | [플러그인 분리 결정](2026-10-10-Plugin-Modularization-Decisions.md) |
| 타게팅 위치 | 별도 `KataTargeting` 플러그인(2026-09-24 사용자 결정). PC·몬스터 모두 엔진 `UTargetingPreset`을 쓴다 | [플러그인 분리 결정](2026-10-10-Plugin-Modularization-Decisions.md), [타게팅 결정](2026-10-10-Targeting-Decisions.md) |
| 카메라 기반 | 자체 구현, SpringArm 대신 플레이어 기준 궤도 트랙(2026-09-27 사용자 결정) | [카메라 시스템 계획](../plan/Camera-Plan.md) |
| 오토 대시 | Motion Warping 대신 자체 구현. 루트 모션 커브와 맞지 않았다 | [오토 대시 결정](2026-10-10-Auto-Dash.md) |
| 피격 반응 표현 | 반응 GA가 Kata 액션 또는 가산 몽타주를 재생한다(2026-10-10 사용자 결정) | [피격 반응 계획](../plan/Hit-Reaction-Plan.md) |
| 공격 토큰 | 별도 토큰 대신 KataAI의 Pressure·Capability로 다룬다 | [#91](https://github.com/jaykop/Kata/issues/91) |
| 전역 메시지 라우터 | 보류(2026-09-24 사용자 확인) | [검토 기록](2026-09-24-Gameplay-Message-Router-Review.md) |

PC·AI가 수집·필터·점수·선택 단계를 공유하는 타게팅 파이프라인은 계획에서 제안으로 남아 있었다. 현재 구조는 [타게팅 사용법](../manual/Targeting.md)을 따른다.

## 구현하지 않은 후보의 행방

계획의 시스템별 요구 가운데 #24 범위에서 구현하지 않은 후보다. 연결한 이슈는 가장 가까운 열린 이슈이며, 그 이슈의 범위로 확정됐다는 뜻은 아니다.

| 후보 | 이어지는 곳 |
|---|---|
| 퍼셉션 인지 단계(의심·수색·놓침·복귀), 인지 전파, 액션 소음 보고, AI 동시 공격 제한 | [#91 AI 후속: 감각·어그로와 Pressure](https://github.com/jaykop/Kata/issues/91) |
| 카메라 연출 모드, 자동 정렬, 다중 상태 | [카메라 시스템 계획](../plan/Camera-Plan.md), [#90 카메라 후속](https://github.com/jaykop/Kata/issues/90) |
| 저스트 입력(패리) | [#87 가드·패링·저스트 회피](https://github.com/jaykop/Kata/issues/87) |
| UI·메뉴 중 입력 차단과 컨텍스트 전환 | [#82 일시정지·메뉴·입력 설정](https://github.com/jaykop/Kata/issues/82) |
| 조우(Encounter) 볼륨과 웨이브 | [레벨 이벤트 계획](../plan/Level-Event-Plan.md), [#63](https://github.com/jaykop/Kata/issues/63) |
| 체크포인트·휴식 재스폰, 처치 기록 저장 | [#88 세이브 계층](https://github.com/jaykop/Kata/issues/88) |
| 피격 반응·히트스톱 | [#23](https://github.com/jaykop/Kata/issues/23), [피격 반응 계획](../plan/Hit-Reaction-Plan.md) |
| 피격 VFX·SFX | [#104 히트 Cue 피격 VFX·SFX](https://github.com/jaykop/Kata/issues/104) |
| 스폰 연출(등장 중 무적·AI 비활성), 일반 적 풀링, 적 체력바 | 연결 이슈 없음. 필요해지면 새 이슈로 만든다 |

## 근거

- 삭제한 계획의 확정 표와 작업 표(AGS-1~AGS-6). 원문은 Git 기록에 있다.
- 이 기록 작성 시점의 열린 이슈 목록(2026-10-16 조회).

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 결정의 이전 위치 | 계획과 각 devlog 대조(에이전트, 2026-10-16) | 위 표의 기록에 결정이 있다 | 빌드·실행 확인은 이 작업과 무관하다 |

## 남은 제한과 후속 작업

연결 이슈가 없는 후보(스폰 연출, 풀링, 적 체력바)는 샘플이나 진행 중인 작업이 요구할 때 이슈로 만든다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| `docs/plan/Action-Game-Systems-Plan.md` | 삭제. 원문은 Git 기록에 있다 |
| [카메라 계획](../plan/Camera-Plan.md), [피격 반응 계획](../plan/Hit-Reaction-Plan.md), [레벨 이벤트 계획](../plan/Level-Event-Plan.md) | 삭제한 계획 링크를 이 기록으로 바꿨다 |
| [문서 목록](../README.md) | plan 항목을 지우고 이 기록을 더했다 |
