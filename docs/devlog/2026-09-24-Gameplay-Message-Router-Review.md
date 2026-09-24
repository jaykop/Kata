# 전역 메시지 라우터 도입 검토

작성: 2026-09-24  
갱신: 2026-09-24  
유형: 결정 기록  
대상: 외부 플러그인 GameplayMessageRouter  
기준: 2026-09-24 외부 저장소 확인. Kata 소스 변경 없음

## 배경과 결론

2026-09-24에 [imnazake/GameplayMessageRouter](https://github.com/imnazake/GameplayMessageRouter)를 검토했다.
Gameplay Tag 채널로 USTRUCT 메시지를 전역에 보내고 구독하는 GameInstance 서브시스템이다.
사용자가 도입 보류를 확인했다. 이 기록은 [액션 게임 기반 시스템 계획](../plan/Action-Game-Systems-Plan.md)에서 옮겼다.

## 변경 또는 진단 내용

| 항목 | 발견 내용 | 판단 |
|---|---|---|
| 저장소 상태 | Lyra `GameplayMessageSubsystem`을 떼어 낸 것으로 자체 추가 기능이 거의 없다. 확인 시점에 아카이브 상태이고 LICENSE 파일이 없다. | 이 포크는 쓰지 않는다 |
| Kata 적합성 | 액터 단위 신호는 이미 GAS Gameplay Event(`KataTask_SendGameplayEvent`)로 처리한다. 전역 방송은 대상 정보가 없어 액션·콤보 흐름에 맞지 않는다. | Kata 플러그인에 넣지 않는다 |
| 쓸 만한 곳 | 타게팅·스포너·HUD 사이의 느슨한 알림(락온 대상 변경, 웨이브 시작·종료, 콤보 카운터 UI). | 필요해질 때 재검토 |

## 주요 결정과 이유

- Kata 코어에 넣지 않는다. KataRuntime·KataGraph에 외부 플러그인 의존성이 생기기 때문이다.
- 재검토 조건: HUD나 기반 시스템 사이에 전역 알림이 실제로 필요해지면 다시 검토한다. 그때는 이 포크 대신 UE 5.8 Lyra 원본을
  `ProjectKata` 프로젝트 쪽 플러그인으로 두고 `UPSTREAM.md`로 출처를 관리한다. Epic EULA상 공개 저장소에 포함할 수 있는지 먼저 확인한다.
  Kata에는 필요한 확장점만 두고 라우터 연결 코드는 프로젝트 쪽에 둔다.
- 도입 시 주의: GameInstance 범위라 구독이 레벨 이동 뒤에도 남으므로 리스너 핸들을 직접 해제해야 한다.
  메시지 구조체 타입이 맞는지는 실행 중에만 확인된다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 외부 저장소 | 저장소 내용 확인 | 위 발견 내용 | UE 5.8 호환성 |

## 남은 제한과 후속 작업

없음. 재검토 조건이 생기면 이슈를 만든다.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [액션 게임 기반 시스템 계획](../plan/Action-Game-Systems-Plan.md) | 검토 절을 이 기록으로 옮기고 링크만 남김 |
