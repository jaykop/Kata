# AI Send Trigger 거절 재시도 정리

작성: 2026-10-11  
갱신: 2026-10-11  
유형: 구현 기록  
대상: KataGraph `UKataGraphInstance`·`UKataGraphComponent` 트리거 결과 API, KataAI `Kata Task: AI Send Trigger`  
기준: 이 기록과 같은 커밋의 작업 트리 변경. 같은 파일에 함께 있던 #58 미커밋 변경은 포함하지 않는다

## 배경과 결론

[#47](https://github.com/jaykop/Kata/issues/47) 측정 중 AI Send Trigger가 대상 액션의 시작 거절을 계속 다시 보내는 경로가 드러났다([해석 비용 측정 기록](2026-10-11-Resolved-Action-Start-Check.md#남은-제한과-후속-작업)).
Single Frame을 끄고 구간을 준 태스크는 Immediate 전이 대상이 쿨다운·태그·조건·차단으로 거절되는 동안 구간 끝까지 매 프레임 같은 트리거를 보냈다.
AI 한 체가 프레임마다 액션 해석 한 번, `LogKata` 로그 한 줄, 에디터 그래프 디버그 Rejected 기록 한 건을 만들었다. 기본값 Single Frame도 시작과 한 번의 Tick에서 두 번 시도했다.

그래프가 트리거 처리 결과를 나눠 돌려주는 API를 추가하고, AI 태스크는 게임플레이 거절을 그 구간의 끝으로 보도록 바꿨다. 창 대기와 엣지 조건 불일치는 이전처럼 다시 시도한다. 연결 이슈는 [#57](https://github.com/jaykop/Kata/issues/57)이다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 그래프 트리거 결과 | `SendTrigger`의 bool이 창 대기 보관, 엣지 불일치, 대상 액션 거절, 설정 오류를 모두 false로 돌려줬다 | `EKataGraphTriggerResult`와 `SendTriggerWithResult`를 추가했다. bool `SendTrigger`는 Transitioned·Reserved를 true로 보는 래퍼로 남겼다 |
| 거절 분류 | `StartNode`는 bool만 돌려준다 | `ApplyTransition`이 결과 enum을 돌려준다. 시작 실패 뒤 그래프가 ContractError로 끝났으면 Failed, 아니면 Rejected다 |
| AI 재시도 | false면 구간 끝까지 다시 보냈다 | Rejected·Failed에서 bDone을 켠다. Buffered·NoMatch·Ignored는 다시 시도한다 |
| 주석 | 컴포넌트 `SendTrigger`의 "버퍼 없이" 설명과 AI 태스크의 "순간 태스크는 Tick을 받지 않는다" 설명이 실제 동작과 달랐다 | #8 보관 동작과 Instant·Single Frame 차이에 맞게 고쳤다 |

## 주요 결정과 이유

- 2026-10-11 사용자가 결과 API 추가와 거절 시 중단 조합을 골랐다. 함께 검토한 대안은 재시도 간격, 그래프 거절 로그 수준 하향, 기존 `GetLastActionStartResult` 간접 판정이었다.
- 재시도 간격은 구간 도중 쿨다운 해제를 잡을 수 있지만 비용이 0이 되지 않는다. 로그 수준 조정은 해석 비용을 그대로 두고 PC 로그까지 줄인다.
- `GetLastActionStartResult` 간접 판정은 코어를 바꾸지 않지만, 같은 액션의 다른 트리거가 남긴 이전 결과를 이 태스크의 거절로 잘못 읽을 수 있다.
- 결과 API는 코어 KataGraph에 확장 지점으로만 둔다. 재시도 정책은 KataAI가 정하므로 KataAI → KataGraph 의존 방향을 유지한다.
- 거절 시 그 구간을 포기하는 것은 PC 입력 한 번이 거절되면 그대로 끝나는 것과 같다. 대가로 구간 도중 쿨다운이 풀려도 그 구간에서는 콤보를 잇지 않는다.
- 게임플레이 거절의 범위는 #46의 분류(`IsGameplayRejection`, BlockedByAbilityTags 포함)를 그대로 쓴다.
- [AI 그래프 트리거 계획](../plan/AI-Graph-Trigger-Plan.md)의 판정 시점 결정("거절되면 구간 안에서 다시 시도한다")을 이 결정으로 갱신했다.

## 근거

- [KataGraphInstance.h](../../Plugins/Kata/Source/KataGraph/Public/KataGraphInstance.h): `EKataGraphTriggerResult`, `UKataGraphInstance::SendTriggerWithResult`.
- [KataGraphInstance.cpp](../../Plugins/Kata/Source/KataGraph/Private/KataGraphInstance.cpp): `SendTriggerWithResult`의 Buffered·NoMatch 분기, `ApplyTransition`의 Rejected·Failed 분류, `StartNode`의 거절 처리.
- [KataTask_AISendTrigger.cpp](../../Plugins/KataAI/Source/KataAI/Private/Tasks/KataTask_AISendTrigger.cpp): `UKataTaskInstance_AISendTrigger::TrySend`의 결과별 처리.
- [KataActionInstance.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionInstance.cpp): Single Frame 태스크가 시작 직후 `TickTaskOnce`를 한 번 받는 경로.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 거절 반복 경로 | 소스 읽기·에이전트 | Immediate 전이 대상의 게임플레이 거절에서만 해석·로그·기록이 생기고, 창 대기와 엣지 불일치에서는 해석하지 않음을 확인 | 없음 |
| 변경 후 빌드·실행 | 사용자 확인(2026-10-11) | 사용자가 테스트 완료를 보고했다 | 확인한 시나리오의 세부 목록은 보고에 없다 |

## 남은 제한과 후속 작업

- 구간 도중 쿨다운이나 차단 태그가 풀려도 같은 구간에서는 다시 보내지 않는다. 이 동작이 필요한 적이 생기면 거절 뒤 재시도 간격 같은 설정을 따로 검토한다.
- [런타임 사용법](../manual/Runtime-Usage.md)의 그래프 절에 남아 있던 #8 이전 문장("입력 버퍼는 아직 지원하지 않는다")은 같은 날 후속 커밋에서 보관 규칙에 맞게 고쳤다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [연결 이슈 #57](https://github.com/jaykop/Kata/issues/57) | 결과 댓글과 종료는 사용자 확인 후 게시 |
| [AI 사용법](../manual/AI.md) | AI Send Trigger 설정 표와 주의 사항에 거절 시 동작 |
| [런타임 사용법](../manual/Runtime-Usage.md) | `SendTriggerWithResult`와 결과 값 설명 |
| [AI 그래프 트리거 계획](../plan/AI-Graph-Trigger-Plan.md) | 판정 시점 결정 갱신. 계획은 #22에 연결되어 있어 유지 |
| [해석 비용 측정 기록](2026-10-11-Resolved-Action-Start-Check.md) | 남은 제한 항목에 후속 링크 |
