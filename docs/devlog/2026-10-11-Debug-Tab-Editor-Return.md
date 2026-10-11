# 디버그 UI를 인게임 메뉴에서 에디터 탭으로 되돌림

작성: 2026-10-11  
갱신: 2026-10-11  
유형: 결정 기록, 진단  
대상: 샘플 모듈 `ProjectKataEditor`(Kata Debug 탭), 제거한 `ProjectKataDebug`, SlateIM 사용 정책  
기준: main 406fcf9(인게임 메뉴 1차 커밋) 이후 미커밋 작업 트리

## 배경과 결론

치트([#54](https://github.com/jaykop/Kata/issues/54))를 마우스로 조작하고 런타임 스포너도 Dev 빌드에서 쓰려고 SlateIM 사용 정책을 런타임까지 넓혔다.
그다음 샘플 DeveloperTool 모듈 `ProjectKataDebug`에 게임 화면 위 SlateIM 메뉴를 만들었다([#55](https://github.com/jaykop/Kata/issues/55), 커밋 406fcf9). 치트 패널은 사용자가 PIE에서 확인했다.
이어서 스포너를 그 메뉴의 패널로 옮겨 써 보니, 메뉴가 게임 화면을 가려 불편하다는 사용자 판단이 나왔다. 그래서 디버그 UI를 에디터 전용 도킹 탭으로 되돌렸다.

결론:

- `ProjectKataDebug`와 인게임 메뉴(로컬 플레이어 서브시스템, `Home` 단축키, 입력 모드 전환)를 제거했다.
- `ProjectKataEditor`에 Kata Debug 탭 하나를 두고 치트·스포너 패널을 PIE 월드 문맥으로 그린다.
- SlateIM은 다시 Editor 타깃 전용이다. Dev 빌드에서는 플러그인의 콘솔 치트 명령을 쓴다.

## 변경 또는 진단 내용

| 항목 | 인게임 메뉴(406fcf9 및 미커밋 B4) | 되돌린 결과 |
|---|---|---|
| 모듈 | `ProjectKataDebug`(DeveloperTool), 단독 스포너 탭을 지운 `ProjectKataEditor` 제거안 | `ProjectKataEditor`(Editor)만. `ProjectKataDebug` 삭제 |
| SlateIM 활성 범위 | Shipping·Test 제외 전 구성 | Editor 타깃만(406fcf9 이전과 같음) |
| 호스트 | `UKataDebugMenuSubsystem`(로컬 플레이어마다), 게임 뷰포트 루트 | `FKataDebugTab`(SlateIM 도킹 탭, 에디터 수명 동안 하나) |
| 열기 | `Home`, `Kata.DebugMenu` | Tools > Kata > Kata Debug, `Kata.Debug`. `Kata.RuntimeSpawner`는 탭을 Spawner 패널로 연다 |
| 입력·일시정지 | 열면 Game and UI 입력과 커서, 일시정지 체크박스 | 게임 입력을 건드리지 않음. 일시정지는 PIE 기능을 쓴다 |
| 패널 수명 | 메뉴와 함께 생성·소멸, `OnMenuShutdown` | 에디터 수명 동안 유지(FGCObject), PIE 종료 시 `OnPlaySessionEnded` |
| 스포너 설정 | DefaultGame.ini, 사용자별 Game 설정 | DefaultEditor.ini, EditorPerProjectUserSettings(이전 입력값 복구) |
| 스포너 선택 | 펼침 검색 목록 → 드롭다운 | 단독 탭 시절의 `SGraphActionMenu` 검색·묶음 메뉴 |
| Circle 배치 | 호 길이 간격, 둘레를 넘을 때만 원 전체 | 항상 360도 균등, Spacing은 Line만(사용자 결정, 유지) |
| 치트 확장 | 액터 단위 공개 함수 추가 | 유지. 패널이 같은 함수를 부른다 |

### 진단: SlateIM 위젯 재사용으로 입력값이 깨짐

스포너 패널에서 행 선택 목록을 펼치자 `Distance`에 `1133903872`(float 300.0의 비트를 정수로 읽은 값)가 보이고 `Count`가 `0.0`으로 바뀌었다.

- SlateIM은 같은 부모 안의 순서와 위젯 종류 이름(`GetWidgetClass().GetWidgetType()`)으로 기존 위젯을 재사용한다.
- `SSpinBox<float>`와 `SSpinBox<int32>`는 종류 이름이 같다. 앞쪽 줄이 생기거나 사라져 순서가 밀리면 서로의 위젯을 물려받아 값을 잘못 읽고 쓴다.
- 대응: 섹션과 조건부 줄을 항상 그리는 세로 묶음(`KataDebugLayout::BeginSection`)으로 감쌌다. 정수 입력도 float 입력칸으로 받아 반올림하고, 범위를 벗어난 저장값은 기본값으로 되돌린다. 줄 수가 바뀌는 펼침 목록은 쓰지 않는다.

### 진단: 패널 버튼이 세로로 쌓임

패널 버튼을 `HorizontalWrap`에 넣었더니 한 줄에 하나씩 쌓였다. 크기가 자동인 스크롤 영역 안에서 줄바꿈 기준 폭을 받지 못한 것으로 추정한다. `HorizontalStack`으로 바꿨다. 이후 사용자 요청으로 패널 선택은 드롭다운으로 바꿨다.

### 진단: 단독 문구가 단어마다 줄바꿈됨

PIE 전 치트 패널의 "Start PIE to use cheats."가 단어마다 줄이 바뀌었다. SlateIM 텍스트는 자동 줄바꿈이라 칸을 가로로 채우지 않으면 가장 긴 단어 폭까지 줄어든다. 단독 문구 앞에 `HAlign(HAlign_Fill)`을 지정했다.

## 주요 결정과 이유

- **에디터 탭으로 되돌린다.** 사용자가 인게임 메뉴는 게임 화면을 가려 불편하다고 판단했다. 그 상황에서 에디터 탭 추가와 인게임 메뉴 유지 중 에디터 전용 완전 복귀를 골랐다. 실제로 쓰는 환경도 PIE가 대부분이었다. 샘플 콘텐츠는 쿠킹하지 않아 패키징 빌드에서 다룰 대상이 없다.
- **패널 구조는 유지한다.** `UKataDebugPanel` 파생 클래스를 찾아 목록에 넣는 방식과 공용 대상 선택은 호스트와 무관하게 만들었다. 그래서 호스트만 탭으로 바꿨다.
- **스포너 설정·선택 메뉴는 단독 탭 시절로 되돌린다.** 에디터 전용이 되면 런타임용으로 바꾼 이유가 없다. 원래 설정 위치를 쓰면 이전 입력값도 그대로 복구된다.
- 채택하지 않은 대안: 인게임 메뉴를 유지하고 에디터 탭을 추가하는 안. 호스트 두 개를 유지하는 비용이 커서 택하지 않았다.

## 근거

- [KataDebugTab.cpp](../../Source/ProjectKataEditor/KataDebugTab.cpp): 탭, 패널 생성·전환, 대상 선택, PIE 종료 처리.
- [KataDebugPanel.h](../../Source/ProjectKataEditor/KataDebugPanel.h): 패널 기반 클래스, `KataDebugLayout` 도우미.
- [KataDebugPanel_Spawner.cpp](../../Source/ProjectKataEditor/KataDebugPanel_Spawner.cpp): 섹션 묶음, float 입력칸 Count, 저장값 검사.
- [인게임 메뉴 1차 구현(406fcf9)](https://github.com/jaykop/Kata/tree/406fcf9/Source/ProjectKataDebug): 제거한 호스트와 입력 처리.
- UE 5.8 SlateIM `ISlateIMChild::GetWidget<WidgetType>`: 위젯 종류 이름으로 재사용 여부를 판단한다.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 인게임 메뉴 1차(치트 패널) | 사용자 빌드·PIE 확인(2026-10-11) | 통과 | 패드, Standalone, 패키징 |
| 인게임 메뉴의 스포너 패널 | 사용자 PIE 확인(2026-10-11) | 값 깨짐, 목록 공간, Circle 배치, 버튼 배치 문제 보고 → 수정 | 수정본은 인게임 상태로 미확인 |
| 에디터 탭 복귀 | 사용자 Editor 빌드·PIE 확인(2026-10-11) | 통과. 빈 대상 목록이 투명하게 보이던 문제를 어두운 칸으로 고치고, 다중 대상 선택(체크박스)을 추가한 뒤 확인 | 항목별 상세 결과 미보고 |

에이전트는 빌드·테스트·실행을 하지 않았다.

## 남은 제한과 후속 작업

- [#55](https://github.com/jaykop/Kata/issues/55)의 제목과 범위를 Kata Debug 탭 기준으로 고쳐야 한다. 게시는 사용자 확인 후 한다.
- Dev 빌드에서 마우스 UI가 다시 필요해지면 이 기록의 인게임 메뉴 구현(406fcf9)을 참고한다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [AGENTS.md](../../AGENTS.md) | 샘플 모듈 목록과 SlateIM Editor 타깃 전용 정책으로 복구 |
| [Kata Debug 탭 사용법](../manual/Debug-Tab.md) | 인게임 메뉴 manual을 이름을 바꿔 다시 씀 |
| [런타임 스포너 사용법](../manual/Runtime-Spawner.md) | Kata Debug 탭 패널, 원래 설정 위치, Circle 변경 |
| [치트 사용법](../manual/Cheats.md) | 탭 링크 |
| [인게임 디버그 메뉴 계획(당시)](https://github.com/jaykop/Kata/blob/406fcf9/docs/plan/In-Game-Debug-Menu-Plan.md) | #55 종료와 함께 삭제. 결정 이유는 이 기록으로 옮겼다 |
| [#55](https://github.com/jaykop/Kata/issues/55) | 제목·범위 수정 초안. 게시는 사용자 확인 후 |
