# Kata Debug 탭 사용법

갱신: 2026-10-12  
대상: 샘플 Editor 모듈 `ProjectKataEditor`의 SlateIM 도킹 탭  
적용 기준: [#55](https://github.com/jaykop/Kata/issues/55), [결정 기록](../devlog/2026-10-11-Debug-Tab-Editor-Return.md)  
확인 상태: 2026-10-11 사용자 Editor 빌드·PIE 확인(탭 열기, Panel 드롭다운, 치트 패널, 다중 대상 선택, 스포너 패널, PIE 전 안내 문구). 2026-10-12 사용자 Editor 빌드·PIE 확인(마지막 패널·플레이어 치트 복원, 대상 무적·대미지 무시, 버튼 한 줄 배치, PIE 종료 뒤 안내 문구 배치)

## 목적과 준비

PIE 중 게임 화면을 가리지 않고 옆 탭에서 디버그 기능을 조작한다. 지금은 치트 패널과 [런타임 스포너](Runtime-Spawner.md) 패널이 있다.

- 샘플 프로젝트 전용 에디터 도구다. Kata 플러그인은 이 모듈과 SlateIM을 참조하지 않는다. Game 빌드에는 포함되지 않는다.
- 샘플 `.uproject`가 Experimental 플러그인인 SlateIM을 Editor 타깃에서만 켠다.
- 패널은 PIE 월드의 첫 PlayerController를 대상으로 동작한다. PIE가 아니면 패널은 PIE 시작 안내만 보인다.
- 패키징 빌드나 Dev 빌드에서는 같은 기능을 [치트 콘솔 명령](Cheats.md)으로 쓴다.

## 사용 순서

1. Tools 메뉴의 Kata 섹션에서 `Kata Debug`를 연다. 콘솔 명령 `Kata.Debug`로도 열고 닫는다.
2. 탭 위쪽의 `Panel` 드롭다운에서 패널을 고른다. `Kata.RuntimeSpawner`는 탭을 Spawner 패널로 바로 연다.
3. PIE를 시작하고 패널을 쓴다. 탭은 게임 입력 모드나 커서를 건드리지 않으므로, 게임 화면과 탭을 오가며 조작한다.

## 탭 구성

| 항목 | 동작 |
|---|---|
| Panel | 패널을 고르는 드롭다운. `Kata.RuntimeSpawner`처럼 바깥에서 패널을 바꾸면 표시도 따라 바뀐다. 마지막으로 고른 패널을 저장해 에디터를 다시 열어도 그 패널로 연다 |
| Target | 대상 선택을 쓰는 패널 위에 PIE 중에만 보인다. 목록의 체크박스로 여러 대상을 고른다. `Self`와 `Lock-On Target`(PC 타게팅 컴포넌트의 현재 대상)은 선택을 그 하나로 바꾸고, `All`은 목록 전체를 더하며, `Clear`는 선택을 비운다. 목록은 ASC를 가진 살아 있는 Pawn이며 0.5초마다 거리순으로 갱신한다. 선택은 액터 기준으로 기억하므로 정렬이 바뀌어도 유지된다. 목록 칸은 항상 160px 높이의 어두운 칸이며 넘치면 안에서 스크롤한다. 대상이 없으면 칸 안에 안내 문구가 보인다. PIE가 끝나면 선택을 비운다 |

### 치트 패널

| 항목 | 동작 |
|---|---|
| Player | `KataGod`, `KataNoDamage`, `KataInfiniteStamina`, `KataHideFromAI` 체크박스. 콘솔에서 바꾼 값도 그대로 보인다. 패널이 보이는 동안의 값을 저장하고 다음 PIE가 시작되면 탭이 닫혀 있어도 다시 건다(`LogKataDebugCheats`에 한 줄 남긴다) |
| Kill Self, Respawn | PC 사망, 지연 없는 재생성 |
| Selected Targets, All AI | 고른 대상 전부에 Kill, Groggy, Pause AI·Resume AI, 무적(Make·Remove Invincible), 대미지 무시(Ignore·Take Damage)를 적용한다. Pause AI는 하나라도 실행 중이면 모두 정지, 모두 정지해 있으면 모두 재개한다. 무적·대미지 무시도 같은 규칙이다. 대미지 무시는 피격 반응이 그대로라 피격 확인용 적에 쓴다. All AI는 모든 AI 정지·재개. 마지막 동작 결과(적용된 수)가 아래에 보인다 |

명령별 상세 동작과 제한은 [치트 사용법](Cheats.md)을 따른다.

### Spawner 패널

PIE 중 플레이어 정면에 NPC를 생성한다. 사용법은 [런타임 스포너 사용법](Runtime-Spawner.md)을 따른다.

## 패널 추가

1. `ProjectKataEditor` 모듈에 `UKataDebugPanel`을 상속한 클래스를 만든다.
2. `GetDisplayName`과 `Draw`를 구현한다. 필요하면 `GetSortOrder`, `UsesTargetSelection`, `OnShown`, `OnHidden`, `OnPlaySessionStarted`, `OnPlaySessionEnded`를 덮어쓴다.
3. 탭을 처음 열거나 PIE가 시작될 때 비추상 파생 클래스를 모두 찾아 목록에 넣으므로 등록 코드는 필요 없다.

- 패널 인스턴스는 에디터 수명 동안 유지된다. PIE 세션에 묶인 기록(생성한 액터, 반복 실행, 지연 콜백)은 `OnPlaySessionEnded`에서 비운다.
- 저장한 상태를 새 PIE 월드에 다시 걸 때는 `OnPlaySessionStarted`를 쓴다. 탭이 닫혀 있어도 `PostPIEStarted` 알림에서 첫 PlayerController를 전달하며, 없으면 nullptr이다. CheatManager나 치트 확장을 쓰는 패널은 각 객체의 존재를 확인한다. 에디터를 다시 열어도 남길 값은 config 클래스(예: `UKataDebugUserSettings`)에 저장한다.
- `Draw`는 SlateIM 세로 스택 안에서 매 프레임 불린다. 문맥의 월드와 PlayerController는 PIE가 아니면 비어 있다.
- 섹션과 조건부 줄은 `KataDebugLayout::BeginSection`·`EndSection`으로 감싸고, 섹션 제목은 `SectionHeader`를 쓴다.
- 한 줄 문구를 단독으로 그릴 때는 `SlateIM::HAlign(HAlign_Fill)`을 먼저 지정한다. SlateIM 텍스트는 자동 줄바꿈이라 칸을 채우지 않으면 가장 긴 단어 폭까지 줄어 단어마다 줄이 바뀐다.
- SlateIM은 같은 부모 안의 순서와 위젯 종류 이름으로 위젯을 재사용하고, `SSpinBox<float>`와 `SSpinBox<int32>`를 같은 종류로 본다. 앞쪽 줄이 생기거나 사라져 순서가 밀리면 다른 입력칸의 위젯을 물려받아 값이 깨진다.
  조건부 줄은 섹션 마지막에 두거나 항상 그리는 섹션 묶음으로 감싸고, 정수 입력도 float 입력칸으로 받아 반올림한다. 줄 수가 바뀌는 펼침 목록 대신 한 위젯으로 끝나는 메뉴·드롭다운을 쓴다.
- SlateIM은 슬롯 정렬의 변경 여부를 루트 전체의 위젯 일련번호로 비교한다. 앞쪽 대상 목록의 줄 수가 바뀌면 일련번호가 밀려, 패널 내용이 바뀔 때 이전 위젯의 정렬(예: 좁은 제목 정렬)이 남을 수 있다.
  탭은 패널이나 첫 PlayerController의 존재 여부가 바뀐 프레임에 내용 컨테이너 종류를 바꿔 패널 영역을 새로 만든다. 패널 안에서 내용이 통째로 바뀌는 경우에도 같은 증상이 보이면 알린다.
- 큰 목록은 패널에 캐시해 두고 바뀔 때만 다시 만든다.
- 패널은 Kata 플러그인의 공개 API와 치트 확장만 부른다. 화면에 띄워 두기만 하는 상태 표시는 해당 플러그인의 GameplayDebugger 카테고리가 맞다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 패널에 PIE 시작 안내만 보인다 | PIE가 실행 중이 아니다 | PIE를 시작한다 |
| 치트 패널에 사용 불가 문구가 보인다 | PIE PlayerController에 CheatManager가 없다 | 치트를 허용하는 실행 환경인지 확인한다 |
| 에디터를 닫을 때 탭 창이 잠깐 떴다 닫힌다 | 메인 창이 탭째로 파괴될 때 탭 닫힘 알림이 오지 않는다 | 에디터 종료 이벤트에서 그리기만 멈추도록 처리했다. 증상이 다시 보이면 알린다 |
| Dev 빌드에서 탭을 쓸 수 없다 | 에디터 전용 도구다 | 콘솔 치트 명령을 쓴다 |

## 확인 상태와 근거

2026-10-11 사용자가 에디터 탭으로 되돌린 뒤 Editor 빌드와 PIE에서 치트 패널, 다중 대상 선택, 스포너 패널이 동작함을 보고했다. 항목별 상세 결과는 보고되지 않았다.

- [KataDebugTab.cpp](../../Source/ProjectKataEditor/KataDebugTab.cpp): 탭, 패널 목록, 대상 선택, PIE 종료 처리.
- [KataDebugPanel.h](../../Source/ProjectKataEditor/KataDebugPanel.h): 패널 기반 클래스와 섹션 도우미.
- [KataDebugUserSettings.h](../../Source/ProjectKataEditor/KataDebugUserSettings.h): 마지막 패널과 플레이어 치트 토글 저장.
- [KataDebugPanel_Cheats.cpp](../../Source/ProjectKataEditor/KataDebugPanel_Cheats.cpp): 치트 패널.
- [KataDebugPanel_Spawner.cpp](../../Source/ProjectKataEditor/KataDebugPanel_Spawner.cpp): 스포너 패널.
- [작업 상태 #55](https://github.com/jaykop/Kata/issues/55).
