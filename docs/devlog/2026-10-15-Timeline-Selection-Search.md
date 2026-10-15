# 타임라인 범위 선택·검색과 눈금 줄 위치

작성: 2026-10-15  
갱신: 2026-10-15  
유형: 구현 기록, 진단  
대상: KataEditor `SKataTimeline`, `FKataActionEditor`  
기준: [#117](https://github.com/jaykop/Kata/issues/117) 작업 트리. 이 기록과 같은 커밋에 포함한다.

## 배경과 결론

Task가 많은 KataAction 타임라인에서 여러 Task를 고르고 찾기가 불편했다. 사용자 요청으로 다음 세 가지를 바꿨다.

- Shift 클릭으로 범위를 선택한다.
- 우클릭 메뉴의 Enable Tasks·Disable Tasks 두 항목을 선택 상태에 맞는 한 항목으로 합친다.
- 타임라인 툴바에 Task 검색 칸을 둔다.

확인 중에 탭 크기를 바꾸면 시간 눈금 줄이 스크롤하기 전까지 이전 위치에 남는 문제가 나왔다. 원인은 스크롤 영역에서 받아 오던 값이었다. 눈금 줄 위치를 그릴 때 실제로 보이는 영역에서 구하도록 바꿨다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 |
|---|---|---|
| 클릭 선택 | Ctrl과 Shift가 모두 한 Task씩 토글했다 | Ctrl은 토글, Shift는 기준 Task부터 클릭한 Task까지 범위로 교체, Ctrl+Shift는 기존 선택에 범위 추가 |
| Enable/Disable 메뉴 | 두 항목이 항상 보이고 바꿀 것이 없는 쪽만 비활성화했다 | 꺼진 Task가 하나라도 있으면 Enable Tasks만, 모두 켜져 있으면 Disable Tasks만 보인다. 선택이 없으면 둘 다 없다 |
| 검색 | 없음 | Search Tasks 칸. Task 이름·Editor Comment, 그룹 제목·Editor Comment에서 대소문자 없이 찾는다 |
| 눈금 줄 위치 | 탭을 키우거나 행이 줄면 스크롤 전까지 이전 위치에 그려졌다 | 그린 프레임의 잘림 영역에서 보이는 영역의 맨 위를 구해 그리기와 입력 판정에 쓴다 |

## 주요 결정과 이유

- **Shift의 의미를 범위 선택으로 바꿨다.** 언리얼 Outliner와 파일 탐색기의 관례를 따랐다. Ctrl+Shift를 범위 추가로 둔 것은 사용자가 고른 방식이다.
- **범위의 기준은 Shift 없이 마지막으로 클릭한 Task다.** 범위 선택은 기준을 바꾸지 않아, Shift 클릭을 이어서 하면 같은 기준에서 범위만 바뀐다. 범위는 화면에 보이는 행만 모으고 그룹 머리글은 건너뛴다. 기준이 접힌 그룹이나 검색에 가려 행이 없으면 범위를 정할 수 없으므로 단일 선택으로 처리하고 클릭한 Task를 새 기준으로 삼는다.
- **혼합 선택에서는 Enable을 보인다.** 사용자 결정이다. 메뉴는 열 때마다 새로 만들므로 열린 시점의 선택 상태로 항목을 정한다.
- **검색으로 숨긴 Task는 선택에서 뺀다.** 숨은 Task가 선택에 남으면 Delete·복사·Enable 같은 선택 명령이 보이지 않는 Task까지 바꾼다. 선택 정리는 검색어가 바뀔 때만 하고, 다른 갱신 경로에서는 하지 않는다.
- **검색 중에는 그룹을 펼쳐 보인다.** 접힌 그룹 안의 결과가 가려지지 않게 하려는 것이다. 저장된 접힘 상태는 바꾸지 않아 검색어를 지우면 원래대로 돌아온다. 그룹 제목이나 그룹 주석이 맞으면 그룹의 Task를 모두 보여 준다. 맞는 Task가 없는 그룹은 머리글까지 숨긴다.
- **검색어는 편집 화면 상태다.** 에셋과 사용자 설정에 저장하지 않는다.
- **눈금 줄 위치를 스크롤 값에서 받지 않는다.** UE 5.8의 `SScrollBox::GetScrollOffset()`은 요청한 오프셋(`DesiredScrollOffset`)을 그대로 돌려준다. 실제 스크롤 위치는 `Tick`에서 내용 크기에 맞춰 잘리므로 두 값이 어긋날 수 있다. 스크롤이 필요 없어지면 실제 위치는 0이 되는데 요청한 오프셋은 이전 값으로 남는다. 그래서 타임라인을 다시 그려도 눈금 줄은 옛 위치에 그려졌다. 이제 `SKataTimeline`이 `OnPaint`의 잘림 영역에서 보이는 영역의 맨 위를 구하고, 같은 값을 입력 판정에도 쓴다. 이 방식은 스크롤 영역에 의존하지 않는다. 스크롤 영역 밖에 두면 0이 된다. 그래서 `VerticalScrollOffset` 속성은 없앴다.
- 채택하지 않은 대안: `GetViewOffsetFraction()`에 내용 높이를 곱해 실제 위치를 다시 계산하는 방법이다. 내용 높이를 얻는 공개 API가 없다. 타임라인 높이로 대신하면 `SBox` 최소 높이 계산에 묶이므로 검토만 하고 쓰지 않았다.

## 근거

- [SKataTimeline.cpp](../../Plugins/Kata/Source/KataEditor/Private/SKataTimeline.cpp): `GetSelectMode`, `SelectRowAt`의 범위 수집, `OnPaint` 시작의 `CachedRulerTop` 계산.
- [KataActionEditor.cpp](../../Plugins/Kata/Source/KataEditor/Private/KataActionEditor.cpp): `SelectTaskRange`, `PassesTimelineFilter`, `SetTimelineFilter`, `RefreshRows`의 그룹 처리, `MakeTimelineContextMenu`의 Enable/Disable 항목.
- 엔진 소스 `Runtime/Slate/Private/Widgets/Layout/SScrollBox.cpp`: `GetScrollOffset`은 `DesiredScrollOffset`을 반환하고, `Tick`은 `PhysicalOffset`을 잘라 쓰며 스크롤 막대가 필요 없으면 0으로 둔다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 범위 선택·메뉴·검색 | 사용자 Editor 빌드와 에디터 시험 (2026-10-15) | 모두 기대대로 동작한다고 보고 | 항목별 결과는 따로 보고되지 않음 |
| 눈금 줄 위치 | 사용자 Editor 빌드와 에디터 시험 (2026-10-15) | 기대대로 동작한다고 보고 | 항목별 결과는 따로 보고되지 않음 |

자동화 테스트는 추가하지 않았다. 슬레이트 입력과 그리기는 플레이로 확인하는 대상이다.

## 남은 제한과 후속 작업

- 검색 중 그룹 머리글 화살표를 누르면 저장된 접힘 상태만 바뀌고 화면에는 드러나지 않는다. 검색어를 지우면 바뀐 상태가 보인다.
- 키보드로 범위를 선택하는 기능(Shift+방향키)은 없다.
- 프리뷰 Self·Target을 Character Row로 지정하는 기능은 별도 백로그 [#118](https://github.com/jaykop/Kata/issues/118)이다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#117](https://github.com/jaykop/Kata/issues/117) | 구현과 사용자 확인 완료. 커밋 후 결과 댓글을 남기고 닫는다 |
| [에디터 사용법](../manual/Editor-Usage.md) | 선택 조작, Search Tasks, Enable/Disable 메뉴, 확인 상태를 갱신 |
