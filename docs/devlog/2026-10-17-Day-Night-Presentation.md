# 낮밤 연출 액터와 시각별 커브 데이터

작성: 2026-10-17  
갱신: 2026-10-17  
유형: 결정 기록  
대상: KataWorld 낮밤 연출(`UKataDayNightData`, `AKataDayNightDirector`), KataWorld 시각·하루 길이 칸의 단위 메타, 샘플 `LV_FeatureGallery` 배치  
기준: 커밋 `1f917e1` 이후 작업 트리의 #125 변경. 같은 트리에 있는 다른 작업의 미커밋 변경(스포너, 치트, Hit 문서 등)은 대상에서 제외했다

## 배경과 결론

[#127](https://github.com/jaykop/Kata/issues/127)의 두 번째 단계로 [#125](https://github.com/jaykop/Kata/issues/125) 낮밤 연출을 구현했다. [시간 상태](2026-10-17-Time-Of-Day-State.md)가 들고 있는 현재 시각을 읽어 레벨의 태양·달, Sky Light, Sky Atmosphere, Exponential Height Fog를 바꾼다.

시각별 커브는 데이터 에셋 `UKataDayNightData`에 두고, 레벨에 배치하는 `AKataDayNightDirector`가 그 커브를 평가해 적용한다. 편집 중인 레벨에서는 미리보기 시각으로 같은 결과를 본다.
샘플 `LV_FeatureGallery`에서 시각별 모습을 사용자와 함께 확인하며 기본 키를 고쳤고, 밤을 밤답게 보이려면 자동 노출 범위를 제한해야 한다는 것을 확인했다.
확인 중에 시각 입력 칸의 단위 메타 결함을 찾아 #124에서 만든 칸까지 함께 고쳤다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 데이터 에셋 | 없음 | `UKataDayNightData`(UPrimaryDataAsset): 태양·달 커브 묶음(고도·방위·세기·색), Sky Light 세기·색, 안개 밀도·내산란색·방향 내산란색, 대기 Rayleigh·Mie 산란 배율과 하늘 휘도 배율 |
| 연출 액터 | 없음 | `AKataDayNightDirector`(AInfo): 대상 참조와 빈 칸 자동 찾기, 방위 보정, 시각이 바뀔 때만 적용, 편집 레벨 미리보기, 설정 경고 |
| 기본 키 | — | 새 에셋이 바로 하루를 돌도록 태양·달의 고도·방위·세기, Sky Light 세기, 하늘 휘도에 기본 키를 넣었다. 값과 이유는 [낮밤 연출 사용법](../manual/Day-Night.md#kata-day-night-data) |
| 밤 노출 | 엔진 기본 자동 노출(-10~20 EV100)이 달빛 밤을 낮 밝기로 끌어올렸다. 달을 끄면 완전히 까맣다 | 코드는 바꾸지 않았다. 샘플에 Unbound PostProcessVolume을 두고 Min EV100으로 노출 하한을 정한다. `LV_FeatureGallery`는 -1(사용자 조정) |
| 시각·하루 길이 칸 단위 | `Units = "Hours"`·`"Minutes"`가 표시 단위를 바꾸고 단위 없는 입력도 그 단위로 해석했다. Preview Hour에 12를 넣으면 `720.0 min`으로 보이다 12분(0.2시)이 들어갔다 | KataWorld의 시·분·도 메타 7곳을 `ForceUnits`로 바꿨다. #124의 프로젝트 설정·레벨 시간 설정·시간대 구간 칸 포함 |
| 샘플 | 하늘 액터 기본 세트만 있었다 | `DA_KataDayNight_Default` 생성. `LV_FeatureGallery`에 달 Directional Light, 연출 액터, PostProcessVolume 배치. `LV_CombatSandbox`·`LV_TestMap`은 배치하지 않았다(사용자 결정) |

결함: 레벨 시간 설정·프로젝트 설정의 시각과 하루 길이 칸에 숫자만 입력하면 에디터가 고른 표시 단위로 해석된다 — 원인 위치: `Plugins/KataWorld/Source/KataWorld/Public/Time/KataLevelTimeSettings.h` — 이슈: #124

## 주요 결정과 이유

- **연출은 시각을 읽기만 한다(pull).** 매 Tick 시간 Subsystem의 시각을 읽어, 마지막으로 적용한 값과 다를 때만 적용한다. Fixed 모드나 일시정지에서는 할 일이 없다. 델리게이트를 구독하지 않아 정리할 구독이 없다.
- **대상은 참조 칸과 빈 칸 자동 찾기로 정한다.** 비어 있으면 같은 레벨에서 찾고, 태양은 대기 광원 0번, 달은 1번 Directional Light다. 대기 광원이 아닌 보조광은 고르지 않는다. 찾은 결과는 참조 칸에 써넣지 않는다. 2026-10-17 사용자 결정.
- **달은 별도의 Directional Light(대기 광원 1번)로 둔다.** 태양과 같은 커브 묶음을 쓴다. 2026-10-17 사용자 결정.
- **Sky Light는 실시간 캡처를 전제로 한다.** 엔진 주석상 `RecaptureSky`는 히치를 일으키고, 실시간 캡처는 매 프레임 나눠서 다시 캡처한다. 꺼져 있으면 경고만 남긴다. 2026-10-17 사용자 결정.
- **미리보기는 연출 액터의 Preview Hour로 한다.** 에디터 모듈을 더하지 않으려고 미리 본 값이 대상 액터 속성에 남는 것을 받아들였다. 실행하면 현재 시각의 값으로 덮어쓴다. 2026-10-17 사용자 결정.
- 빈 커브는 그 속성을 바꾸지 않는다. Kata의 다른 커브 칸과 같은 관례다.
- 안개·대기 세터는 값이 조금만 달라도 렌더 상태를 다시 만든다. 그래서 눈에 띄지 않는 차이(상대 0.5%, 색 채널 0.005)는 건너뛴다. 조명·Sky Light 세터는 가벼운 경로라 매번 부른다.
- 게임 월드에서 Movable이 아닌 조명을 옮기면 엔진이 매번 경고하고 무시한다. 그래서 회전을 건너뛰고 BeginPlay에서 한 번 경고한다.
- **밤 노출은 코드가 아니라 레벨의 PostProcessVolume으로 정한다.** 달빛도 하늘을 낮과 같은 색으로 산란시키므로, 노출 범위가 넓으면 어떤 세기로도 밤이 낮처럼 보인다. 연출에 노출 보정 커브를 더하는 안도 있었지만, 코드 변경 없이 맵에서 정하는 쪽을 골랐다. 2026-10-17 사용자 결정.
- **기본 키는 미리보기로 확인하며 고쳤다.** 아래 판단은 모두 2026-10-17 사용자 확인을 거쳤다.
  - 처음 기본 키는 세기를 지평선 전에 줄였다. 그러자 대기 감쇠와 겹쳐 18시가 0시보다 어두웠다. 그래서 세기는 떠 있는 동안 일정하게 두고, 감쇠와 노을색은 Sky Atmosphere에 맡겼다.
  - 해가 정확히 지평선에 있으면 직사광이 거의 사라져 지면이 역광 실루엣이 된다. 그래서 일출·일몰을 5.5/18.5시로 옮겨 6시와 18시에 해가 약 5도 떠 있게 했다.
  - 노을 무렵 Sky Light를 2배로 올려 그늘을 채운다.
  - 밤하늘은 하늘 휘도를 0.15로 낮춰 지면보다 어둡게 했다.
  - 달빛을 받은 흰 구름이 낮처럼 보이는 문제는 조명 속성(Cloud Scattered Luminance Scale)으로 조정했다. 그래서 데이터 기본값이 아니라 샘플의 달 조명에 0.3을 넣었다.
- **입력 칸은 `ForceUnits`를 쓴다.** 엔진 숫자 편집기(`SPropertyEditorNumeric.h`)는 `Units`가 있으면 가장 알맞은 단위로 바꿔 표시하고 입력도 그 단위로 읽는다. `ForceUnits`는 그 단위로 고정해 표시하고 읽는다.

## 근거

- [KataDayNightData.h](../../Plugins/KataWorld/Source/KataWorld/Public/DayNight/KataDayNightData.h), [KataDayNightData.cpp](../../Plugins/KataWorld/Source/KataWorld/Private/DayNight/KataDayNightData.cpp): 커브 항목과 기본 키.
- [KataDayNightDirector.h](../../Plugins/KataWorld/Source/KataWorld/Public/DayNight/KataDayNightDirector.h), [KataDayNightDirector.cpp](../../Plugins/KataWorld/Source/KataWorld/Private/DayNight/KataDayNightDirector.cpp): 대상 찾기, 적용 조건, 회전 계산, 안개·대기 임계값, 경고.
- [KataLevelTimeSettings.h](../../Plugins/KataWorld/Source/KataWorld/Public/Time/KataLevelTimeSettings.h), [KataWorldSettings.h](../../Plugins/KataWorld/Source/KataWorld/Public/KataWorldSettings.h), [KataWorldTimeTypes.h](../../Plugins/KataWorld/Source/KataWorld/Public/Time/KataWorldTimeTypes.h): `ForceUnits`로 바꾼 칸.
- 엔진 `Editor/PropertyEditor/Private/UserInterface/PropertyEditor/SPropertyEditorNumeric.h`: `ForceUnits`와 `Units`의 표시·입력 해석.
- 엔진 `SkyLightComponent.h`의 `RecaptureSky` 주석과 `ReflectionEnvironmentRealTimeCapture.cpp`의 나눠 캡처하는 처리.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 에디터 빌드 | 사용자 빌드 보고(2026-10-17) | 구현 뒤, `ForceUnits` 변경 뒤, 기본 키 변경 뒤 모두 통과 | Game 타깃 빌드 |
| 편집 레벨 미리보기 | 사용자 확인(2026-10-17), `LV_FeatureGallery` | 0·6·12·18시 모습과 미리보기 시각 입력(시 단위) 확인 | 다른 샘플 맵 |
| 시간 흐름 적용 | 사용자 PIE(2026-10-17), `KataTime`·`KataDayLength`·`KataTimeFlow` | 시간이 흐를 때 연출이 따라간다 | 시간대 변경 알림, 날짜 넘김, 시간 배율·일시정지 |
| 성능 | — | 측정하지 않았다 | 태양 회전에 따른 그림자 캐시 갱신 비용 |
| 규칙 검사 스크립트 | — | 로컬 실행하지 않았다 | push 뒤 GitHub Actions 결과 |

## 남은 제한과 후속 작업

- 구름량과 날씨에 따른 연출은 [#126](https://github.com/jaykop/Kata/issues/126)에서 한다.
- 시간대별 노출은 맵의 PostProcessVolume 하나로만 정한다. 시간대마다 노출을 설계하려면 별도 결정이 필요하다.
- `LV_CombatSandbox`·`LV_TestMap`에는 낮밤 연출을 배치하지 않았다.
- 샘플 바닥처럼 조명을 받지 않는 재질은 밤에도 밝아 밤 느낌을 깬다. 샘플 재질의 문제라 이번 작업에서 바꾸지 않았다.
- World Partition 맵에서의 대상 찾기와 미리보기는 확인하지 않았다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#125](https://github.com/jaykop/Kata/issues/125) | 구현과 사용자 빌드·미리보기·PIE 확인 완료. 결과 댓글은 사용자 확인 후 게시 |
| [#124](https://github.com/jaykop/Kata/issues/124) | 시간 흐름과 `KataDayLength`·`KataTimeFlow` 실행 확인, 단위 메타 결함 수정. 댓글은 사용자 확인 후 게시 |
| [낮밤 연출 사용법](../manual/Day-Night.md) | 새로 작성 |
| [시간대 사용법](../manual/Time-Of-Day.md) | 연출 링크, 문제 해결 행, 확인 상태 갱신 |
| [AGENTS.md](../../AGENTS.md) | KataWorld 설명에 낮밤 연출 추가 |
| [시간 상태 결정 기록](2026-10-17-Time-Of-Day-State.md) | 당시 기록을 보존한다. 후속 결정은 이 문서에서 다룬다 |
