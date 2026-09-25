# 그래프 전이와 대상 유지 결정

작성: 2026-09-25  
유형: 기존 구현·결정의 정리 기록  
대상: KataGraph·KataRuntime 전이 연결  
기준: 현재 소스·기존 상태 기록. 최초 결정일을 소급 추정하지 않는다.  
관련 이슈: [#12](https://github.com/jaykop/Kata/issues/12), 대상 유지 확장은 [#1](https://github.com/jaykop/Kata/issues/1)

## 배경과 결론

액션은 시간 구간을, 그래프는 전이 목적지를 소유한다. Trigger 이벤트와 이름 붙은 Transition Window로
연결해 입력·AI·Anim Notify가 같은 실행 API를 사용할 수 있게 했다.
GenericGraph 흡수 출처와 변경 내역은 [UPSTREAM.md](../../Plugins/Kata/Source/KataGraph/UPSTREAM.md)에 유지한다.

## 구조와 주요 결정

| 항목 | 계약과 이유 |
|---|---|
| 그래프 기반 타입 | 런타임 Base 클래스 3개를 다시 작성하고 순회에 방문 기록을 둔다. 병렬 엣지는 UHT 제약 때문에 TMap의 값에 FKataGraphEdgeList를 둔다 |
| UKataNode | 공통 추상 부모를 두어 스키마의 노드 필터 아래 Entry·Action·Conduit 타입을 함께 제공한다 |
| Conduit | 액션을 실행하지 않고 다음 실행 노드까지 한 번에 해석한다. 공통 분기를 묶어 다대다 직접 연결의 수를 줄인다 |
| 우선순위 | 큰 Priority, 동률은 저장된 자식·엣지 순서다. TMap 순회 순서를 결과에 사용하지 않는다 |
| Immediate·OnActionEnd | 즉시 교체와 정상 완료 후 예약을 구분한다. 같은 액션의 새 유효 예약은 이전 예약을 교체한다 |
| Branched | 그래프가 실행을 넘길 때와 외부 Interrupted를 구분한다. 뒤에 들어온 종료 요청으로 첫 사유를 덮지 않는다 |
| Keep Target | 기본 true. 끄면 다음 액션 대상을 비우고 Pre Commands가 다시 정할 수 있다. 진입 엣지는 시작 Context를 사용한다 |

엣지 조건을 통과한 뒤 대상 노드의 EntryCondition을 평가한다. Conduit의 내부 경유 엣지는
Window·Timing을 보지 않고, 비어 있거나 같은 트리거를 받는 경로를 고른다. 대상 유지도 출발 액션의 첫 엣지를 따른다.
액션 시작 직후 Pre Commands가 바꾼 TargetActor를 그래프 Context에 다시 기록해 다음 전이에 전달한다.
공용 타게팅 컴포넌트나 구체 대상 선택 Command는 아직 없다.

## 입력·자동 전이와 제한

TriggerTag는 계층 매칭한다. 비면 그래프 시작·정상 액션 완료 때 자동 전이를 평가한다.
진입 노드에서는 Window·Timing을 무시한다. 액션 완료 후 자동 전이에 창을 요구하면 이미 닫힌 상태일 수 있으므로
일반적인 완료 경로에서는 Required Window를 비운다.
SendTrigger는 호출 순간 한 번만 판정한다. PreAcceptSeconds는 시각 판정 값이며 선행 입력을 저장·재평가하지 않는다.
입력 버퍼 후속은 [#8](https://github.com/jaykop/Kata/issues/8)에 있다.

동기 전이가 32단계를 넘으면 ContractError로 종료해 즉시 전이 순환이 계속되지 않게 한다.
SubGraph는 재사용·Context 계약이 미정이고 Alias는 같은 액션 참조로 현재 요구를 표현할 수 있어 보류했다.
실행 결과 전용 Failed 종료는 도입하지 않았다. 시작 거절과 실행 중 분기를 구분하고 추가 요구가 생기면 다시 결정한다.
다중 액션 채널은 현재 없다.

## 근거와 확인 범위

- [KataGraphInstance.cpp](../../Plugins/Kata/Source/KataGraph/Private/KataGraphInstance.cpp): 선택·경유·예약·대상 유지·동기 제한.
- [KataEdge.h](../../Plugins/Kata/Source/KataGraph/Public/KataEdge.h): 전이 설정.
- [KataTask_TransitionWindow.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_TransitionWindow.cpp): 액션의 창 수명.
- [KataActionInstance.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionInstance.cpp): 종료 사유·창 시간 판정.

기존 기록에는 그래프 에셋 생성·Editor 빌드 확인이 있다. 2026-09-24 Keep Target·Command의 빌드·Details 표시는
사용자가 확인했으나 런타임 전이는 미확인이다. 이번에는 소스만 대조하고 빌드·게임·UI 실행을 하지 않았다.
Comment 생성 위치 문제는 [#3](https://github.com/jaykop/Kata/issues/3)에서 재현 확인을 기다린다.
사용법은 [Editor-Usage](../manual/Editor-Usage.md#그래프-에디터), [Runtime-Usage](../manual/Runtime-Usage.md#콤보-그래프-실행)에 반영했다.
