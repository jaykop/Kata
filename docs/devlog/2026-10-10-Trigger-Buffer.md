# 그래프 트리거 버퍼

작성: 2026-10-10  
갱신: 2026-10-10  
유형: 구현 기록  
대상: KataGraph `UKataGraphInstance`, KataRuntime `UKataActionInstance` 전이 창 알림, KataGraphEditor 그래프 디버거  
기준: 이 기록과 같은 커밋의 작업 트리

## 배경과 결론

`SendTrigger`는 호출한 순간에만 판정했다. 요구하는 전이 창이 아직 닫혀 있으면 입력을 버렸으므로, 창이 열리기 조금 전에 누른 콤보 입력은 사라졌다. Transition Window의 `PreAcceptSeconds`는 "창이 열리기 이만큼 전 입력까지 받는다"는 값이었지만, 입력을 보관하는 곳이 없어 판정 결과를 바꾸지 못했다([그래프 전이 기록](2026-09-25-Graph-Transition.md)).

그래프 인스턴스가 창 때문에 막힌 마지막 트리거를 보관하도록 했다. 현재 액션에서 그 창이 열리면 보관한 트리거를 도착 시각 기준으로 다시 평가한다. 이제 `PreAcceptSeconds`가 창별 보관 폭으로 작동한다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 창 개방 알림 | 없음 | `UKataActionInstance::OnTransitionWindowOpened`(C++ 전용 멀티캐스트). 같은 태그 창이 처음 열리고 액션이 실행 중일 때 창 상태를 기록한 뒤 알린다 |
| 트리거 보관 | 거절된 트리거를 버림 | 일치하는 수동 엣지가 전이 창 판정에서만 탈락했으면 태그와 도착 시각을 슬롯 하나에 보관한다. `SendTrigger`는 false를 돌려준다 |
| 재평가 | 없음 | 창이 열리면 그 창을 요구하는 엣지만 보관 시각으로 평가한다. 성립하면 엣지 Timing에 따라 즉시 전이하거나 예약한다 |
| 버퍼 비우기 | 해당 없음 | 트리거 수락, 액션 변경·종료, 그래프 종료 |
| 그래프 디버거 | Transition·Reserved·Rejected·Ended | `Buffered` 행 추가. 같은 트리거를 다시 보관하면 시각만 갱신하고 기록을 남기지 않는다 |

## 주요 결정과 이유

- **마지막 하나만 보관:** 사용자 결정이다. 연타해도 마지막 입력만 남아 결과를 예측하기 쉽다. 트리거별 슬롯은 여러 입력이 한꺼번에 후보가 되어 우선순위 해석이 복잡해진다.
- **창 개방 이벤트로 재평가:** 그래프 인스턴스는 Tick이 없다. 보관한 입력의 판정이 바뀌는 때는 창이 열리는 순간뿐이므로 Tick 폴링을 추가하지 않았다.
- **열린 창의 엣지로만 평가:** 창이 필요 없는 엣지는 입력이 도착한 순간 이미 평가를 마쳤다. 다시 보면 그때 조건 때문에 거절된 입력이 나중에 뒤늦게 나갈 수 있다.
- **보관 폭은 `PreAcceptSeconds`:** 별도의 버퍼 유지 시간을 두지 않았다. 너무 이른 입력은 `AcceptsTriggerAt`의 시각 판정에서 떨어진다. 그래서 재평가에 실패해도 같은 액션의 다른 창을 위해 보관을 유지한다.
- **반환값 유지:** `SendTrigger`의 true는 계속 "지금 전이 또는 예약"을 뜻한다. 입력 계층의 진입 실패 정리와 AI Send Trigger 태스크의 재시도가 이 의미에 기대고 있다.
- **액션이 바뀌면 버림:** 보관한 입력은 이전 액션의 창을 기다리던 것이다. 다음 액션으로 넘기면 의도하지 않은 다음 단계 콤보가 나갈 수 있다.
- **캔슬 창 선행 수용 제외:** 사용자 결정이다. 이동은 Cancel While Held로 이미 받는다. 점프를 보관했다가 나중에 끊으면, 끊는 순간 점프가 나가지 않아 동작이 어색하다. 필요해지면 별도 작업으로 다룬다.
- **재진입:** 창 태스크가 시작하는 도중에 알림이 나가므로, 즉시 전이가 현재 액션을 그 자리에서 끝낼 수 있다. AI Send Trigger 태스크가 이미 같은 경로를 쓴다. 바인딩은 `AddUObject`로 하고 액션을 떼어 낼 때 `RemoveAll`로 해제한다.

입력이 액션 Tick보다 먼저 처리되는 프레임에서 창이 열려도, 이전에는 그 입력을 버렸다. 이제는 보관한 시각과 창이 열린 시각이 같아 `PreAcceptSeconds`가 0이어도 받는다.

## 근거

- [KataGraphInstance.cpp](../../Plugins/Kata/Source/KataGraph/Private/KataGraphInstance.cpp): `SendTrigger`, `HandleTransitionWindowOpened`, `ApplyTransition`, `ConsiderNodeTransitions`의 창 차단 판정.
- [KataActionInstance.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionInstance.cpp): `OpenTransitionWindow`의 첫 개방 알림.
- [SKataGraphDebugView.cpp](../../Plugins/Kata/Source/KataGraphEditor/Private/SKataGraphDebugView.cpp): `Buffered` 행.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| Editor 빌드, PIE 선입력·이른 입력·기존 콤보·액션 변경·AI 콤보 | 사용자 빌드와 PIE (2026-10-10) | 통과 보고 | Game 타깃 |

## 남은 제한과 후속 작업

- 액션이 바뀌면 이전 액션에서 보관한 입력은 버린다.
- 캔슬 창의 선행 수용은 없다.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [#8](https://github.com/jaykop/Kata/issues/8) | 구현과 사용자 확인 결과 |
| [런타임 사용법](../manual/Runtime-Usage.md) | `PreAcceptSeconds`와 `SendTrigger` 보관 동작 |
| [입력 사용법](../manual/Input.md) | 문제 해결 표의 입력 처리 구간 항목 |
| [그래프 전이 기록](2026-09-25-Graph-Transition.md), [캔슬 창 기록](2026-10-04-Cancel-Window.md) | 당시 기록은 유지하고 후속 기록 링크만 추가 |
