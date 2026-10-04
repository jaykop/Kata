# 입력 캔슬 창과 창 태그 재구성

작성: 2026-10-04  
갱신: 2026-10-04  
유형: 구현 기록  
대상: KataRuntime 창 태스크, KataGraph 엣지 창 태그, KataFramework 입력 처리  
기준: 이 기록과 같은 커밋의 작업 트리

## 배경과 결론

공격 같은 액션을 이동키나 점프 키로 끊고 싶다는 요청이 있었다. 기존 Transition Window는 그래프 엣지가 요구할 때만 전이를 허용하는 이름표다. 이동 입력은 그래프로 가지 않고, 엣지에는 "액션을 끝내고 기본 동작으로 돌아가는" 목적지가 없어서 이 요청을 처리할 수 없었다.

그래프를 거치지 않고 액션을 끊는 Cancel Window 태스크를 추가했다. 입력 계층은 `UKataActionComponent::TryCancelKata`로 캔슬을 요청한다. 점프는 그래프 액션이 아니라 `ACharacter::Jump`로 처리한다. 창 태그 루트를 `Window` 하나로 합쳤고, Transition Window도 항목 배열로 바꿨다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 창 태그 | `TransitionWindow.Combo`, `TransitionWindow.Cancel` | `Window.Transition.Combo`, `Window.Transition.Dodge`, `Window.Cancel.Move`, `Window.Cancel.Jump`. `Config/Tags/Window.ini` |
| Transition Window | 태스크 하나가 `WindowTag`와 `PreAcceptSeconds` 한 쌍을 연다 | `Windows` 배열. 항목마다 태그와 선행 수용 폭을 둔다. 빈 목록·빈 태그·중복 태그는 설정 오류다 |
| Cancel Window | 없음 | 신규. 항목마다 `CancelTag`, `bCancelWhileHeld`를 둔다. 액션 인스턴스가 태그별 개수와 홀드 허용 개수를 센다 |
| 캔슬 API | 없음 | `UKataActionComponent::TryCancelKata(CancelTag, bNewPress)`. 창이 받으면 액션을 Cancelled로 끝낸다 |
| 입력 설정 | 이동·시점과 Input 태그 경로만 있음 | `JumpAction`, `CancelBindings` 추가 |

## 주요 결정과 이유

- **Transition Window와 분리:** 처음에는 Transition Window에 캔슬 기능을 합치는 방안을 검토했다. 그런데 홀드 허용은 입력 캔슬에만 의미가 있고, 그래프 전이는 다음 Kata 액션을 정하는 반면 입력 캔슬은 액션을 끝내기만 한다. 역할이 달라 사용자 결정으로 태스크를 나눴다.
- **태그 루트:** `TransitionWindow.Cancel`처럼 그래프 전이용 이름이 캔슬을 뜻하면 두 태스크와 헷갈린다. 그래서 `Window.Transition.*`과 `Window.Cancel.*`로 나눴다. 다른 루트(`Input`, `Trigger`)처럼 단수형 `Window`를 쓴다.
- **항목 배열:** 같은 구간이라도 태그마다 선행 수용 폭과 홀드 허용이 다르다. 그래서 태그 컨테이너 하나에 설정을 공유하는 대신, 태그와 설정을 묶은 항목의 배열로 했다.
- **누름과 홀드:** 코어는 입력 상태를 저장하지 않는다. 입력 계층이 Started를 새로 누른 요청으로, Triggered를 누르고 있는 요청으로 보낸다. 홀드를 허용한 창은 열린 첫 프레임에 끊긴다.
- **처리 순서:** 캔슬 바인딩을 다른 바인딩보다 먼저 등록한다. 그래서 액션을 끊은 입력이 같은 프레임에 이동·점프로 이어진다.
- **점프 차단:** Kata 액션이 실행 중이면 점프하지 않는다. 이 규칙이 없으면 공격 도중 언제든 점프가 나간다.
- **이전 코드와 리다이렉트 없음:** 태그 이름을 바꾸면서 해당 에셋을 어차피 다시 지정해야 했다. 그래서 `WindowTag`의 로드 시 이전 코드와 태그 리다이렉트를 넣지 않고 사용자가 에셋을 직접 수정한다.
- **캔슬 창의 선행 수용 폭 제외:** 창이 열리기 전에 누른 입력을 받으려면 최근 입력을 저장해야 한다. 이는 입력 버퍼([#8](https://github.com/jaykop/Kata/issues/8))의 범위다.

## 근거

- [KataTask_CancelWindow.h](../../Plugins/Kata/Source/KataRuntime/Public/Tasks/KataTask_CancelWindow.h): 항목 구조와 태스크 동작 규칙.
- [KataTask_TransitionWindow.h](../../Plugins/Kata/Source/KataRuntime/Public/Tasks/KataTask_TransitionWindow.h): 항목 배열로 바꾼 전이 창.
- [KataActionComponent.h](../../Plugins/Kata/Source/KataRuntime/Public/Runtime/KataActionComponent.h): `TryCancelKata`.
- [KataInputHandlerComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Input/KataInputHandlerComponent.cpp): 캔슬 바인딩의 등록 순서와 점프 처리.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 빌드, 이동·점프 캔슬, 기존 콤보 | 사용자 Editor 빌드와 PIE (2026-10-04) | 작동 확인 보고 | 게임패드, Game 타깃 |

## 남은 제한과 후속 작업

- 이번 변경으로 기존 창 태그와 Transition Window 값이 사라진다. 기존 에셋은 사용자가 다시 지정한다.
- 창이 열리기 전에 누른 입력은 저장하지 않는다(#8).
- 가드·패리 캔슬은 해당 기능을 만들 때 `Window.Cancel.*` 또는 `Window.Transition.*` 태그를 추가한다.

## 연관 문서 반영

| 문서 | 반영 내용 |
|---|---|
| [#19](https://github.com/jaykop/Kata/issues/19) | 입력 계층 이슈에서 이 작업을 추적한다 |
| [입력 사용법](../manual/Input.md) | Jump Action, Cancel Bindings, "입력으로 액션 캔슬" 항목 |
| [런타임 사용법](../manual/Runtime-Usage.md) | 기본 태스크 표의 Transition Window·Cancel Window, `TryCancelKata` |
| [게임플레이 태그](../manual/Gameplay-Tags.md) | `Window.Transition`, `Window.Cancel` 루트 |
| [에디터 사용법](../manual/Editor-Usage.md) | 엣지 창 태그 예시와 선택 목록 |
