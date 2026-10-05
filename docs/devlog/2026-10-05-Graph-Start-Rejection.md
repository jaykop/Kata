# 그래프 전이의 액션 시작 거절 처리

작성: 2026-10-05  
갱신: 2026-10-05  
유형: 구현 기록  
대상: KataRuntime·KataGraph  
기준: #28 미커밋 구현

## 배경과 결론

그래프는 다음 액션을 요청하기 전에 기존 액션을 Branched로 끝냈다. 쿨다운·조건 등 정상적인 게임플레이 사유로 시작이 거절돼도 기존 공격이 끊기고 ContractError로 종료됐다.
사용자가 승인한 #28의 결정에 따라 시작 판정·초기화를 성공한 뒤 기존 액션을 교체하고, 거절 시점에 따라 유지·대기·정상 종료하도록 변경했다.

## 변경 내용과 이유

- UKataActionComponent의 C++ API PlayKataActionTransition은 호출자가 지정한 인스턴스가 현재 실행 중인 액션과 일치할 때만 차단 정책을 우회한다. 일반 시작 요청에는 기존 정책을 적용한다.
- StartResolved는 초기화 성공 후 기존 액션을 종료한다. 그래프 전이는 Branched, 일반 교체는 Interrupted를 사용한다.
- 그래프는 임시 Context에서 Keep Target을 처리하고, 시작 성공 후 노드·대상·예약을 반영한다. 거절된 즉시 전이는 기존 상태를 보존하며 다른 엣지를 대신 고르지 않는다.
- 게임플레이 거절은 Log로 기록한다. 실행 중에는 유지하고, 정상 종료 후에는 Completed, 진입 시에는 WaitingForEntry로 처리한다. 실제 오류는 ContractError를 유지한다.
- 시작·종료 알림 중 트리거 재진입을 막고, 콜백에서 그래프가 종료되면 새 액션도 Cancelled로 정리한다.

액션 조건을 그래프 선택 단계로 옮기지 않았다. 시작 허용 판정은 액션 런타임에 유지해 두 번 평가하거나 그래프에 GAS 정책을 복제하지 않는다.

## 근거

- [KataActionComponent](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionComponent.cpp): 시작 판정·초기화와 교체 순서.
- [KataGraphInstance](../../Plugins/Kata/Source/KataGraph/Private/KataGraphInstance.cpp): 전이 결과와 그래프 상태 처리.
- [연결 이슈 #28](https://github.com/jaykop/Kata/issues/28): 확정된 거절 처리 규칙.

## 확인 범위와 결과

2026-10-05 사용자가 안내한 쿨다운 전이 거절 확인을 수행하고 예상대로 작동한다고 보고했다. 확인 범위는 두 번째 공격의 쿨다운 중 즉시 전이가 거절돼도 첫 번째 공격이 유지되는 동작이다. 에이전트 빌드·테스트·별도 코드 검사는 미실시이며 빌드 결과는 별도로 보고되지 않았다. 에셋과 모듈 의존성은 변경하지 않았다.

## 남은 확인과 연관 문서 반영

쿨다운에 따른 즉시 전이 거절 시 공격 유지는 사용자 확인 완료다. 다른 거절 사유, 예약·자동 전이 거절 시 정상 종료와 진입 거절 시 실패 반환은 별도 확인 결과가 없다.
[Runtime 사용법](../manual/Runtime-Usage.md)에 실행 계약을 반영했다. 공개 이슈의 결과 댓글·라벨 변경은 사용자 확인 후 게시한다.
