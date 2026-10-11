# 인게임 디버그 메뉴 계획

작성: 2026-10-11  
갱신: 2026-10-11  
연결 이슈: [#55 인게임 디버그 메뉴 (SlateIM, 샘플 프로젝트)](https://github.com/jaykop/Kata/issues/55)  
현재 상태 근거: UE 5.8 SlateIM 플러그인 소스, [런타임 스포너 도구 기록](../devlog/2026-10-10-Runtime-Spawner-Tool.md), [런타임 스포너 사용법](../manual/Runtime-Spawner.md)  
대체 관계: 없음

## 목적과 현재 상태

PIE와 Dev 빌드에서 치트·스포너 같은 디버그 기능을 게임 화면 위의 패널로 조작하고 싶다. 앞으로 UI가 필요한 디버그 기능도 같은 곳에 패널을 더해 넣는다.

현재 상태는 다음과 같다.

- 런타임 스포너는 샘플 Editor 모듈 `ProjectKataEditor`의 SlateIM 도킹 탭이다. 에디터가 있어야 열리므로 에디터에서 띄운 Standalone Game(`-game`)이나 패키징 빌드에서는 쓸 수 없다.
- SlateIM은 엔진 기준으로 런타임에서 쓸 수 있다. 네 모듈이 모두 Runtime 타입이고, `SlateIM::BeginViewportRoot(ULocalPlayer*)`로 게임 뷰포트 위에 그린다. 에디터 뷰포트용 오버로드만 `WITH_EDITOR`다.
- 2026-10-11에 SlateIM 사용 정책을 바꿨다. 샘플에서만 켜고 Shipping·Test 구성에서는 끄며, 인게임 UI는 샘플 DeveloperTool 모듈에 둔다([AGENTS.md](../../AGENTS.md)).

치트 로직은 [치트 매니저 계획](Cheat-Manager-Plan.md)에서 플러그인별로 만든다. 이 계획은 그 위의 UI와 스포너 이전을 다룬다.

## 범위

- 포함
  - 샘플 DeveloperTool 모듈과 메뉴 호스트: 열기·닫기, 입력 모드·커서 전환, 패널 목록과 전환.
  - 패널 확장 규격과 공용 대상 선택.
  - 치트 패널.
  - 런타임 스포너를 패널로 이전하고 에디터 탭을 없앤다.
- 제외
  - 치트 로직 자체. [치트 매니저 계획](Cheat-Manager-Plan.md)이 맡는다.
  - Kata 플러그인에서 패널을 제공하는 구조. 플러그인은 SlateIM을 참조하지 않는다.
  - GAS Inspector의 인게임판, 표시 전용 오버레이(GameplayDebugger 카테고리로 충분하다).
  - 패드 조작 지원. 확인 전이며 후속 검토로 둔다.

## 구조

### 모듈

- 이름 제안: `ProjectKataDebug`. 타입 `DeveloperTool`이라 Editor 타깃과 Game 타깃의 Debug·Development 구성에서만 빌드되고 Test·Shipping에서는 빠진다.
- `.uproject`의 SlateIM 참조는 `TargetAllowList: ["Editor"]`를 빼고 `TargetConfigurationDenyList: ["Shipping", "Test"]`를 둔다.
- 의존: SlateIM, Slate·SlateCore·InputCore, GAS, 그리고 패널이 쓰는 Kata 런타임 모듈. 다른 모듈은 이 모듈에 의존하지 않는다.
- 엔진 `SlateIMInGame`은 쓰지 않는다. 위젯 베이스가 복제 액터이고 Server RPC로 닫히므로 프로젝트 규칙과 맞지 않는다. SlateIM 코어를 직접 쓰고, 그 베이스와 같은 방식으로 `FSlateApplication::OnPreTick`에서 그린다.

### 메뉴 호스트

- `ULocalPlayerSubsystem` 하나가 메뉴 상태를 가진다. PIE 인스턴스와 Standalone 모두 로컬 플레이어마다 생기고, 월드와 함께 정리된다.
- 열기·닫기: 콘솔 명령과 단축키. 단축키는 Slate 입력 전처리기(`IInputProcessor`)로 받아 게임 입력 에셋과 섞지 않는다.
- 열면 마우스 커서를 보이고 `GameAndUI` 입력 모드로 바꾼다. 닫으면 열기 전의 커서 표시 상태와 게임 입력 모드로 되돌린다. SlateIM은 이 처리를 하지 않는다.
- 레이아웃: 한쪽에 패널 목록, 나머지에 선택한 패널 내용. 패널 전환 방식(버튼 열·탭)은 구현 때 정한다.

### 패널 확장 규격

새 디버그 기능은 패널 클래스 하나를 더해서 넣는다.

- `UKataDebugPanel`(가칭) UObject 기반 클래스를 상속한다. 호스트가 시작할 때 파생 클래스를 찾아 로컬 플레이어마다 인스턴스를 만든다. 등록 코드가 필요 없고, 패널이 잡는 UObject 참조를 GC가 추적한다.
- 패널이 구현하는 것: 표시 이름·순서, 매 프레임 그리기, 열기·닫기·월드 정리 알림.
- 호스트가 넘기는 문맥: 월드, PlayerController, 공용 대상 선택.
- 공용 대상 선택: 월드의 ASC 보유 액터를 거리순 목록으로 보여 주고, `Self`·현재 타게팅 대상·목록에서 고른 액터 중 하나를 선택 상태로 둔다. 치트 패널과 이후 패널이 같은 선택을 쓴다.
- 패널은 플러그인의 공개 API와 치트 함수만 부른다. 플러그인 쪽 상태를 보여 줘야 하면 플러그인이 SlateIM 없는 조회 API를 내놓고 패널이 그린다.

## 확장성 진단

UI가 필요한 디버그 기능을 이 모듈에서 계속 확장할 수 있는지 항목별로 본 결과다.

| 항목 | 판단 | 근거와 대응 |
|---|---|---|
| 조작형 디버그 기능 추가 | 가능 | 패널 클래스 하나로 추가한다. 호스트·입력·대상 선택은 공용이다 |
| 플러그인이 패널을 직접 제공 | 불가 | 플러그인은 SlateIM을 참조하지 않는다. 플러그인은 조회·명령 API만 내놓는다. 다른 프로젝트에서 재사용이 필요해지면 호스트를 플러그인으로 옮기는 정책 결정을 다시 한다 |
| 패키징한 Dev 빌드에서 사용 | 조건부 | 코드는 동작한다. 하지만 샘플 콘텐츠가 모두 `/Game/KataSample`에 있고 쿠킹 제외(`DirectoriesToNeverCook`)라서, 지금 패키징 빌드에는 패널이 다룰 테이블·캐릭터가 없다. 당장 실사용 범위는 PIE와 Standalone Game이다 |
| 에디터 전용 위젯 | 대체 필요 | `SGraphActionMenu`, 디테일 뷰, 에셋 피커는 쓸 수 없다. SlateIM 기본 위젯(입력란, 목록, 표, 콤보)으로 만든다 |
| 큰 목록 | 주의 | 즉시 모드라 매 프레임 그린다. 에셋 목록처럼 큰 데이터는 지금 스포너처럼 캐시해 두고 바뀔 때만 다시 만든다 |
| 표시 전용 정보 | 다른 경로 | 화면에 띄워 두는 상태 표시는 해당 플러그인의 GameplayDebugger 카테고리가 맞다(KataCamera 선례). 이 모듈은 조작 UI를 맡는다 |
| 패드 조작 | 미확인 | SlateIM 위젯이 Slate 내비게이션으로 패드 조작되는지 확인하지 않았다. 마우스 조작을 기준으로 시작한다 |

## 런타임 스포너 이전

스포너의 배치 계산·생성 요청·반복 실행 로직은 그대로 옮긴다. 에디터에 묶인 부분만 바꾼다.

| 현재 에디터 의존 | 인게임 대체 |
|---|---|
| `FSlateIMNomadTabBase` 도킹 탭, Tools 메뉴 항목 | 메뉴 패널. 콘솔 명령 `Kata.RuntimeSpawner`는 메뉴를 스포너 패널로 연다 |
| `GEditor->PlayWorld` | 호스트 문맥의 월드 |
| `FEditorDelegates::EndPIE`, `OnEditorClose` 우회 | 서브시스템 정리와 월드 정리 알림. 도킹 탭 재생성 문제는 사라진다 |
| `SGraphActionMenu` 선택 메뉴(GraphEditor 모듈) | SlateIM 입력란으로 거른 목록. 폴더 묶음은 표의 하위 행으로 보인다 |
| `UKataRuntimeSpawnerSettings`(`config=Editor`, DefaultEditor.ini) | `config=Game`(DefaultGame.ini). 모듈이 바뀌어 ini 섹션 이름이 달라지므로 기존 항목을 옮긴다 |
| `UKataRuntimeSpawnerUserSettings`(EditorPerProjectUserSettings) | 사용자별 Game 설정. 기존 마지막 입력값은 한 번 사라진다 |
| 설정 열기 버튼(`ISettingsModule`) | `WITH_EDITOR`일 때만 보인다 |
| AssetRegistry 카탈로그, 행 로드 | 런타임에서도 동작한다. 패키징 빌드에서는 쿠킹된 에셋만 보인다 |

설정 클래스는 config 전용이고 에셋에 직렬화되지 않으므로 클래스 Redirect는 필요 없다. ini 섹션만 옮긴다.

## 확정 사항과 미확정 사항

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| UI 위치 | 확정 | 샘플 DeveloperTool 모듈(2026-10-11 사용자 결정) |
| SlateIM 정책 | 확정 | 샘플 전용, Shipping·Test 제외, `SlateIMInGame` 미사용([AGENTS.md](../../AGENTS.md)) |
| 이슈 분할 | 확정 | 치트와 인게임 UI를 나눈다. 스포너 이전은 이 이슈에 포함한다(2026-10-11 사용자 결정) |
| 모듈 이름 | 제안 | `ProjectKataDebug` |
| 열기 단축키 | 확정 | `Home`. 패드 조합은 패드 조작 검토 때 정한다(2026-10-11 사용자 결정) |
| 메뉴를 연 동안 게임 일시정지 | 확정 | 메뉴의 체크박스로 고르며 기본은 끔(2026-10-11 사용자 결정) |
| `ProjectKataEditor`의 처리 | 확정 | 스포너를 옮기면 모듈을 제거하고, 에디터 도구가 다시 필요할 때 만든다(2026-10-11 사용자 결정) |
| 패키징 Dev 빌드의 콘텐츠 | 보류 | 이번 이슈는 PIE와 Standalone Game을 대상으로 한다. 패키징 빌드에서 쓸 콘텐츠 위치는 별도로 정한다(2026-10-11 사용자 결정) |
| 에디터 도킹 탭 호스트 | 보류 | 같은 패널을 에디터 탭에서 PIE 월드 문맥으로 그리는 두 번째 호스트. 패널은 호스트와 무관하게 만들어 두었으므로 나중에 붙일 수 있다. 인게임 메뉴를 써 보고 커서 전환·화면 가림이 불편하면 추가한다(2026-10-11 사용자 결정) |

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| B1 | 높음 | 모듈 추가, `.uproject` SlateIM 참조 변경, 메뉴 호스트(열기·닫기, 입력 모드, 일시정지 체크박스, 패널 목록) | 없음 | PIE와 Standalone Game에서 메뉴가 열리고 닫히며, 닫으면 게임 조작이 원래대로 돌아온다. Shipping 구성에 모듈이 없다 |
| B2 | 높음 | 패널 확장 규격과 공용 대상 선택 | B1 | 빈 시험 패널을 클래스 하나로 추가해 목록에 보인다 |
| B3 | 높음 | 치트 패널 | B2, #54 A1~A5 | 패널 버튼과 토글이 콘솔 명령과 같은 결과를 낸다 |
| B4 | 보통 | 스포너 패널 이전, 에디터 탭과 `ProjectKataEditor` 모듈 제거 | B2 | 기존 스포너 기능(배치 패턴, AI 모드, 반복, 일괄 제거, 입력값 유지)이 패널에서 동작한다 |
| B5 | 보통 | 문서 정리 | B1~B4 | 새 manual, 스포너 manual 갱신, AGENTS.md 모듈 목록 반영 |

## 영향과 제한

- 모듈 경계: Kata 플러그인 코드는 이 작업으로 바뀌지 않는다. 패널은 플러그인 공개 API만 쓴다.
- 입력: 메뉴를 연 동안 마우스 입력은 UI가 먼저 받는다. 게임 입력이 동시에 들어가는지는 `GameAndUI` 모드의 기본 동작을 따르며, 실제 조작감은 사용자 확인이 필요하다.
- 스포너 반복 실행은 지금처럼 `FTSTicker`의 실시간을 쓴다. 게임 일시정지나 시간 배율의 영향을 받지 않는다.
- SlateIM은 Experimental이라 엔진 버전이 바뀌면 API가 바뀔 수 있다. 사용 범위가 샘플 모듈 하나라 영향은 그 모듈에 그친다.

## 사용자 확인 항목

- 구현 완료: B1~B4의 코드가 들어가고, Shipping·Test 구성에서 모듈과 SlateIM이 빠진다.
- 실행 확인(사용자): PIE와 Standalone Game에서 메뉴 열기·닫기, 입력 복원, 치트 패널, 스포너 패널의 기존 기능.
- 아직 확인하지 않은 범위: 패키징 Development 빌드, 패드 조작.
- 이 목록만으로 에이전트가 빌드·테스트를 하지 않는다.

## 완료 시 갱신할 문서

- 연결 이슈: 구현·확인 상태.
- 새 manual `In-Game-Debug-Menu.md`: 여는 법, 패널 목록, 패널 추가 방법.
- [런타임 스포너 사용법](../manual/Runtime-Spawner.md): 에디터 탭에서 메뉴 패널로 바뀐 사용법.
- [AGENTS.md](../../AGENTS.md): 샘플 모듈 목록과 `ProjectKataEditor` 처리 결과.
- devlog: SlateIM 런타임 허용과 `SlateIMInGame`을 쓰지 않은 이유, 스포너 이전 결정.
- [문서 목록](../README.md): 새 manual 링크.
