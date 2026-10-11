# 인게임 디버그 메뉴 사용법

갱신: 2026-10-11  
대상: 샘플 DeveloperTool 모듈 `ProjectKataDebug`  
적용 기준: [#55](https://github.com/jaykop/Kata/issues/55), [인게임 디버그 메뉴 계획](../plan/In-Game-Debug-Menu-Plan.md)  
확인 상태: 2026-10-11 사용자 Editor 빌드·PIE 확인(열기·닫기, 입력 복원, 일시정지, 치트 패널, 대상 선택, 에디터 창 단축키 무시). 패드·Standalone·패키징 빌드 미확인

## 목적과 준비

PIE와 Standalone Game의 게임 화면 위에서 디버그 기능을 마우스로 조작한다. 지금은 치트 패널이 있으며, 런타임 스포너는 후속 단계에서 패널로 옮긴다.

- 샘플 프로젝트 전용이다. Kata 플러그인은 이 모듈과 SlateIM을 참조하지 않는다.
- `ProjectKataDebug`는 DeveloperTool 모듈이라 Editor 타깃과 Game 타깃의 Debug·Development 구성에서만 빌드된다. Test·Shipping 구성에는 들어가지 않는다.
- 샘플 `.uproject`가 Experimental 플러그인인 SlateIM을 Shipping·Test 구성만 빼고 켠다.
- 치트 패널은 PlayerController에 CheatManager가 있어야 한다. PIE와 Standalone Game에서는 기본으로 만들어진다.

## 사용 순서

1. PIE나 Standalone Game을 시작하고 게임 화면을 클릭해 포커스를 준다.
2. `Home` 키를 누르거나 콘솔에서 `Kata.DebugMenu`를 입력한다. 화면 왼쪽 위에 메뉴가 열리고 마우스 커서가 보인다.
3. 패널의 버튼과 체크박스를 쓴다. 메뉴가 열린 동안에도 캐릭터 조작은 받는다.
4. 다시 `Home`을 누르거나 `Close`를 누르면 닫힌다. 커서 표시는 열기 전 상태로, 입력은 게임 입력 모드로 돌아간다.

## 메뉴 구성

| 항목 | 동작 |
|---|---|
| Pause Game | 켜면 메뉴를 연 동안 게임을 일시정지한다. 기본은 끔이다. 메뉴가 건 일시정지만 닫을 때 푼다 |
| 패널 버튼 | 패널이 둘 이상일 때 보인다. 누르면 그 패널로 바뀐다 |
| Target | 대상 선택을 쓰는 패널 위에 보인다. `Self`, `Lock-On Target`(PC 타게팅 컴포넌트의 현재 대상), `Clear` 버튼과 거리순 목록으로 고른다. 목록은 ASC를 가진 살아 있는 Pawn이며 0.5초마다 갱신한다 |

### 치트 패널

| 항목 | 동작 |
|---|---|
| Player 체크박스 | `KataGod`, `KataNoDamage`, `KataInfiniteStamina`, `KataHideFromAI`와 같다. 콘솔에서 바꾼 값도 그대로 보인다 |
| Kill Self, Respawn | PC 사망, 지연 없는 재생성 |
| Selected Target | 고른 대상의 Kill, Groggy, Pause AI·Resume AI |
| All AI | 모든 AI 정지·재개 |

명령별 상세 동작과 제한은 [치트 사용법](Cheats.md)을 따른다.

## 패널 추가

1. `ProjectKataDebug` 모듈에 `UKataDebugPanel`을 상속한 클래스를 만든다.
2. `GetDisplayName`과 `Draw`를 구현한다. 필요하면 `GetSortOrder`, `UsesTargetSelection`, `OnShown`, `OnHidden`을 덮어쓴다.
3. 메뉴가 시작할 때 비추상 파생 클래스를 모두 찾아 목록에 넣으므로 등록 코드는 필요 없다.

- `Draw`는 SlateIM 세로 스택 안에서 매 프레임 불린다. 큰 목록은 패널에 캐시해 두고 바뀔 때만 다시 만든다.
- 패널은 Kata 플러그인의 공개 API와 치트 확장만 부른다. 플러그인 상태를 보여 줘야 하면 플러그인에 SlateIM 없는 조회 API를 추가한다.
- 화면에 띄워 두기만 하는 상태 표시는 해당 플러그인의 GameplayDebugger 카테고리가 맞다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| `Home`을 눌러도 열리지 않는다 | 게임 뷰포트에 포커스가 없다 | 게임 화면을 클릭한 뒤 누르거나 `Kata.DebugMenu`를 쓴다 |
| 닫은 뒤 다른 UI의 입력 모드가 풀렸다 | 열기 전 입력 모드는 엔진에서 읽을 수 없어 게임 입력 모드로 되돌린다 | 그 UI를 다시 연다 |
| 패드로 메뉴를 조작할 수 없다 | 패드 조작은 확인하지 않았다 | 마우스로 조작한다 |
| 패키징한 Development 빌드에서 다룰 대상이 없다 | 샘플 콘텐츠(`/Game/KataSample`)는 쿠킹하지 않는다 | PIE나 Standalone Game에서 쓴다 |
| 치트 패널에 사용 불가 문구가 보인다 | PlayerController에 CheatManager가 없다 | 치트를 허용하는 실행 환경인지 확인한다 |

## 확인 상태와 근거

2026-10-11 사용자가 Editor 빌드 후 PIE에서 위 사용 순서와 치트 패널 동작을 확인했다. 패드 조작, Standalone Game, 패키징한 Development 빌드는 확인하지 않았다.

- [KataDebugMenuSubsystem.cpp](../../Source/ProjectKataDebug/KataDebugMenuSubsystem.cpp): 열기·닫기, 입력 모드, 일시정지, 대상 선택.
- [KataDebugMenuInputProcessor.cpp](../../Source/ProjectKataDebug/KataDebugMenuInputProcessor.cpp): `Home` 단축키.
- [KataDebugPanel.h](../../Source/ProjectKataDebug/KataDebugPanel.h): 패널 기반 클래스.
- [KataDebugPanel_Cheats.cpp](../../Source/ProjectKataDebug/KataDebugPanel_Cheats.cpp): 치트 패널.
- [작업 상태 #55](https://github.com/jaykop/Kata/issues/55).
