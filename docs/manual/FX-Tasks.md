# 연출 태스크 사용법

갱신: 2026-10-14  
대상: 액션 타임라인의 Play Sound·Camera Shake(코어 `KataRuntime`), Spawn VFX(`KataFramework`)  
적용 기준: [연출 태스크 결정 기록](../devlog/2026-10-14-FX-Tasks.md), [#7](https://github.com/jaykop/Kata/issues/7)  
확인 상태: 2026-10-14 사용자 Editor 빌드와 프리뷰·PIE 확인. 범위는 아래 "확인 상태와 근거"를 따른다

## 목적과 준비

휘두르는 소리, 칼날 궤적, 내려찍기 흔들림처럼 **액션 타임라인의 시각에 맞춰** 내는 연출을 다룬다.
맞는 순간의 이펙트·소리·흔들림·진동은 이 태스크가 아니라 히트 처리기의 Gameplay Cue가 맡는다([피격 반응 사용법](Hit-Reaction.md)).

- Play Sound·Camera Shake는 코어만 있으면 쓸 수 있다. Play Sound는 엔진 모듈 AudioExtensions를 쓰며 MetaSound 플러그인에 의존하지 않는다.
- Spawn VFX는 `KataFramework`에 있으며 엔진 Niagara 플러그인이 켜져 있어야 한다. `KataFramework.uplugin`이 Niagara를 의존으로 선언한다.
- 세 태스크 모두 Add Task 메뉴의 **FX** 묶음에 있고 실행 단계(Phase)는 Presentation이다.

## 사용 순서

1. 액션 편집기에서 Add Task → FX → Play Sound / Camera Shake / Spawn VFX를 추가한다.
2. Timeline Details에서 에셋(Sound, Shake Class, System)을 지정하고 Start Time을 연출 시점에 맞춘다.
3. 한 번 내고 스스로 끝나는 연출은 Duration 0과 Completed Policy Keep Playing으로 둔다. 구간이 끝날 때 거둘 연출은 Duration을 구간 길이로 두고 Completed Policy를 Stop 계열로 고른다.
4. 프리뷰에서 ▶로 재생해 시점을 맞추고, PIE에서 실제 캐릭터로 확인한다.

## 주요 설정과 실행 계약

### 공통

| 상황 | 동작 |
|---|---|
| 정상 완료(Completed) | Completed Policy를 따른다. 기본값은 끝까지 재생이다 |
| 중단·취소·Kata 종료 | 정책과 관계없이 정리한다. Sound는 Fade Out Time 동안 줄이고, Shake는 블렌드 아웃하고, VFX는 방출을 멈춘다 |
| 루프 에셋 + Keep Playing | 스스로 끝나지 않으므로 정리 쪽으로 처리하고 데이터 검증에서 경고한다 |
| 재생·생성 실패 | 태스크를 일찍 완료하지 않는다. 의존으로 이어진 태스크의 시작 시각이 연출 성공 여부에 따라 바뀌지 않게 하기 위해서다 |

### Play Sound

| 항목 | 의미 | 기본값·실패 시 |
|---|---|---|
| Sound | Sound Wave, Sound Cue, MetaSound Source | 비우면 설정 오류 |
| Attach To Mesh | 켜면 Avatar의 스켈레탈 메시 소켓에 붙어 따라간다. 끄면 시작 순간의 소켓 위치에 남는다 | 켬. 메시가 없으면 Avatar 루트 컴포넌트를 기준으로 한다 |
| Attach Socket | 기준 소켓 | 비우면 메시 원점 |
| Volume Multiplier, Pitch Multiplier | 볼륨·피치 배율 | 1. Pitch가 0 이하이면 설정 오류 |
| Attenuation Override | 감쇠 설정 | 비우면 사운드 에셋 설정 |
| Audio Parameters | 재생 전에 넘길 MetaSound 입력. 이름과 타입이 MetaSound 입력과 같아야 한다 | 이름이 맞지 않는 값은 엔진이 무시하며 태스크는 보고하지 않는다 |
| Completed Policy | Keep Playing / Stop | Keep Playing |
| Fade Out Time | 멈출 때 줄이는 시간(초). 중단될 때도 쓴다 | 0.2. 0이면 즉시 멈춘다 |

### Camera Shake

| 항목 | 의미 | 기본값·실패 시 |
|---|---|---|
| Shake Class | 흔들림 클래스 | 비우면 설정 오류 |
| Scale | 세기 배율. 반경 모드에서는 거리 감쇠를 곱한다 | 1 |
| Target | Avatar Camera: Avatar를 조종하는 로컬 플레이어 카메라만 흔든다. AI가 실행하면 아무도 흔들리지 않는다. Local Players In Radius: Avatar 위치를 진원으로 반경 안의 로컬 플레이어 카메라를 흔든다 | Avatar Camera |
| Inner Radius, Outer Radius, Falloff | 반경 모드 전용. 진원에서 카메라 위치까지의 거리가 Inner 이하면 Scale 그대로, Outer 이상이면 흔들지 않고, 그 사이는 Falloff 지수 곡선으로 줄인다 | 300cm, 1500cm, 1(선형). Outer가 Inner보다 작으면 설정 오류 |
| Completed Policy | Keep Playing / Stop(블렌드 아웃) | Keep Playing |

보스 AI의 공격이 플레이어 카메라를 흔들게 하려면 Local Players In Radius를 쓴다.

### Spawn VFX

| 항목 | 의미 | 기본값·실패 시 |
|---|---|---|
| System | Niagara 시스템 | 비우면 설정 오류 |
| Attach To Mesh | 켜면 기준 메시 소켓에 붙어 따라간다. 끄면 시작 순간의 소켓 위치에 생성하고 따라가지 않는다 | 켬 |
| Mesh Source, Weapon Slot | Hit Trace와 같은 규칙으로 기준 메시를 고른다. Avatar의 Kata Hit Box 컴포넌트가 Character·Weapon 메시를 제공하고, 컴포넌트가 없으면 Character만 캐릭터 메시로 대신한다 | Character. 메시를 찾지 못하면 경고 후 생성하지 않는다 |
| Attach Socket | 기준 소켓 | 비우면 메시 원점 |
| Location Offset, Rotation Offset, Scale | 소켓 기준 오프셋과 크기 | 0, 0, 1 |
| Completed Policy | Keep Playing / Stop Emitting(남은 파티클은 수명대로) / Destroy Immediately | Keep Playing |

Niagara 컴포넌트 풀링은 쓰지 않는다. 풀로 돌아간 컴포넌트가 다른 생성에 재사용되면 태스크가 남의 이펙트를 정리할 수 있기 때문이다.

## 프리뷰와 게임의 차이

| 태스크 | 프리뷰 ▶ 재생 | 재생 헤드 Seek | PIE·게임 |
|---|---|---|---|
| Play Sound | 감쇠·방향 없이 UI 사운드로 들린다 | 소리 내지 않는다 | Attenuation과 부착 위치대로 들린다 |
| Camera Shake | 흔들리지 않는다(프리뷰에 PlayerController가 없음) | 같음 | 흔들린다 |
| Spawn VFX | 표시한다 | 표시한다. 뒤로 Seek하면 처음부터 다시 시뮬레이션한다 | 표시한다 |

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 할 일 |
|---|---|---|
| 프리뷰에서 재생 헤드를 끌 때 소리가 안 난다 | 의도된 동작. 프리뷰가 Seek하는 동안 월드 오디오 재생을 끈다 | ▶ 재생으로 확인한다 |
| MetaSound 입력이 반영되지 않는다 | Audio Parameters의 이름·타입이 MetaSound 입력과 다르다 | Param Name 철자와 Param Type을 맞춘다 |
| 이펙트가 소켓을 따라오지 않고 생성 위치에 남는다 | Niagara 이미터가 월드 공간(Local Space 꺼짐)이라 파티클이 생성 위치에 남는다. 태스크 부착과는 별개다 | 따라가야 하는 이펙트는 이미터의 Local Space를 켠다. 궤적처럼 지나간 자리에 남기려면 월드 공간에서 계속 방출한다 |
| 이펙트가 엉뚱한 위치(메시 원점)에 생긴다 | Attach Socket이 그 메시에 없다 | 메시의 소켓 이름을 확인한다. 소켓 이름과 실제 칼끝 위치가 다를 수 있다 |
| Output Log에 `Kata VFX task ... found no weapon mesh` | Weapon 슬롯에 장착한 메시가 없거나 Kata Hit Box 컴포넌트가 없다 | 장비를 장착하거나 Mesh Source를 Character로 둔다 |
| 보스 공격에서 카메라가 흔들리지 않는다 | Target이 Avatar Camera면 AI 실행에서는 흔들 카메라가 없다 | Local Players In Radius를 쓴다 |
| 미지원 | Niagara User Parameter 전달, 타임라인에서 Gameplay Cue 실행, 프리뷰 카메라 흔들림 | 필요해지면 후속 작업으로 다룬다 |

## 확인 상태와 근거

2026-10-14 사용자가 Editor 빌드 뒤 샘플 `KA_BlackKnight_Attack01`로 확인했다. 확인용 에셋은 `MS_Test_Whoosh`(노이즈 엔벨로프 MetaSound, 입력 `Intensity`), `CS_Hit_Heavy`, `NS_Test_Spark`(DirectionalBurst 템플릿, 로컬 공간)다.

- Play Sound: 프리뷰 ▶ 재생 시 소리, Seek 중 무음, PIE 소리, Audio Parameters `Intensity` 값에 따른 크기 변화.
- Camera Shake: 플레이어 공격 흔들림, 보스 공격의 거리 감쇠, Avatar Camera에서 보스 공격 무반응, 프리뷰 무동작.
- Spawn VFX: Weapon 메시 소켓 부착, 프리뷰 표시, 플레이어·보스 공격 표시, 회피 캔슬 뒤 재생성 없음.

Completed Policy의 Stop 계열(Sound Stop, Shake Stop, VFX Stop Emitting·Destroy Immediately)과 Attach To Mesh 끔, 루프 에셋 경고는 실행으로 확인하지 않았다.

- [KataTask_PlaySound.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_PlaySound.cpp): 재생 전 입력 전달, Seek 무음, 프리뷰 UI 사운드, 종료 정리.
- [KataTask_CameraShake.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_CameraShake.cpp): 대상 선택, 반경 감쇠, 셰이크 풀 재사용 대응.
- [KataTask_SpawnVFX.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Tasks/KataTask_SpawnVFX.cpp): 기준 메시 선택, 부착·위치 생성, 종료 정책.
- [작업 상태](https://github.com/jaykop/Kata/issues/7).
