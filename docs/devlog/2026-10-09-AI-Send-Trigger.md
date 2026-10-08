# AI 콤보 트리거를 액션 태스크로 발신

작성: 2026-10-09  
갱신: 2026-10-09  
유형: 결정 기록  
대상: KataAI `Kata Task: AI Send Trigger`, 액션 편집기 Add Task 메뉴·Auto Resize  
기준: #22 하위 작업. 이 기록과 같은 날의 커밋

## 배경과 결론

AI는 그래프 진입 트리거만 보낼 수 있어서, PC처럼 Transition Window에서 다음 공격으로 이어지는 콤보를 만들 수 없었다.
처음에는 StateTree Task가 창 태그를 보고 트리거를 보내는 방식을 제안했다. 사용자가 액션 타임라인에 트리거 발신 태스크를 배치하는 방식을 제안했고, 진단 결과 이쪽이 낫다고 보고 확정했다.
같은 작업에서 사용자 요청으로 액션 편집기의 Add Task 메뉴를 카테고리 트리로 바꾸고 Auto Resize 동작을 고쳤다.

## 변경 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| AI 트리거 발신 | 없음. StateTree Task가 진입 트리거만 보냄 | KataAI에 `UKataTask_AISendTrigger`. 소유자의 `UKataGraphComponent::SendTrigger`로 보낸다 |
| Add Task 메뉴 | 엔진 클래스 선택기의 이름순 목록 | `SGraphActionMenu` 카테고리 트리와 검색. 카테고리는 태스크 클래스의 `KataTaskCategory` 메타 |
| Auto Resize | Duration 자동 설정(몽타주 지정) 때만 맞춤. 표시 길이는 전역 값 하나 | 에셋을 열 때와 편집 갱신마다 맞춤. 표시 길이는 에셋별로 저장 |

## 주요 결정과 이유

- 발신 위치는 액션 타임라인이다. 전이 타이밍을 창과 같은 화면에서 프레임 단위로 정할 수 있다. StateTree 쪽 방식은 창 태그를 거쳐 간접적으로 맞춰야 했다.
- 같은 액션을 PC가 쓰면 PC 콤보가 저절로 이어지는 문제는 소유 Pawn이 플레이어 조종일 때 아무것도 하지 않는 것으로 막았다. 처음에 이 안을 반대한 근거가 이것이었는데 해결할 수 있었다.
- 태스크는 KataAI에 둔다. 코어는 AI와 그래프 발신을 모르게 유지하고, 엣지는 원래 트리거로 발동하므로 코어 변경이 필요 없다.
- 기본값은 Single Frame이다(사용자 결정). 전이 가능 구간은 Transition Window가 정하므로 이 태스크는 입력처럼 한 시점에 보내면 된다. 같은 시각의 창 태스크보다 늦게 시작하도록 기본 Order Hint를 1로 뒀다(같은 단계 안에서 Order Hint 오름차순 실행).
- Chance는 시작 때 한 번 굴린다. 구간 모드에서 매 프레임 굴리면 사실상 항상 성공한다.
- Condition의 대상은 AI 타게팅의 현재 대상이다. 콤보 도중 대상이 바뀌거나 사라진 것을 반영해, 콤보 중단을 별도 정책 없이 조건으로 처리한다.
- AI가 스스로 하는 이탈도 Cancel Window를 지키게 하는 원칙을 정했다. 피격·사망·연출 같은 강제 중단은 예외다. 구현은 반응 행동·Pressure 설계 때 한다([계획](../plan/AI-Graph-Trigger-Plan.md)).
- 창 밖 배치를 경고하는 데이터 검증은 보류했다. 액션 검증은 태스크별 `GetConfigurationError`만 보므로 태스크가 다른 태스크의 구간을 볼 수 없다. 코어 검증 확장 지점과 Editor Validator 중 선택이 남았다.

## 근거

- [KataTask_AISendTrigger.cpp](../../Plugins/KataAI/Source/KataAI/Private/Tasks/KataTask_AISendTrigger.cpp): 플레이어 제외, 확률, 조건 대상, 재시도.
- [KataActionEditor.cpp](../../Plugins/Kata/Source/KataEditor/Private/KataActionEditor.cpp): `MakeTaskClassMenu`, `ResizeViewToTasks`, 에셋별 설정 키.
- [KataAction.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Action/KataAction.cpp): 같은 시각 태스크의 단계·Order Hint 정렬.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| AI Send Trigger | 사용자 빌드·실행 보고 | 2026-10-09 트리거 실행 확인 보고 | 항목별(확률·조건·PC 제외·분기) 결과는 보고되지 않음 |
| Add Task 메뉴 | 사용자 빌드·UI 확인 | 카테고리 메뉴 표시 확인 | 로드되지 않은 Blueprint 태스크 표시 |
| Auto Resize | 사용자 확인 보고 | 2026-10-09 동작 확인 보고 | 항목별 결과는 보고되지 않음 |
