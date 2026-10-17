# 시간대 사용법

갱신: 2026-10-17  
대상: KataWorld의 시간 상태(`UKataWorldTimeSubsystem`, `AKataLevelTimeSettings`, Kata World 설정, 시간 치트)  
적용 기준: [#124](https://github.com/jaykop/Kata/issues/124), 상위 작업 [#127](https://github.com/jaykop/Kata/issues/127)  
확인 상태: 2026-10-17 사용자 에디터 빌드 통과. 같은 날 사용자 PIE에서 `LV_TestMap`의 레벨 설정(Fixed, 21시) 적용과 `KataTime` 출력을 확인했다. 시간 흐름, 시간대 변경 알림, 나머지 치트와 경계 동작은 실행 확인 전이다

## 목적과 준비

게임 안의 현재 시각과 시간대를 한 곳에서 관리한다. 낮밤 연출([#125](https://github.com/jaykop/Kata/issues/125))과 날씨([#126](https://github.com/jaykop/Kata/issues/126)), 이후의 게임플레이 연결이 이 상태를 읽는다.
지금은 상태만 제공하므로 태양이나 하늘 같은 화면은 바뀌지 않는다.

- `KataWorld` 플러그인을 켠다. 샘플 프로젝트는 켜 두었다.
- 시간대 태그는 프로젝트가 `TimeOfDay` 루트 아래에 정의한다. 샘플은 `Config/Tags/TimeOfDay.ini`에 `TimeOfDay.Dawn`·`TimeOfDay.Day`·`TimeOfDay.Dusk`·`TimeOfDay.Night`를 둔다.
- 시간 Subsystem은 게임·PIE 월드에서만 만들어진다. 편집 중인 레벨과 에디터 프리뷰 월드에는 없다.
- 싱글플레이 전용이며 복제하지 않는다. 세이브에 저장하지 않으므로 레벨을 새로 열면 시작 값부터 다시 흐른다.

## 사용 순서

1. 프로젝트 설정 > Plugins > Kata World에서 Default Hour, Day Length Minutes, Time Phases를 정한다. 샘플은 하루 24분, 새벽 5시·낮 7시·해질녘 17시·밤 19시 구간이다.
2. 레벨마다 시작 시각이나 흐름을 다르게 하려면 레벨에 Kata Level Time Settings 액터를 하나 배치하고 Time Mode와 Initial Hour를 정한다. 하루 길이도 바꾸려면 Day Length Minutes 왼쪽 체크를 켜고 값을 정한다.
3. Blueprint나 C++에서 `UKataWorldTimeSubsystem`을 얻어 Get Time Of Day, Get Time Phase를 읽는다. 시간대 변화는 On Time Phase Changed에 바인딩해 받는다.
   바인딩하기 전에 정해진 시간대는 알리지 않으므로, 바인딩 직후 Get Time Phase로 현재 값을 먼저 읽는다.
4. PIE에서 [시간 치트](#시간-치트)로 시각과 흐름을 확인한다.

C++에서는 `UKataWorldTimeSubsystem::Get(WorldContext)`로 얻고 `OnTimePhaseChangedNative`를 구독할 수 있다. 소스 기준이며 C++ 구독 예제는 실행 확인 전이다.

## 주요 설정과 실행 계약

### Kata World 설정

| 항목 | 의미·입력 | 기본값·빈 값 |
|---|---|---|
| Default Hour | 레벨 설정이 없을 때 시작하는 시각(시). 0~24, 24는 0시 | 9 |
| Day Length Minutes | 게임 하루(24시간)가 지나는 실제 시간(분). 0.1 이상 | 24. 레벨 설정이 덮어쓰지 않으면 이 값을 쓴다 |
| Time Phases | 시간대 구간 목록. 항목마다 Start Hour와 Phase Tag(`TimeOfDay` 아래) | 비어 있으면 시간대를 알리지 않고 Get Time Phase는 빈 태그다 |

- 구간은 입력 순서와 상관없이 Start Hour 순으로 정렬해 쓴다. 한 구간은 다음 구간의 Start Hour 전까지 이어진다.
- 첫 구간보다 이른 시각은 전날부터 이어진 마지막 구간에 속한다. 샘플에서 3시는 `TimeOfDay.Night`다.
- Phase Tag가 없는 구간은 PIE 시작 때 경고(`LogKataWorld`)를 남기고 뺀다.

### Kata Level Time Settings

| 항목 | 의미·입력 | 기본값·실패 시 동작 |
|---|---|---|
| Time Mode | Flowing은 하루 길이에 맞춰 흐르고, Fixed는 Initial Hour에 멈춘다 | Flowing |
| Initial Hour | 레벨을 시작하는 시각. Fixed에서는 고정 시각이다 | 9 |
| Day Length Minutes | 왼쪽 체크를 켜면 이 레벨에서만 프로젝트 설정 대신 쓴다 | 체크 꺼짐, 24 |

- BeginPlay에서 시간 Subsystem에 등록할 때 값을 한 번 읽는다. 실행 중에 바꾸려면 Subsystem 함수를 쓴다.
- 한 월드에서는 먼저 등록된 설정 하나만 쓴다. 두 번째부터는 `Level time settings ... ignored` 경고를 남기고 무시한다.
- 등록된 액터의 EndPlay에서 모드는 Flowing, 하루 길이는 프로젝트 설정으로 돌아가고 현재 시각은 유지한다. 등록이 거절된 액터의 EndPlay는 시간 상태를 바꾸지 않는다.

### 시간 Subsystem

| API | 의미·입력 | 계약 |
|---|---|---|
| Get Time Of Day | 현재 시각(시) | 0 이상 24 미만 |
| Set Time Of Day | 시각을 바로 바꾼다 | 0 이상 24 미만으로 정규화한다(26 → 2, -1 → 23, 24 → 0). 두 모드에서 모두 쓸 수 있고 모드와 Day Count는 바꾸지 않는다. 유한하지 않은 값은 경고 후 무시 |
| Get / Set Time Mode | Flowing·Fixed | 현재 시각은 유지한다 |
| Get / Set Day Length Minutes | 하루 길이(실제 분) | 0.1분보다 짧으면 0.1분으로 올린다. 유한하지 않은 값은 경고 후 무시 |
| Get Time Phase | 현재 시간대 태그 | 구간이 없으면 빈 태그 |
| Get Day Count | 이 월드가 시작된 뒤 시간이 흘러 0시를 지난 횟수 | 시작한 날이 0 |
| On Time Phase Changed | 시간대가 바뀔 때 Old Phase, New Phase를 알린다 | C++ `OnTimePhaseChangedNative`가 먼저 불린다. 한 Tick에 여러 구간을 건너뛰어도 이전 태그에서 최종 태그로의 변경만 알리며, 두 태그가 같으면 알리지 않는다 |

시계는 다음 규칙으로 흐른다.

- 게임 1시간은 실제로 Day Length Minutes × 60 ÷ 24초다. 기본 24분이면 1분이다.
- 시간 배율의 영향을 받지 않는다. `slomo`나 [시간 배율 요청](Time-Dilation.md)으로 월드가 느려져도 하루는 같은 속도로 흐른다.
- 엔진 일시정지 중에는 멈춘다.
- 흘러서 24시를 넘으면 0시로 돌아가고 Day Count가 1 늘어난다. 긴 프레임 지연으로 하루 이상 지나도 지난 날짜를 모두 센다.

### 시간 치트

PIE 뷰포트를 클릭하고 `` ` `` 키로 연 콘솔이나 Output Log의 Cmd 입력란에 입력한다. 결과는 콘솔과 `LogKataWorld` 로그에 한 줄로 남는다. Shipping에서는 쓸 수 없다.

| 명령 | 인자 | 동작 |
|---|---|---|
| `KataTime [시각]` | 비우면 조회, 숫자면 그 시각으로 설정 | 결과 예: `KataTime 21.00 h, phase TimeOfDay.Night, Fixed, day length 24.0 min, day 0` |
| `KataTimeFlow [0\|1]` | 토글 | 1이면 Flowing, 0이면 Fixed. 비우면 반전 |
| `KataDayLength <분>` | 실제 분 | 하루 길이를 바꾼다. 0.5면 하루가 30초다 |

다른 Kata 치트는 [치트 사용법](Cheats.md)에 있다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 시각이 바뀌어도 태양·하늘이 그대로다 | 연출은 아직 없다 | 낮밤 연출 [#125](https://github.com/jaykop/Kata/issues/125)를 기다린다 |
| `KataTime`을 입력해도 결과가 없다 | Console Variables Editor 창에 입력하면 `not a recognized console variable or command` 경고만 남는다 | PIE 뷰포트 콘솔이나 Output Log의 Cmd에 입력하고 `LogKataWorld`를 본다 |
| 시간이 흐르지 않는다 | 레벨 설정이 Fixed이거나 게임이 일시정지 중이다 | `KataTime`으로 모드를 확인하고 `KataTimeFlow 1`로 켠다 |
| Fixed인데 시각이 바뀐다 | 시각 지정 함수와 `KataTime`은 두 모드에서 모두 동작한다 | 의도한 동작이다. 흐름만 멈춘다 |
| 시간대가 `None`이다 | Time Phases가 비었거나 모든 구간에 태그가 없다 | Kata World 설정에 구간과 태그를 넣는다 |
| 레벨 설정이 무시된다 | 같은 월드에 설정 액터가 둘 이상 있다 | 하나만 남긴다 |

## 확인 상태와 근거

소스 기준 동작은 아래 파일에서 확인했다. 사용자 실행 확인 범위는 문서 머리의 확인 상태를 따른다.

- [KataWorldTimeSubsystem.h](../../Plugins/KataWorld/Source/KataWorld/Public/Time/KataWorldTimeSubsystem.h): 시계·시간대·알림 계약.
- [KataLevelTimeSettings.h](../../Plugins/KataWorld/Source/KataWorld/Public/Time/KataLevelTimeSettings.h): 레벨 단위 설정과 등록 규칙.
- [KataWorldSettings.h](../../Plugins/KataWorld/Source/KataWorld/Public/KataWorldSettings.h): 프로젝트 기본값.
- [KataWorldCheatExtension.h](../../Plugins/KataWorld/Source/KataWorld/Public/Cheats/KataWorldCheatExtension.h): 시간 치트.
- [플러그인 분리 결정 기록](../devlog/2026-10-10-Plugin-Modularization-Decisions.md): `KataWorld` 추가와 의존 방향.
- [작업 상태](https://github.com/jaykop/Kata/issues/127).
