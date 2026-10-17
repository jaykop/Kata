# 낮밤·날씨의 시간 상태와 KataWorld 선행 생성

작성: 2026-10-17  
갱신: 2026-10-17  
유형: 결정 기록  
대상: 새 위성 플러그인 `KataWorld`, 시간 상태(`UKataWorldTimeSubsystem`, `AKataLevelTimeSettings`, `UKataWorldSettings`, 시간 치트)  
기준: 커밋 `5310bb7` 이후 작업 트리의 #124 변경. 같은 트리에 있는 다른 작업의 미커밋 변경(스포너, Hit 문서 등)은 대상에서 제외했다

## 배경과 결론

사용자가 낮밤과 날씨 시스템을 요청했다. 첫 단계는 화면 연출만 하되, 나중에 게임플레이(AI 시야, 조건)에 연결해도 구조를 다시 짜지 않아야 했다.
상위 작업 [#127](https://github.com/jaykop/Kata/issues/127)을 시간 상태 [#124](https://github.com/jaykop/Kata/issues/124), 낮밤 연출 [#125](https://github.com/jaykop/Kata/issues/125), 날씨 [#126](https://github.com/jaykop/Kata/issues/126)으로 나눴다.

#124에서 위성 플러그인 `KataWorld`를 만들고 시간 상태 계층을 구현했다. 시각과 시간대는 월드 Subsystem이 들고, 연출은 그 상태를 읽기만 한다. 레벨 이벤트 [#63](https://github.com/jaykop/Kata/issues/63)이 쓰기로 한 플러그인을 이 작업이 먼저 만들었다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| 플러그인 구성 | 위성은 KataTargeting·KataAI·KataCamera 셋이었다. `KataWorld`는 #63 계획에만 있었다 | `KataWorld` 플러그인과 Runtime 모듈을 만들고 `.uproject`, `AGENTS.md`, 규칙 검사 스크립트·manual에 넣었다 |
| 시간 상태 | 없음 | `UKataWorldTimeSubsystem`이 시각(0~24), 모드(Flowing·Fixed), 하루 길이, 시간대 태그, 날짜 수를 가진다. 시간대가 바뀌면 네이티브·동적 델리게이트로 알린다 |
| 설정 | 없음 | 프로젝트 설정 Kata World(기본 시각, 하루 길이, 시간대 구간)와 레벨에 배치하는 `AKataLevelTimeSettings` |
| 치트 | 없음 | `KataTime`, `KataTimeFlow`, `KataDayLength` |
| 샘플 | 없음 | `TimeOfDay` 태그(Dawn·Day·Dusk·Night)와 시간대 구간(5·7·17·19시), 하루 24분 |

## 주요 결정과 이유

- **상태와 연출을 나눈다.** 연출은 매 프레임 시각을 읽고(pull) 시간대 변경은 델리게이트로 받는다(push). 게임플레이를 나중에 연결할 때는 읽는 쪽만 더하면 된다.
  다른 위성은 `KataWorld`를 참조할 수 없으므로(위성 간 의존은 KataAI → KataTargeting만 허용) 게임플레이 연결은 `KataFramework`가 잇거나 태그로 전달한다. 2026-10-17 사용자 확인.
- **연출은 엔진 DaySequence를 쓰지 않고 직접 만든다.** UE 5.8의 Experimental 플러그인 DaySequence는 시간 흐름, 고정 시각 모드, 볼륨·전역 Modifier 섞기를 이미 제공한다.
  Experimental API가 바뀔 위험을 피하려고 직접 구현을 골랐다. 상태 계층이 분리돼 있어 나중에 연출 방식만 DaySequence로 바꿀 수 있다. 2026-10-17 사용자 결정.
- **시간은 실시간으로 흐르고, 레벨 단위로 고정할 수 있다.** 지역 볼륨 단위의 고정은 넣지 않았다. #63의 지역 볼륨과 겹치기 때문이다. 2026-10-17 사용자 결정.
- **레벨 단위 설정은 배치 액터로 한다.** `AWorldSettings` 하위 클래스는 프로젝트에 하나만 지정할 수 있어 다른 플러그인과 부딪힌다. 배치하지 않은 레벨은 프로젝트 기본값으로 흐른다. 2026-10-17 사용자 확인.
- **시계는 시간 배율을 무시하고 일시정지는 따른다.** 히트스톱이나 슬로모션으로 월드가 잠깐 느려질 때 해가 같이 느려질 이유가 없다.
  월드 Tick의 시간을 월드 유효 배율로 나누는 방식은 [시간 배율 요청](2026-10-12-Time-Dilation-Requests.md)의 만료 시계와 같다. 2026-10-17 사용자 확인.
- **날씨는 Blueprint 호출과 치트로만 바꾼다.** 무작위 일정은 넣지 않는다(#126). 2026-10-17 사용자 결정.
- **태그 루트는 `TimeOfDay`와 `Weather`다.** #63의 `LevelEvent`·`Region`·`WorldFlag`와 겹치지 않는다. `Weather`는 #126에서 쓴다. 2026-10-17 사용자 확인.
- 시간대 구간은 프로젝트 설정에 두고, Subsystem이 시작할 때 Start Hour 순으로 정렬한 사본을 쓴다. 공유 설정 객체에 실행 상태를 두지 않기 위해서다.
- `KataWorld`는 KataTargeting의 치트 인자 파서를 쓸 수 없어서(위성 간 의존 금지) 같은 규칙의 토글 파서를 따로 두었다.

## 근거

- [KataWorldTimeSubsystem.h](../../Plugins/KataWorld/Source/KataWorld/Public/Time/KataWorldTimeSubsystem.h): 시계와 시간대 알림 계약.
- [KataWorldTimeSubsystem.cpp](../../Plugins/KataWorld/Source/KataWorld/Private/Time/KataWorldTimeSubsystem.cpp): `Tick`의 배율 보정, `CachePhases`·`FindPhase`의 구간 규칙.
- [KataLevelTimeSettings.h](../../Plugins/KataWorld/Source/KataWorld/Public/Time/KataLevelTimeSettings.h): 레벨 단위 설정의 등록 규칙.
- [플러그인 분리 결정 기록](2026-10-10-Plugin-Modularization-Decisions.md): 위성 의존 방향과 `KataWorld` 추가 단락.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 에디터 빌드 | 사용자 빌드 보고(2026-10-17) | 통과. 설명 검토에서 나온 `GetTimeOfDay`의 24.0 반환 경계를 보정한 뒤 다시 빌드해 통과 | Game 타깃 빌드 |
| 레벨 설정 적용 | 사용자 PIE, `LV_TestMap`에 Fixed·21시 설정 액터(MCP로 배치, 저장 안 함), `KataTime` | `21.00 h, phase TimeOfDay.Night, Fixed, day length 24.0 min, day 0` 로그 확인 | — |
| 시간 흐름, 날짜 넘김, 시간대 변경 알림, 나머지 치트, 배율·일시정지 동작 | — | 미확인 | 전부 |
| 규칙 검사 스크립트 | — | 로컬 실행하지 않았다 | push 뒤 GitHub Actions 결과 |

## 남은 제한과 후속 작업

- 화면 연출이 없다. 태양·하늘·안개는 #125, 날씨는 #126에서 한다. 비·눈 파티클 때문에 `KataWorld`에 Niagara 의존을 더할지는 #126에서 정한다.
- #63 LE-1에는 이벤트 채널과 디버거 카테고리가 남는다. 그때 `KataWorld`에 코어 `Kata` 등 실제로 쓰는 의존을 더한다.
- 세이브에 시각을 저장하지 않는다. 세이브 계층([#88](https://github.com/jaykop/Kata/issues/88))에서 다룬다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#124](https://github.com/jaykop/Kata/issues/124) | 구현 완료, 실행 확인은 일부. 상태 댓글과 라벨은 사용자 확인 후 게시 |
| [시간대 사용법](../manual/Time-Of-Day.md) | 새로 작성 |
| [게임플레이 태그 사용법](../manual/Gameplay-Tags.md) | 루트 `TimeOfDay` 추가 |
| [치트 사용법](../manual/Cheats.md) | 시간 치트 세 명령 추가 |
| [AGENTS.md 규칙 검사](../manual/Agents-Rules-Check.md) | 플러그인 일곱 개, 위성 네 개로 갱신 |
| [레벨 이벤트 계획](../plan/Level-Event-Plan.md) | 플러그인 생성이 #124로 옮겨진 점과 의존 추가 시점 |
| [플러그인 분리 결정 기록](2026-10-10-Plugin-Modularization-Decisions.md) | `KataWorld` 추가 후속 단락 |
