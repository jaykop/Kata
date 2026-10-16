# 연출 태스크 사용법

갱신: 2026-10-17  
대상: 액션 타임라인의 Play Sound·Camera Shake(코어 `KataRuntime`), Spawn VFX·Trail(`KataFramework`)  
적용 기준: [연출 태스크 결정 기록](../devlog/2026-10-14-FX-Tasks.md), [#7](https://github.com/jaykop/Kata/issues/7), Trail은 [#109](https://github.com/jaykop/Kata/issues/109)  
확인 상태: 2026-10-14 사용자 Editor 빌드와 프리뷰·PIE 확인. Trail은 2026-10-15 사용자 Editor 빌드와 프리뷰·PIE 확인. 범위는 아래 "확인 상태와 근거"를 따른다

## 목적과 준비

휘두르는 소리, 칼날 궤적, 내려찍기 흔들림처럼 **액션 타임라인의 시각에 맞춰** 내는 연출을 다룬다.
맞는 순간의 이펙트·소리·흔들림·진동은 이 태스크가 아니라 히트 처리기의 Gameplay Cue가 맡는다([피격 반응 사용법](Hit-Reaction.md)).

- Play Sound·Camera Shake는 코어만 있으면 쓸 수 있다. Play Sound는 엔진 모듈 AudioExtensions를 쓰며 MetaSound 플러그인에 의존하지 않는다.
- Spawn VFX와 Trail은 `KataFramework`에 있으며 엔진 Niagara 플러그인이 켜져 있어야 한다. `KataFramework.uplugin`이 Niagara를 의존으로 선언한다.
- 네 태스크 모두 Add Task 메뉴의 **FX** 묶음에 있고 실행 단계(Phase)는 Presentation이다.

## 사용 순서

1. 액션 편집기에서 Add Task → FX → Play Sound / Camera Shake / Spawn VFX / Trail을 추가한다.
2. Timeline Details에서 에셋(Sound, Shake Class, System)을 지정하고 Start Time을 연출 시점에 맞춘다.
3. Trail은 Single Frame을 끄고 Duration을 0보다 큰 궤적 구간 길이로 지정한다.
4. Play Sound·Camera Shake·Spawn VFX에서 한 번 내고 스스로 끝나는 연출은 Duration 0과 Completed Policy Keep Playing으로 둔다. 구간이 끝날 때 거둘 연출은 Duration을 구간 길이로 두고 Completed Policy를 Stop 계열로 고른다.
5. 프리뷰에서 ▶로 재생해 시점을 맞추고, PIE에서 실제 캐릭터로 확인한다.

## 주요 설정과 실행 계약

### 공통

| 상황 | 동작 |
|---|---|
| 정상 완료(Completed) | Completed Policy를 따른다. 기본값은 끝까지 재생이다 |
| 실행 중인 태스크의 중단·취소·Kata 종료 | 정책과 관계없이 정리한다. Sound는 Fade Out Time 동안 줄이고, Shake는 블렌드 아웃하고, VFX는 방출을 멈춘다 |
| 루프 에셋 + Keep Playing | 스스로 끝나지 않으므로 정리 쪽으로 처리하고 데이터 검증에서 경고한다 |
| 재생·생성 실패 | 태스크를 일찍 완료하지 않는다. 의존으로 이어진 태스크의 시작 시각이 연출 성공 여부에 따라 바뀌지 않게 하기 위해서다 |

이미 Completed로 끝난 태스크는 이후 Kata가 취소되어도 종료 처리를 다시 받지 않는다. Keep Playing으로 남긴 연출은 그대로 재생된다. 취소 시 회수해야 하는 연출은 태스크가 그 시점까지 실행 중이도록 Duration을 설정한다.

### Play Sound

| 항목 | 의미 | 기본값·실패 시 |
|---|---|---|
| Sound | Sound Wave, Sound Cue, MetaSound Source | 비우면 설정 오류 |
| Attach To Mesh | 켜면 Avatar의 스켈레탈 메시 소켓에 붙어 따라간다. 끄면 시작 순간의 소켓 위치에 남는다 | 켬. 메시가 없으면 Avatar 루트 컴포넌트를 기준으로 한다 |
| Attach Socket | 기준 소켓 | 비우면 메시 원점 |
| Volume Multiplier, Pitch Multiplier | 볼륨·피치 배율 | 1. Pitch가 0 이하이면 설정 오류 |
| Attenuation Override | 감쇠 설정 | 비우면 사운드 에셋 설정 |
| Audio Parameters | 재생 전에 넘길 MetaSound 입력. 이름과 타입이 MetaSound 입력과 같아야 한다 | 태스크는 입력 이름과 타입의 유효성을 직접 검사하지 않는다 |
| Completed Policy | Keep Playing / Stop | Keep Playing |
| Fade Out Time | 멈출 때 줄이는 시간(초). 중단될 때도 쓴다 | 0.2. 0이면 즉시 멈춘다 |

### Camera Shake

| 항목 | 의미 | 기본값·실패 시 |
|---|---|---|
| Shake Class | 흔들림 클래스 | 비우면 설정 오류 |
| Scale | 세기 배율. 반경 모드에서는 거리 감쇠를 곱한다 | 1 |
| Target | Avatar Camera: Avatar를 조종하는 로컬 플레이어 카메라만 흔든다. AI가 실행하면 아무도 흔들리지 않는다. Local Players In Radius: Avatar 위치를 진원으로 반경 안의 로컬 플레이어 카메라를 흔든다 | Avatar Camera |
| Inner Radius, Outer Radius, Falloff | 반경 모드 전용. 진원에서 카메라 위치까지의 거리가 Inner 이하면 Scale 그대로, Inner보다 멀면서 Outer 이상이면 흔들지 않고, 그 사이는 Falloff 지수 곡선으로 줄인다 | 300cm, 1500cm, 1(선형). Outer가 Inner보다 작으면 설정 오류 |
| Completed Policy | Keep Playing / Stop(블렌드 아웃) | Keep Playing |

보스 AI의 공격이 플레이어 카메라를 흔들게 하려면 Local Players In Radius를 쓴다.
끝난 셰이크 참조는 태스크 Tick과 종료 때 해제한다. 그 사이 같은 셰이크 객체가 종료된 뒤 다른 곳에서 재사용되면 이전 재생과 새 재생을 구분하지 못하므로, 태스크 종료가 새 흔들림을 멈출 가능성이 남아 있다.

### Spawn VFX

| 항목 | 의미 | 기본값·실패 시 |
|---|---|---|
| System | Niagara 시스템 | 비우면 설정 오류 |
| Attach To Mesh | 켜면 기준 메시 소켓에 붙어 따라간다. 끄면 시작 순간의 소켓 위치에 생성하고 따라가지 않는다 | 켬 |
| Mesh Source, Weapon Slot | Hit Trace와 같은 규칙으로 기준 메시를 고른다. Avatar의 Kata Hit Box 컴포넌트가 Character·Weapon 메시를 제공하고, 컴포넌트가 없으면 Character만 ACharacter의 메시로 대신하며, ACharacter가 아니면 첫 스켈레탈 메시를 찾는다 | Character. 메시를 찾지 못하면 경고 후 생성하지 않는다 |
| Attach Socket | 기준 소켓 | 비우면 메시 원점 |
| Location Offset, Rotation Offset, Scale | 소켓 기준 오프셋과 크기 | 0, 0, 1 |
| Completed Policy | Keep Playing / Stop Emitting(남은 파티클은 수명대로) / Destroy Immediately | Keep Playing |

Niagara 컴포넌트 풀링은 쓰지 않는다. 풀로 돌아간 컴포넌트가 다른 생성에 재사용되면 태스크가 남의 이펙트를 정리할 수 있기 때문이다.

### Trail

칼날·팔다리처럼 소켓을 따라 놓인 부위가 태스크 구간 동안 지나간 자리를 Niagara 리본으로 그린다.
소켓 경로는 Hit Trace와 같은 방법으로 계산한다. 그래서 저프레임이나 한 프레임보다 짧은 구간에서도 판정과 같은 호와 구간 경계를 따른다([Hit Trace 사용법](Hit-Trace.md)).

| 항목 | 의미 | 기본값·실패 시 |
|---|---|---|
| System | 아래 User 파라미터 계약을 읽어 리본을 만드는 Niagara 시스템 | 비우면 설정 오류. 계약의 출력 파라미터가 없으면 데이터 검증이 경고하고, 실행 중에는 없는 파라미터에 값을 넘기지 않는다 |
| Mesh Source, Weapon Slot | Spawn VFX와 같은 규칙으로 기준 메시를 고른다. Character는 몸의 소켓(예: 팔꿈치→손, 무릎→발), Weapon은 장비 메시의 소켓을 쓴다 | Weapon. 메시를 찾지 못하면 경고 후 그리지 않는다 |
| Sockets | 궤적의 폭을 이루는 소켓 2개 이상. 한쪽 끝부터 순서대로 넣는다. 이웃한 두 소켓 사이마다 리본 하나가 생긴다 | 2개 미만이거나 빈 이름이 있으면 설정 오류. 메시에 없는 소켓이 있으면 경고 후 그리지 않는다 |
| Duration | 궤적을 그리는 구간 길이 | 0 이하이거나 Single Frame이면 설정 오류 |

실행 규칙:

- 시작할 때 Niagara 컴포넌트를 기준 메시에 붙여 만든다(풀링 없음). 입자 위치는 월드 좌표로 넘기므로 이미터를 월드 공간(Local Space 끔)으로 두면 컴포넌트가 메시를 따라가도 리본은 휘두른 자리에 남는다. 컴포넌트는 Solo 모드와 마지막 틱 그룹으로 실행해, 같은 프레임에 넘긴 샘플을 그 프레임에 그린다.
- 매 프레임 포즈가 확정된 뒤 `UKataHitSubsystem`이 이번 프레임 구간을 샘플링해 넘긴다. 첫 프레임은 구간 시작점부터 넘기고, 이후 프레임은 직전 프레임의 마지막 샘플에서 이어 간다.
- 정상 완료로 끝나면 종료 시각까지 넘긴 뒤 방출을 멈춘다. 중단·취소·Kata 종료로 끝나면 더 넘기지 않고 방출을 멈춘다. 이미 그린 입자는 수명대로 사라진다.
- 방출을 멈춘 뒤에도 컴포넌트가 사라질 때까지 Avatar의 Custom Time Dilation을 따른다. 그래서 히트스톱 동안 궤적도 함께 멈춘다.
- 생성이 실패하거나(거리 컬링 포함) 메시·소켓이 없어도 태스크를 일찍 끝내지 않는다.

Niagara User 파라미터 계약(이름은 User 네임스페이스 아래):

| 방향 | 이름 | 타입 | 뜻 |
|---|---|---|---|
| 입력 | `KataTrailSampleDistance` | float | 샘플 수 계산에 쓰는 소켓 이동 거리 기준(cm). 없거나 0 이하면 10, 0보다 크고 1 미만이면 1로 본다 |
| 입력 | `KataTrailSampleAngle` | float | 샘플 수 계산에 쓰는 첫 소켓→끝 소켓 방향의 회전각 기준(도). 없거나 0 이하면 10, 0보다 크고 1 미만이면 1로 본다 |
| 출력 | `KataTrailCount` | int | 이번 프레임에 새로 생성할 입자 수. 새 샘플이 없으면 0 |
| 출력 | `KataTrailPositions` | Position Array | 소켓 구간의 중점(월드) |
| 출력 | `KataTrailWidths` | Float Array | 소켓 구간의 길이(cm). 리본 폭으로 쓴다 |
| 출력 | `KataTrailSides` | Vector Array | 앞 소켓에서 뒤 소켓으로 향하는 단위 벡터. 리본의 Custom Side Vector로 쓴다 |
| 출력 | `KataTrailRibbonIds` | Int32 Array | 소켓 구간 번호(0부터). Ribbon ID로 쓴다 |
| 출력 | `KataTrailLinkOrders` | Float Array | 궤적 시작부터 샘플 시각까지의 Kata 시간(초). Ribbon Link Order로 쓴다 |
| 출력 | `KataTrailAges` | Float Array | 샘플 시각부터 이번 프레임까지 지난 Kata 시간(초). 생성할 때 입자 나이로 쓴다 |
| 출력 | `KataTrailBladeRanges` | Vector2D Array | 칼날 전체 길이 대비 그 구간의 시작·끝 비율. 칼날 폭 방향 UV에 쓴다 |

입력 간격과 출력 파라미터의 존재 여부는 궤적을 등록할 때 한 번 읽는다. 출력 파라미터의 존재 여부는 이름만 비교하므로 타입은 위 표와 맞춰야 한다.
출력 배열은 이번 프레임에 생성할 입자 하나당 한 항목이며 길이는 Count와 같다. 궤적 하나의 프레임별 샘플 수는 이동 거리와 회전각을 각 기준값으로 나눈 값 중 큰 쪽을 올림해 정한다. 진행 구간이 있으면 최소 1개, 최대 64개다(첫 프레임의 구간 시작점은 따로 센다). 경로를 근사 측정하고 샘플 수 상한도 적용하므로, 설정한 거리·각도가 실제 샘플 간격의 상한을 보장하지는 않는다.
이미터는 매 프레임 Count만큼 입자를 즉시 생성하고, 이번 프레임에 생성하는 입자의 실행 번호로 배열을 읽어 위 속성을 설정한다. 리본 렌더러는 Facing Mode를 Custom Side Vector로 두고 Ribbon ID와 Link Order를 바인딩한다.

## 프리뷰와 게임의 차이

| 태스크 | 프리뷰 ▶ 재생 | 재생 헤드 Seek | PIE·게임 |
|---|---|---|---|
| Play Sound | UI 사운드로 재생하고 공간화를 끈다. 감쇠 설정은 별도로 해제하지 않는다 | Seek로 시간을 진행하는 동안 새 소리를 재생하지 않는다. 처음부터 다시 시작할 때 시각 0의 태스크는 재생될 수 있다 | Attenuation과 부착 위치대로 들린다 |
| Camera Shake | 흔들리지 않는다(프리뷰에 PlayerController가 없음) | 같음 | 흔들린다 |
| Spawn VFX | 표시한다 | 표시한다. 뒤로 Seek하면 처음부터 다시 시뮬레이션한다 | 표시한다 |
| Trail | 표시한다. 단, 출력 배열은 CPU 이미터에만 전달된다 | 앞으로 Seek하면 지나간 구간을 그린다. 뒤로 Seek하면 처음부터 다시 시뮬레이션한다 | 표시한다 |

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 할 일 |
|---|---|---|
| 프리뷰에서 재생 헤드를 끌 때 소리가 안 난다 | Seek로 시간을 진행하는 동안 Play Sound는 새 소리 재생을 생략한다. 이미 재생 중인 소리를 멈추는 처리는 아니며, 처음부터 다시 시작할 때 시각 0의 태스크는 오디오 차단 전에 시작한다 | 소리는 ▶ 재생으로 확인한다. Seek 전체의 무음은 보장하지 않는다 |
| MetaSound 입력이 반영되지 않는다 | Audio Parameters의 이름·타입이 MetaSound 입력과 다르다 | Param Name 철자와 Param Type을 맞춘다 |
| 이펙트가 소켓을 따라오지 않고 생성 위치에 남는다 | Niagara 이미터가 월드 공간(Local Space 꺼짐)이라 파티클이 생성 위치에 남는다. 태스크 부착과는 별개다 | 따라가야 하는 이펙트는 이미터의 Local Space를 켠다. 궤적처럼 지나간 자리에 남기려면 월드 공간에서 계속 방출한다 |
| 이펙트가 엉뚱한 위치(메시 원점)에 생긴다 | Attach Socket이 그 메시에 없다 | 메시의 소켓 이름을 확인한다. 소켓 이름과 실제 칼끝 위치가 다를 수 있다 |
| Output Log에 `Kata VFX task ... found no weapon mesh` | Weapon 슬롯에 장착한 메시가 없거나 Kata Hit Box 컴포넌트가 없다 | 장비를 장착하거나 Mesh Source를 Character로 둔다 |
| 보스 공격에서 카메라가 흔들리지 않는다 | Target이 Avatar Camera면 AI 실행에서는 흔들 카메라가 없다 | Local Players In Radius를 쓴다 |
| 궤적이 그려지지 않는다 | System에 `KataTrail` 출력 파라미터가 없거나 이미터가 Count·배열을 읽지 않는다. 또는 Sockets 중 메시에 없는 소켓이 있다 | 데이터 검증 경고와 Output Log의 `Kata trail task ... needs socket` 경고를 확인한다 |
| 프리뷰에서만 궤적이 그려지지 않는다 | 이미터가 GPU 시뮬레이션이다. 프리뷰의 궤적 출력 배열은 CPU 이미터만 지원한다 | 궤적 이미터는 CPU 시뮬레이션으로 만든다 |
| 미지원 | Spawn VFX의 Niagara User Parameter 전달, 소켓 하나로 그리는 점 궤적, 타임라인에서 Gameplay Cue 실행, 프리뷰 카메라 흔들림 | 필요해지면 후속 작업으로 다룬다 |

## 확인 상태와 근거

2026-10-14 사용자가 Editor 빌드 뒤 샘플 `KA_BlackKnight_Attack01`로 확인했다. 확인용 에셋은 `MS_Test_Whoosh`(노이즈 엔벨로프 MetaSound, 입력 `Intensity`), `CS_Hit_Heavy`, `NS_Test_Spark`(DirectionalBurst 템플릿, 로컬 공간)다.

- Play Sound: 프리뷰 ▶ 재생 시 소리, Seek 중 무음, PIE 소리, Audio Parameters `Intensity` 값에 따른 크기 변화.
- Camera Shake: 플레이어 공격 흔들림, 보스 공격의 거리 감쇠, Avatar Camera에서 보스 공격 무반응, 프리뷰 무동작.
- Spawn VFX: Weapon 메시 소켓 부착, 프리뷰 표시, 플레이어·보스 공격 표시, 회피 캔슬 뒤 재생성 없음.
- Trail(2026-10-15, 확인용 에셋 `NS_Test_WeaponTrail`): 프리뷰·PIE 표시, `t.MaxFPS 20`에서 고프레임과 같은 호, 한 프레임보다 짧은 구간, 회피 캔슬 시점에서 멈춤, 히트스톱 동안 정지, `slomo 0.2`에서 앞끝과 꼬리, 데이터 검증의 `KataTrailBladeRanges` 누락 경고.

Completed Policy의 Stop 계열(Sound Stop, Shake Stop, VFX Stop Emitting·Destroy Immediately)과 Attach To Mesh 끔, 루프 에셋 경고는 실행으로 확인하지 않았다.

- [KataTask_PlaySound.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_PlaySound.cpp): 재생 전 입력 전달, Seek 진행 중 새 소리 재생 생략, 프리뷰 UI 사운드, 종료 정리.
- [KataTask_CameraShake.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Tasks/KataTask_CameraShake.cpp): 대상 선택, 반경 감쇠, 끝난 셰이크 참조 해제와 종료 정리. 풀 재사용 시의 한계는 Camera Shake 절을 따른다.
- [KataTask_SpawnVFX.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Tasks/KataTask_SpawnVFX.cpp): 기준 메시 선택, 부착·위치 생성, 종료 정책.
- [KataTask_Trail.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Tasks/KataTask_Trail.cpp): 설정 오류, 계약 검증, Niagara 생성 방식, 등록·닫기.
- [KataHitSubsystemTrail.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/HitTrace/KataHitSubsystemTrail.cpp): 샘플 수, 입자 배열 작성, 방출 정지와 시간 배율.
- [작업 상태](https://github.com/jaykop/Kata/issues/7), Trail은 [#109](https://github.com/jaykop/Kata/issues/109).
