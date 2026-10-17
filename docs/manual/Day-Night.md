# 낮밤 연출 사용법

갱신: 2026-10-17  
대상: KataWorld의 낮밤 연출(`UKataDayNightData`, `AKataDayNightDirector`)  
적용 기준: [#125](https://github.com/jaykop/Kata/issues/125), 상위 작업 [#127](https://github.com/jaykop/Kata/issues/127)  
확인 상태: 2026-10-17 사용자 에디터 빌드 통과. 같은 날 `LV_FeatureGallery`의 편집 레벨 미리보기로 0·6·12·18시 모습을 확인하며 기본 키와 샘플 값을 정했다. 기본 키를 마지막으로 고친 뒤(일출·일몰 5.5/18.5시, Sky Light·하늘 휘도 기본 키) 다시 빌드해 통과했고, PIE에서 `KataTime`·`KataDayLength`로 시간이 흐를 때 연출이 따라가는 것을 확인했다. Game 타깃 빌드와 World Partition 맵은 확인 전이다

## 목적과 준비

[시간대](Time-Of-Day.md)의 현재 시각에 맞춰 레벨의 태양·달, Sky Light, Sky Atmosphere, Exponential Height Fog를 바꾼다.
연출은 시간 상태를 읽기만 하고 바꾸지 않는다. 시각을 바꾸려면 시간 Subsystem 함수나 시간 치트를 쓴다.

- `KataWorld` 플러그인을 켠다. 샘플 프로젝트는 켜 두었다.
- 레벨에 하늘 액터가 있어야 한다. 엔진 기본 레벨 템플릿의 Directional Light, Sky Light, Sky Atmosphere, Exponential Height Fog를 그대로 쓸 수 있다.
- 게임에서 태양·달을 돌리려면 그 Directional Light의 Mobility가 Movable이어야 한다.
- Sky Light는 Real Time Capture를 켠다. 켜 두면 엔진이 매 프레임 하늘을 나눠서 다시 캡처하므로 하늘빛이 시각을 따라간다.
- 구름(Volumetric Cloud)은 태양 방향에 따라 엔진이 알아서 비춘다. 구름량 같은 날씨 연출은 [#126](https://github.com/jaykop/Kata/issues/126)에서 다룬다.
- 싱글플레이 전용이며 복제하지 않는다.

## 사용 순서

1. Content Browser에서 Miscellaneous > Data Asset을 고르고 클래스로 Kata Day Night Data를 골라 데이터 에셋을 만든다.
   새 에셋에는 태양·달의 Elevation·Azimuth·Intensity, Sky Light Intensity, Sky Luminance Factor에 기본 키가 들어 있어 따로 편집하지 않아도 하루가 돈다.
2. 레벨에 Kata Day Night Director를 하나 배치하고 Day Night Data에 1의 에셋을 지정한다.
3. Targets의 칸은 비워 두면 같은 레벨에서 찾는다([대상 찾기](#대상-찾기)). 같은 종류의 액터가 여럿이면 원하는 액터를 직접 지정한다.
4. Preview Hour를 움직여 편집 중인 레벨에서 시각별 모습을 확인한다. 커브를 고친 뒤에는 Preview Hour를 다시 움직여야 반영된다.
5. 밤이 낮처럼 밝게 보이면 Unbound PostProcessVolume을 두고 Exposure의 Min EV100으로 자동 노출 하한을 정한다([밤 노출](#밤-노출)).
6. PIE를 시작하고 [시간 치트](Time-Of-Day.md#시간-치트)로 시각을 바꿔 결과를 본다. 예: `KataTime 18`, `KataDayLength 0.5`.

달을 쓰려면 Directional Light를 하나 더 배치하고 다음처럼 설정한다.

- Atmosphere Sun Light를 켜고 Atmosphere Sun Light Index를 1로 둔다. 태양은 0이다.
- Mobility를 Movable로 둔다.
- Forward Shading Priority를 태양보다 낮게 둔다. 반투명·볼류메트릭 안개에 쓰일 조명을 태양으로 고정하기 위해서다.
- 달빛 색은 조명의 Light Color로 정해도 된다. 데이터의 Moon Color 커브를 비워 두면 조명의 색을 바꾸지 않는다.
- 구름이 밤에 하얗게 빛나 낮처럼 보이면 달 조명의 Cloud Scattered Luminance Scale을 낮춘다. 샘플은 0.3이다.

### 밤 노출

프로젝트에서 Extend Default Luminance Range를 켜면 자동 노출의 기본 범위는 -10~20 EV100이다. 이처럼 범위가 넓으면 어두운 장면을 크게 밝힐 수 있어 달빛만 있는 밤도 낮처럼 보일 수 있다.
Unbound PostProcessVolume의 Exposure > Min EV100으로 노출이 밝아질 수 있는 한계를 정한다. 값이 높을수록 밤이 어둡고, 낮 노출보다 높으면 낮도 어두워진다.
`LV_FeatureGallery`에는 PostProcessVolume(라벨 PostProcess_DayNightExposure, Min EV100 -1)을 두었다. 나머지 샘플 맵에는 아직 배치하지 않았다.

## 주요 설정과 실행 계약

### Kata Day Night Data

모든 커브의 가로축은 시각(0 이상 24 미만의 시)이다. 자정에서 끊기지 않도록 0시와 24시의 키를 같은 상태로 맞춘다.
**비어 있는 커브는 해당 속성을 바꾸지 않는다.** 바꾸고 싶은 항목만 채운다.

| 항목 | 의미·단위 | 적용 대상과 규칙 |
|---|---|---|
| Sun / Moon > Elevation | 광원이 하늘에 보이는 고도(도). 양수가 지평선 위 | 조명 Pitch = -Elevation |
| Sun / Moon > Azimuth | 광원이 하늘에 보이는 수평 방향(도) | 조명 Yaw = Azimuth + 180 + Yaw Offset. 하루 한 바퀴는 0시 0도에서 24시 360도처럼 이어서 적는다 |
| Sun / Moon > Intensity | 조명 세기(lux) | 0보다 작으면 0 |
| Sun / Moon > Color | 조명 색(선형) | Use Temperature가 켜져 있으면 색온도의 색과 곱해진다 |
| Sky Light > Sky Light Intensity | Sky Light 세기 배율 | 0보다 작으면 0 |
| Sky Light > Sky Light Color | Sky Light 색(선형) | 캡처한 하늘빛에 곱해진다 |
| Fog > Fog Density | 안개 전체 밀도 | 0보다 작으면 0 |
| Fog > Fog Inscattering Color | 안개가 띠는 색(선형) | 안개의 Fog Inscattering Color |
| Fog > Directional Inscattering Color | 태양 쪽을 볼 때 안개에 더해지는 색(선형) | 안개의 Directional Inscattering Color |
| Atmosphere > Rayleigh Scattering Scale | 낮 하늘의 파란 정도 | 0보다 작으면 0 |
| Atmosphere > Mie Scattering Scale | 태양 주변의 빛 번짐 | 0보다 작으면 0 |
| Atmosphere > Sky Luminance Factor | 하늘 밝기·색조 배율(선형) | Sky Atmosphere의 Sky Luminance Factor |

- Elevation과 Azimuth 중 하나만 비어 있으면 그 성분은 조명의 현재 회전을 유지한다.
- 색 커브는 External Curve가 지정돼 있거나 내부 R·G·B 중 하나라도 키 또는 기본값이 있으면 적용한다. 내부 Alpha에만 데이터가 있으면 적용하지 않는다.
  내부 R·G·B 채널에 키와 기본값이 모두 없으면 그 채널은 0으로 평가되므로 세 채널을 함께 채운다. Alpha에 키가 없으면 1로 평가된다.
- 기본 키(새 에셋):

| 항목 | 기본 키 |
|---|---|
| Sun > Elevation | 5.5시에 뜨고 12시에 60도, 18.5시에 진다. 0시가 가장 낮다(-60도) |
| Sun > Intensity | 4.5~19.5시 10 lux. 4시 이전과 20시 이후는 0 |
| Moon > Elevation | 18시에 뜨고 0시에 50도, 6시에 진다 |
| Moon > Intensity | 17시~다음 날 7시 0.3 lux. 7.5~16.5시는 0 |
| Sky Light Intensity | 1. 5.5~6.5시와 17.5~18.5시는 2 |
| Sky Luminance Factor | 5~19시 1. 20시~다음 날 4시 0.15 |

- 해가 정확히 지평선에 있으면 직사광이 대기를 길게 지나며 거의 사라진다. 그러면 하늘만 밝고 지면은 역광 실루엣이 된다. 그래서 일출·일몰을 5.5/18.5시로 두어 6시와 18시에 해가 지평선 위 약 5도에 있게 했다.
- 세기는 광원이 떠 있는 동안 일정하게 둔다. 지평선 근처의 감쇠와 노을의 색은 Sky Atmosphere가 태양 고도에 따라 만든다. 세기 커브로 미리 줄이면 해 질 녘이 한밤보다 어두워진다.
  0으로 내리는 구간은 광원이 지평선 아래 10~19도쯤에 있을 때다. 박명이 끝난 뒤에는 보이지 않는 조명의 그림자 계산을 줄이기 위해서다.
- 노을 무렵에는 하늘에 비해 그늘이 어두워지므로 Sky Light를 2배로 올린다.
- 달빛도 하늘을 낮과 같은 파란색으로 산란시킨다. 그러면 밤하늘이 지면보다 밝고 채도도 높아 낮처럼 보인다. 그래서 박명이 끝난 뒤에는 하늘 휘도를 0.15로 낮춘다.
- 조명 색 커브와 안개, 나머지 대기 커브는 기본으로 비어 있다.
- 커브 칸의 External Curve에 Curve Float·Curve Linear Color 에셋을 지정하면 그 에셋을 대신 쓴다.

### Kata Day Night Director

| 항목 | 의미·입력 | 기본값·빈 값 |
|---|---|---|
| Day Night Data | 적용할 커브 에셋 | 비어 있으면 아무것도 바꾸지 않고 PIE 시작 때 경고한다 |
| Yaw Offset | 이 레벨에서 태양·달 방위에 더하는 각도(도) | 0 |
| Targets > Sun Light, Moon Light, Sky Light, Sky Atmosphere, Height Fog | 바꿀 레벨 액터 | 비어 있으면 같은 레벨에서 찾는다 |
| Preview > Preview In Editor | 편집 중인 레벨에 Preview Hour를 적용할지 | 켜짐. 끄면 마지막으로 적용한 값이 그대로 남는다 |
| Preview > Preview Hour | 편집 중인 레벨에서 미리 볼 시각(시) | 9. 게임·PIE에는 영향이 없다 |

실행 중에는 다음 규칙으로 적용한다.

- BeginPlay에서 대상을 정하고 현재 시각을 한 번 적용한다. 대상은 이때 한 번만 정하므로 실행 중에 배치한 하늘 액터는 쓰지 않는다.
- 매 프레임 시간 Subsystem의 Get Time Of Day를 읽어, 마지막으로 적용한 시각과 다를 때만 다시 적용한다. Fixed 모드에서도 시각 지정 함수나 치트로 값이 바뀌면 다시 적용한다. 게임이 일시정지 중이면 적용하지 않는다.
- 안개와 대기는 값이 조금만 바뀌어도 렌더 상태를 다시 만든다. 실수 값은 새 값의 절댓값을 기준으로 0.5%(최소 허용 오차 0.0001) 이하의 차이를 건너뛴다. 색은 모든 채널의 차이가 0.005 이하면 건너뛴다. 조명과 Sky Light는 시각을 적용할 때마다 갱신한다.
- 시간 상태의 델리게이트를 구독하지 않으므로 종료할 때 정리할 구독이 없다.

### 대상 찾기

참조 칸이 비어 있는 항목만 연출 액터와 같은 레벨에서 찾는다. 찾은 결과는 참조 칸에 써넣지 않는다.

| 항목 | 찾는 액터 |
|---|---|
| Sun Light | Atmosphere Sun Light가 켜져 있고 Index가 0인 첫 Directional Light |
| Moon Light | Atmosphere Sun Light가 켜져 있고 Index가 1인 첫 Directional Light. 없으면 달 커브를 쓰지 않는다 |
| Sky Light, Sky Atmosphere, Height Fog | 클래스별로 처음 찾은 액터 |

- Atmosphere Sun Light가 꺼진 Directional Light(보조광 등)는 태양·달로 고르지 않는다.
- Sun Light와 Moon Light에 같은 액터를 지정하면 달을 쓰지 않고 PIE 시작 때 경고한다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 태양이 움직이지 않는다 | 레벨에 연출 액터가 없거나 Day Night Data가 비어 있다. 시간이 Fixed다 | 연출 액터와 데이터를 확인하고 `KataTime`으로 모드를 본다. `LogKataWorld` 경고를 확인한다 |
| PIE에서만 태양이 안 돈다 | Directional Light가 Movable이 아니다. 편집 레벨에서는 회전하지만 게임에서는 엔진이 막는다 | Mobility를 Movable로 바꾼다. `... is not Movable and will not rotate at runtime` 경고가 남는다 |
| 하늘빛과 그늘진 곳의 밝기가 시각을 따라오지 않는다 | Sky Light의 Real Time Capture가 꺼져 있다. 연출 액터는 다시 캡처를 요청하지 않는다 | Real Time Capture를 켠다. `... does not use real-time capture` 경고가 남는다 |
| 밤이 낮처럼 밝게 보인다 | 달빛도 하늘을 낮과 같은 색으로 산란시킨다. 자동 노출 범위가 넓으면 어두운 장면을 크게 밝힐 수 있다 | [밤 노출](#밤-노출)대로 Min EV100을 제한한다. 하늘과 구름만 밝으면 Sky Luminance Factor의 밤 값과 달 조명의 Cloud Scattered Luminance Scale을 낮춘다 |
| 밤이 너무 어둡다 | 달이 없거나 약하다. Min EV100이 너무 높다 | 달 조명을 두고 Moon Intensity를 올리거나 Min EV100을 낮춘다 |
| 맵을 저장했더니 조명 값이 바뀌어 있다 | 미리보기 값이 대상 액터 속성에 남은 채 저장된다 | 의도한 동작이다. 실행하면 현재 시각의 값으로 덮어쓴다. 연출 액터를 지우면 저장된 마지막 미리보기 값이 남는다 |
| 커브를 고쳐도 편집 레벨이 그대로다 | 미리보기는 연출 액터의 값이 바뀔 때만 다시 적용한다 | Preview Hour를 다시 움직인다 |
| 다른 레벨의 하늘 액터를 바꾸지 않는다 | 자동 찾기는 연출 액터와 같은 레벨만 본다 | 하늘 액터와 같은 레벨에 배치하거나 참조 칸에 지정한다 |

- World Partition 맵에서의 동작은 확인하지 않았다. 샘플 맵 3개는 World Partition을 쓰지 않는다.
- 태양을 매 프레임 회전하면 그림자 캐시가 매 프레임 다시 그려질 수 있다. 성능은 측정 전이다.

## 확인 상태와 근거

사용자 확인 범위는 문서 머리의 확인 상태를 따른다. 나머지는 소스 기준이다.

- [KataDayNightData.h](../../Plugins/KataWorld/Source/KataWorld/Public/DayNight/KataDayNightData.h): 커브 항목과 의미.
- [KataDayNightData.cpp](../../Plugins/KataWorld/Source/KataWorld/Private/DayNight/KataDayNightData.cpp): 기본 키.
- [KataDayNightDirector.h](../../Plugins/KataWorld/Source/KataWorld/Public/DayNight/KataDayNightDirector.h): 대상 찾기, 미리보기, 실행 계약.
- [KataDayNightDirector.cpp](../../Plugins/KataWorld/Source/KataWorld/Private/DayNight/KataDayNightDirector.cpp): 빈 커브 판정, 회전 계산, 안개·대기의 변경 임계값, 경고.
- [시간대 사용법](Time-Of-Day.md): 연출이 읽는 시각과 시간 치트.
- [작업 상태](https://github.com/jaykop/Kata/issues/125).
