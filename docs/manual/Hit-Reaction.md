# 피격 반응 사용법

갱신: 2026-10-17  
대상: 피격 반응을 만드는 캐릭터·전투 설정 담당 / KataFramework `UKataHitReactionAbility`, `UKataHitReactionGameplayEffectComponent`  
적용 기준: [피격 반응 계획](../plan/Hit-Reaction-Plan.md), [#23](https://github.com/jaykop/Kata/issues/23)  
확인 상태: 반응 판정, `Kata Action` 모드의 `Light` 반응, `Additive Montage` 모드의 `Flinch` 반응은 2026-10-10 사용자 PIE로 확인했다. 하이퍼아머 공격 중 흔들림은 샘플이 없어 확인 전이다. `Groggy` 샘플은 2026-10-11 사용자 PIE로 확인했다. 히트스톱([#72](https://github.com/jaykop/Kata/issues/72))은 2026-10-12 사용자 Editor 빌드와 실행으로 확인했다(PIE 공격자·피격자 정지, 30·60·144fps 몽타주·판정 일치, 무적 대상 미적용, 액션 프리뷰 Target 더미). 피격 연출([#73](https://github.com/jaykop/Kata/issues/73))은 2026-10-12 사용자 Editor 빌드와 PIE로 확인했다(PC 공격 명중 시 때린 쪽 화면 흔들림·패드 진동, 듀얼센스·XInput 패드, 적에게 맞을 때 맞은 쪽 화면 흔들림, 무적 대상 미재생). 맞을 때의 패드 진동은 따로 보고되지 않았고 Heavy 등급은 확인 전이다. `Medium`·`Heavy` 반응 샘플([#70](https://github.com/jaykop/Kata/issues/70))은 2026-10-17 사용자 PIE로 방향별 반응과 끝부분 이동 캔슬을 확인했다

## 목적과 준비

맞은 캐릭터가 공격의 강도와 방향에 맞게 흔들리거나 경직되게 한다. 피격은 세 단계로 나뉜다.

| 단계 | 담당 | 하는 일 |
|---|---|---|
| 공격 | Hit Trace의 `Apply Gameplay Effect` 처리기 | 공격마다 다른 값(피해 계수, Poise·Groggy 피해, Impact 등급)을 스펙에 담아 피해 GE를 적용한다([Hit Trace 사용법](Hit-Trace.md)) |
| 판정 | 피해 GE의 `Kata Hit Reaction` 컴포넌트 | 무적·피해·Poise·Groggy 처리가 끝난 뒤 반응 종류를 정해 맞은 쪽 ASC에 반응 이벤트를 한 번 보낸다([Attribute 사용법](Attributes.md)) |
| 반응 | `Kata Hit Reaction Ability`의 Blueprint 자식 | 반응 이벤트로 활성화되어 방향을 고르고 반응 Kata나 가산 몽타주를 재생한다 |

준비 조건은 다음과 같다.

- 맞는 캐릭터에 Stance 세트와 회복 GE가 있고, 피해 GE에 `Kata Hit Reaction` 컴포넌트가 있다([Attribute 사용법](Attributes.md)의 "경직 스탯(Stance) 구성").
- Kata Combat 설정의 Hit Reaction 항목에 반응 이벤트 태그가 지정되어 있다.
- 반응 Kata를 쓰려면 맞는 캐릭터에 `UKataActionComponent`가 있다.

## 사용 순서

1. Blueprint 클래스를 만들고 부모로 `Kata Hit Reaction Ability`를 고른다. 반응 종류마다 하나씩 만든다(예: `GA_HitReaction_Light`).
2. Class Defaults에서 다음을 채운다.
   - Triggers: Trigger Tag에 반응 이벤트 태그(예: `Event.HitReaction.Light`), Trigger Source는 Gameplay Event.
   - Activation Owned Tags: 반응 상태 태그(예: `Status.HitReaction.Light`). 반응이 재생되는 동안 ASC에 붙는다.
   - Activation Blocked Tags: 이 반응을 막을 상태. 다운·그로기 중 경직을 막으려면 `Status.HitReaction.Knock.Down`·`Status.HitReaction.Groggy`를 넣는다.
   - Mode와 방향별 데이터(아래 표).
3. 캐릭터 Gameplay Data의 Granted Abilities에 이 클래스를 넣는다.
4. PIE에서 맞혀 본다. `log LogKataFramework Verbose`로 판정 로그 `Kata hit reaction on ...`과 방향 로그 `Kata hit reaction ability ... direction ...`을 볼 수 있다.

## 주요 설정과 실행 규칙

| UI 항목 또는 API | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| `Mode` | `Kata Action`: 반응 Kata를 재생한다. `Additive Montage`: 가산 몽타주를 재생한다 | 기본 Kata Action |
| `Directional Actions` | 방향(Front·Back·Left·Right)별 반응 Kata | 고른 방향이 비면 Front를 쓴다. Front도 없으면 경고 후 바로 끝난다 |
| `Directional Montages` | 방향별 가산 몽타주 | 같은 규칙 |
| `Reset Groggy On End` | 반응이 끝날 때 Groggy를 0으로 되돌린다 | 기본 꺼짐. 그로기 반응에 켠다 |
| `ResolveHitDirection` | 맞은 쪽 기준 피격 방향을 고르는 Blueprint Pure 함수 | 아래 방향 규칙 |

방향 규칙:

- 방향은 맞은 캐릭터 기준으로 **공격이 들어온 쪽**이다. 휘청이는 쪽이 아니다. `Left`는 왼쪽에서 맞았다는 뜻이며, `Left`에 넣는 애니메이션은 보통 오른쪽으로 휘청인다.
- HitResult의 `TraceStart`→`TraceEnd`(접촉한 서브스텝의 무기 이동)에서 수평 성분이 Kata Combat의 Min Horizontal Direction Ratio(기본 0.5) 이상이면 이동의 반대쪽을 피격 방향으로 본다. 좌우는 맞은 쪽 기준이다. 무기가 맞은 쪽의 왼쪽에서 오른쪽으로 지나가면 Left이며, 마주 선 공격자가 자기 왼쪽에서 오른쪽으로 베면 Right가 된다.
- 수평 성분이 부족하거나(내려찍기) 이동이 없으면 공격자가 선 쪽으로 고른다. 공격자도 없으면 Front다.
- 맞은 쪽의 앞·오른쪽 방향과 비교해 더 가까운 축의 방향 하나를 고른다.

실행 규칙:

- Ability는 액터별 인스턴스이며, 같은 반응 이벤트가 다시 오면 진행 중인 반응을 끝내고 다시 시작한다.
- `Kata Action` 모드는 `UAbilityTask_PlayKataAction`으로 재생하며 공격자를 Kata의 대상으로 넘긴다. 반응 Kata는 현재 액션을 끊을 수 있도록 Blocking Policy의 Can Interrupt Active Kata를 켠다.
  반응 Kata가 끝나거나 끊기거나 시작이 거절되면 Ability가 끝나고 상태 태그가 사라진다.
- 반응 Kata는 이동을 따로 막지 않는다. 반응 중에 PC 이동 입력과 AI 경로 이동을 막으려면 반응 애니메이션의 Enable Root Motion을 켠다.
  루트 이동량이 0인 애니메이션도 켜 두면 루트 모션 몽타주가 Character Movement의 속도를 덮어 제자리에 선다. 공격·회피 몽타주와 같은 규칙이다([입력 사용법](Input.md)).
- 반응 중에 누른 공격·회피 입력은 공격·회피 Kata의 시작이 거절되어 버려진다. 끝부분의 Cancel Window(`Window.Cancel.Move`)로만 반응을 일찍 끝낼 수 있다.
- 반응 Kata가 현재 액션을 끊으면 PC 콤보 그래프는 끝나고 AI StateTree Task는 Failed를 반환한다.
- `Additive Montage` 모드는 엔진의 Play Montage And Wait로 재생하고 몽타주가 끝나면 Ability가 끝난다. 액션 슬롯을 쓰지 않으므로 현재 액션이 이어진다. 공격 몽타주와 다른 Slot Group을 쓴다.
- 맞은 캐릭터의 체력이 0이면 판정 컴포넌트가 로그 없이 반응 이벤트를 보내지 않는다. 사망 처리는 [사망 사용법](Death.md)을 따르며, 사망 정리가 진행 중인 반응 Ability를 취소한다.

## 피격 연출

`Apply Gameplay Effect` 히트 처리기가 GE 적용 성공을 확인하면 맞은 쪽과 때린 쪽의 연출을 Gameplay Cue로 재생한다. 체력 감소 여부는 확인하지 않으므로 피해가 0이거나 대미지 무시 상태여도 재생한다. 반응 판정과 별개라 Stance 세트가 없는 대상도 받는다.

| Cue | 담는 것 | 실행하는 액터 | 고르는 순서 |
|---|---|---|---|
| 맞은 쪽 | 피격자 종류·재질에 따른 VFX·SFX, 피격자 기준 흔들림·진동 | 피격자 | 처리기 `Hit Cue Tag` → Impact 행 `Hit Cue Tag` |
| 때린 쪽 | 무기·스킬에 따른 VFX·SFX, 공격자 기준 흔들림·진동 | 공격자 | 처리기 `Attacker Cue Tag` → Impact 행 `Attacker Cue Tag` |

1. Blueprint 클래스를 만들고 부모로 `GameplayCueNotify_Burst`를 고른다. Class Defaults의 Gameplay Cue Tag에 `GameplayCue` 아래 태그를 지정한다.
2. Burst Effects에 파티클·사운드·카메라 흔들림·포스 피드백 등을 넣는다.
3. 프로젝트 설정 Kata Combat의 Impact Responses 행에 등급별 기본 Cue를 지정한다. 특정 공격만 다르게 하려면 그 공격의 처리기에서 지정한다.

- 흔들림·진동·렌즈 효과는 항목의 Spawn Condition에서 Locally Controlled Source를 **Target Actor**, Policy를 Local Only로 둔다. Target Actor는 Cue를 실행한 액터이므로, 맞은 쪽 Cue는 피격자가, 때린 쪽 Cue는 공격자가 플레이어일 때만 재생한다. 기본값 Instigator Actor는 두 Cue 모두 공격자를 가리킨다.
- Burst Cue에서 카메라 흔들림·포스 피드백은 각각 한 칸이다. 파티클·사운드는 여러 항목을 넣고 항목마다 Surface Type 조건을 둘 수 있다.
- 배치 위치는 히트의 접촉 지점이고 방향은 HurtBox 표면의 바깥쪽이다.
- Cue는 복제하지 않는 경로로 실행한다. 무적 등으로 GE 적용이 막히거나 대상에 ASC가 없으면 재생하지 않는다.
- 재질 조건은 아직 동작하지 않는다. Hit Trace가 HitResult에 Physical Material을 넣지 않아 항상 Default 재질로 판정한다.
- 샘플: `Content/KataSample/GAS/Cues`의 `GCN_Hit_Light`·`GCN_Hit_Heavy`(맞은 쪽)와 `GCN_Attack_Light`·`GCN_Attack_Heavy`(때린 쪽, 흔들림 배율 0.5)는 카메라 흔들림(`CS_Hit_Light`·`CS_Hit_Heavy`)과 포스 피드백(`FFE_Hit_Light`·`FFE_Hit_Heavy`, 때린 쪽은 둘 다 Light)만 담는다. Burst의 포스 피드백에는 세기 배율이 없어 때린 쪽은 약한 에셋을 쓴다. VFX·SFX는 아직 없고, `Knock.Back`·`Knock.Down` 등급의 Cue 에셋도 없다.

## 히트스톱

`Apply Gameplay Effect` 히트 처리기가 GE 적용 성공을 확인하면 공격자와 피격자에게 히트스톱을 요청한다. 체력 감소 여부는 확인하지 않으므로 피해가 0이거나 대미지 무시 상태여도 요청한다. 반응 판정과 별개라 Stance 세트가 없는 대상에도 적용한다.

1. 공격의 Hit Trace `Apply Gameplay Effect` 처리기에서 `Hit Stop Duration`(초)을 0보다 크게 둔다([Hit Trace 사용법](Hit-Trace.md)).
2. 필요하면 프로젝트 설정 Kata Combat의 Hit Stop 항목에서 배율과 Priority를 바꾼다.

| UI 항목 | 의미·입력 | 기본값·빈 값·실패 시 동작 |
|---|---|---|
| `Hit Stop Duration` (처리기) | 공격별 요청 유지 시간(초). [시간 배율 요청](Time-Dilation.md)의 만료 시계로 재며 엔진 일시정지 중에는 멈춘다 | 0이면 걸지 않는다. 지정 시간이 지난 첫 Subsystem Tick에서 해제된다 |
| `Hit Stop Time Dilation` (Kata Combat) | 멈추는 동안의 시간 배율. 0이면 완전히 멈춘다 | 0.05 |
| `Hit Stop Priority` (Kata Combat) | [시간 배율 요청](Time-Dilation.md)의 Priority | 100. Time Dilation 태스크의 기본 0보다 높아 슬로모션 구간 중에도 이긴다 |

- 처리기가 GE를 적용한 직후 적용 결과를 확인하고 건다. 무적 등으로 적용이 막히거나 대상에 ASC가 없으면 걸지 않는다.
- 요청은 `UKataTimeDilationSubsystem`으로 공격자와 피격자의 `CustomTimeDilation`에 건다. Kata 실행기([런타임 사용법](Runtime-Usage.md))와 몽타주, Hit Trace가 모두 이 값을 따라 함께 느려진다.
- 한 공격이 같은 프레임에 여러 대상을 맞히면 공격자는 가장 늦게 끝나는 요청까지 멈춘다.
- 처리기를 거치지 않는 피해(낙사·환경 피해 등)에는 걸리지 않는다. 방어 결과별 히트스톱 배율은 [#87](https://github.com/jaykop/Kata/issues/87) 범위다.
- 멈추는 공격자는 Kata Context의 Avatar다. Kata 실행기를 소유한 액터가 Avatar와 다르면 실행기 타임라인은 멈추지 않는다.

## 가산 반응 준비

`Additive Montage` 모드는 가산 애니메이션, 별도 Slot Group의 슬롯, 그 슬롯을 지나는 AnimGraph가 필요하다. 순서를 지킨다.

1. 스켈레톤의 Anim Slot Manager에서 새 그룹과 슬롯을 만든다. 샘플은 `AdditiveGroup` 그룹의 `HitReactionAdditive` 슬롯이다.
   그룹을 먼저 만들지 않으면 엔진이 슬롯을 `DefaultGroup`에 등록하거나 그 그룹으로 취급하므로 가산 몽타주가 공격 몽타주를 멈춘다.
2. AnimBlueprint의 AnimGraph에서 기존 `DefaultSlot` 노드와 Output Pose 사이에 1의 슬롯 노드를 넣는다. 슬롯 노드는 가산 몽타주를 들어온 포즈 위에 더한다.
3. 반응 애니메이션의 Additive Anim Type과 Base Pose Type을 정한다. 가산 몽타주를 만든 뒤 Montage의 슬롯을 1의 슬롯으로 바꾼다.

DS3 가산 원본은 FBX가 가산 정보를 담지 못해, 변환기가 캐릭터의 대기 자세 첫 프레임 위에 원본 규칙대로 합성한 일반 클립으로 들어온다.
이 클립은 `Animations/AdditiveSource/<무기>/AS_<캐릭터>_AdditiveSource_<원본 번호>_<무기>`에 있다(2026-10-11 이전 이름은 `PoseCorrections/.../PoseBaked_*`).
흑기사의 기준 대기 자세는 Greatsword 대기다. 클립을 그대로 재생하면 대기 자세에서 맞아 흔들리는 모습이 보인다.

1. `AdditiveSource` 클립을 복제하고 Additive Anim Type을 `Local Space`, Base Pose Type을 `Local Anim Frame`, 프레임을 0으로 둔다.
   원본 delta는 첫 프레임이 0이므로 프레임 0이 곧 기준 대기 자세다. 원본 `AdditiveSource` 클립은 비교용으로 남긴다.
2. 복제한 클립으로 가산 몽타주를 만든다.

UE 가산은 기준 자세와의 차이를 현재 자세 앞에 곱한다. 원본 DS3 가산은 뼈 로컬 공간에서 뒤에 곱하므로, 기준 대기 자세에서 크게 벗어난 자세(크게 휘두르는 공격 등)에서는
원본과 조금 다르게 흔들린다. 진단과 수치는 [DS3 가산 애니메이션 공간 devlog](../devlog/2026-10-11-DS3-Additive-Child-Space.md)에 있다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 판정 로그는 나오는데 반응이 없다 | 이벤트 태그에 맞는 반응 Ability가 부여되지 않았거나 Activation Blocked Tags에 걸렸다 | Gameplay Data의 Granted Abilities와 Trigger Tag를 확인한다 |
| 반응 Kata가 재생되지 않고 바로 끝난다 | 현재 액션의 차단 정책에 걸려 시작이 거절됐다 | 반응 Kata의 Can Interrupt Active Kata와 현재 액션의 Blocked Kata Tags를 확인한다. Verbose 로그에 거절 결과가 남는다 |
| 가산 반응 중 공격이 끊기거나 이상하다 | 가산 슬롯이 공격 몽타주와 같은 Slot Group에 있다. 그룹을 저장하지 않으면 AnimGraph 컴파일 때 슬롯이 `DefaultGroup`에 다시 등록된다 | 스켈레톤의 Anim Slot Manager에서 슬롯을 별도 그룹으로 옮기고 스켈레톤을 저장한다 |
| 반응 중에 캐릭터가 이동한다 | 반응 애니메이션의 루트 모션이 꺼져 있어 이동 입력·AI 경로 이동이 그대로 적용된다 | 반응 AnimSequence의 Enable Root Motion을 켠다 |
| Enable Root Motion을 켰는데 몸만 밀렸다가 제자리로 돌아온다 | 원본 클립에 루트 이동이 없다. 변환 때 별도 이동 정보가 있는 클립만 루트로 옮긴다(흑기사 `Light`·`Medium`·`Heavy`) | 클립 문제가 아니다. 캐릭터를 실제로 밀어내는 기능은 [#120](https://github.com/jaykop/Kata/issues/120)에서 다룬다 |
| 맞았는데 반응이 없다(판정 로그는 `Flinch`) | `Flinch` 반응 Ability가 없거나, 다른 경직 반응 중이라 Activation Blocked Tags에 걸렸다 | `Flinch` Ability 부여와 차단 태그를 확인한다. 샘플은 `Light`·`Medium`·`Heavy` 중 `Flinch`를 막는다 |
| 몇 번 맞힌 뒤부터 반응도 판정 로그도 없다 | 체력이 0이 되어 판정 컴포넌트가 이벤트를 보내지 않는다 | GAS Inspector에서 Health와 `Status.Dead`를 확인한다. 사망 Ability가 없으면 죽은 대상은 연출 없이 바로 사라진다([사망 사용법](Death.md)) |
| 가산 흔들림 중 다리가 크게 들렸다 돌아온다 | 가산 클립을 이전 변환 규칙(부모 공간, Reference Pose 기준)으로 합성했다 | 수정된 변환기로 다시 추출해 Reimport하고 Base Pose Type을 `Local Anim Frame` 0으로 둔다 |
| 가산 흔들림이 대기 자세와 다른 모양으로 나온다 | Base Pose Type이 클립을 합성한 기준 자세와 다르다 | DS3 변환 클립은 `Local Anim Frame` 0을 쓴다 |
| 어떤 반응·방향이 골라졌는지 화면에서 알기 어렵다 | 판정 결과는 Verbose 로그로만 남는다 | 스포너 `Dummy` 설정의 `Show Hit Info`(기본 켬)로 만든 더미는 머리 위에 반응 이벤트 태그·방향·계산 피해·HP를 보인다([스포너 사용법](Spawner.md#더미로-쓰기)). 방향은 `ResolveHitDirection`과 같은 판정이다 |

## 확인 상태와 근거

반응 판정은 2026-10-10 사용자가 보스 흑기사를 상대로 한 PIE 로그로 Flinch, Light와 Poise 채움, Groggy, 사망 후 판정 없음을 확인했다.
같은 날 샘플 `GA_BlackKnight_HitReaction_Light`와 4방향 반응 Kata로 PC·보스 AI 양쪽에서 방향별 경직, 공격 중단, AI 복귀, 끝부분 이동 캔슬,
연속 경직 재시작, 상태 태그 부착·제거, 루트 모션으로 반응 중 이동이 막히는 것을 사용자 PIE로 확인했다.
같은 날 `GA_BlackKnight_HitReaction_Flinch`로 가산 흔들림 재생, 공격·이동 유지, `Light` 중 차단, 사망 후 반응 없음을 사용자 PIE로 확인했고,
2026-10-11 변환기를 원본 가산 공간대로 고쳐 다시 추출한 뒤, 흔들림 중 다리가 들리던 문제가 사라진 것을 사용자가 육안으로 확인했다. 하이퍼아머 공격 중 흔들림은 확인 전이다.

샘플 에셋은 다음과 같다. 방향은 피격자 기준이다.

| 반응 | Ability | 애니메이션 | F·B·L·R 원본 |
|---|---|---|---|
| `Light` | `Characters/BlackKnight/GameplayAbility/GA_BlackKnight_HitReaction_Light` | `Kata/KA_BlackKnight_HitReaction_Light_*`, `Art/.../HitReaction/Greatsword/AM_BlackKnight_HitReaction_Light_*` | 008000·008001·008002·008003 |
| `Medium` | `Characters/BlackKnight/GameplayAbility/GA_BlackKnight_HitReaction_Medium` | `Kata/KA_BlackKnight_HitReaction_Medium_*`, `Art/.../HitReaction/Greatsword/AM_BlackKnight_HitReaction_Medium_*` | 008010·008011·008012·008013 |
| `Heavy` | `Characters/BlackKnight/GameplayAbility/GA_BlackKnight_HitReaction_Heavy` | `Kata/KA_BlackKnight_HitReaction_Heavy_*`, `Art/.../HitReaction/Greatsword/AM_BlackKnight_HitReaction_Heavy_*` | 008020·008021·008022·008023 |
| `Flinch` | `Characters/BlackKnight/GameplayAbility/GA_BlackKnight_HitReaction_Flinch` | `Art/.../HitReaction/Sword/AM_BlackKnight_HitReaction_Flinch_*`(슬롯 `HitReactionAdditive`), 가산 클립 `AS_BlackKnight_HitReaction_Additive_00900*_Sword` | 009000·009001·009003·009002 |
| `Groggy` | `Characters/BlackKnight/GameplayAbility/GA_BlackKnight_HitReaction_Groggy`(Reset Groggy On End 켬) | `Kata/KA_BlackKnight_HitReaction_Groggy_*`, `Art/.../HitReaction/Retargeted/AM_BlackKnight_HitReaction_Groggy_*` | 용사냥 갑옷 c3160의 008700·008701·008702·008703을 리타게팅 |

`Light`의 좌우는 원본 데이터에서 머리가 밀리는 방향으로 확인했다(008002는 오른쪽으로 밀리므로 왼쪽 피격). 2026-10-11 사용자도 몽타주 프리뷰에서 R이 오른쪽에서 맞아 왼쪽으로 휘청이는 것을 확인했다. `Flinch`의 좌우는 흔들림이 오가서 데이터로 판단하지 못한 추정값이다.
변환한 원본 클립 이름에도 측정한 방향이 들어 있다(예: `AS_BlackKnight_HitReaction_L_008002_Greatsword`). 이름에 `Candidate`가 남은 반응은 방향을 데이터로 확정하지 못한 클립이다.
`Medium`·`Heavy` 원본은 이름에 `Candidate`가 남아 있지만, 2026-10-17 사용자가 몽타주 프리뷰에서 008010~008013과 008020~008023이 각각 F·B·L·R 피격 순서임을 확인했다.
두 반응은 `Light`와 같은 구조다. 반응 Kata는 몽타주(Blend In 0.1초)를 끝까지 재생하고, 이동 캔슬 창이 `Medium` 0.80초, `Heavy` 1.17초부터 끝까지 열린다. 이 시각은 원본 TAE의 끝 캔슬 시작이다.
Activation Blocked Tags는 `Status.HitReaction.Knock.Down`·`Status.HitReaction.Groggy`이고, `Flinch`의 Activation Blocked Tags에 `Status.HitReaction.Medium`·`Status.HitReaction.Heavy`를 더해 두 반응 중에는 흔들림이 나지 않는다.
부여는 `Groggy`와 같이 `DA_GameplaySet_BlackKnight`와 PC `DA_GameplaySet`이다. 은기사·사냥개 세트에는 없어서, 이 두 캐릭터는 `Medium`·`Heavy` 공격에 맞으면 반응하지 않는다.
샘플 공격은 `KA_BlackKnight_Attack01`이 `Impact.Medium`, `KA_BlackKnight_Attack02`가 `Impact.Heavy`다. Poise가 남아 있는 동안은 `Flinch`가 나오고, 무너진 뒤 맞은 공격의 등급 반응이 나온다.
`Medium`·`Heavy` 원본 클립에는 루트 이동이 없어 몸이 밀렸다가 제자리로 돌아온다. 반응 중 캐릭터를 실제로 밀어내는 기능은 [#120](https://github.com/jaykop/Kata/issues/120)에서 다룬다.
경로는 `/Game/KataSample/` 기준이며 `Art/...`는 `Art/Characters/BlackKnight/Animations`다. 흑기사에는 Greatsword용 가산 클립이 없어 Sword 클립을 쓴다.
`ABP_CharacterBase`의 AnimGraph에 `HitReactionAdditive` 슬롯 노드가 있고, `SK_BlackKnight`에 `AdditiveGroup` 그룹이 있다.

`Groggy`는 흑기사 원본에 강인도 붕괴 클립이 없어 같은 DS3 상태의 용사냥 갑옷 클립을 흑기사로 리타게팅한 임시 에셋을 쓴다. 3.67초 동안 밀리며 무릎을 꿇었다가 일어난다.
이동 캔슬 창은 원본 TAE의 끝 캔슬 시작에 맞춰 3.07초부터 열린다. `Light`·`Flinch`의 Activation Blocked Tags에 `Status.HitReaction.Groggy`가 있어 그로기 중에는 새 경직이 나지 않는다.
부여는 `DA_GameplaySet_BlackKnight`와 PC `DA_GameplaySet`이며, Skeleton이 다른 은기사 세트에는 넣지 않았다. PIE에서는 `KataGroggy Nearest` 치트([치트 사용법](Cheats.md))로 바로 확인할 수 있다.
2026-10-11 사용자 PIE에서 치트로 보스와 PC 양쪽의 그로기 재생, 그로기 중 새 경직 없음, 종료 뒤 AI 복귀와 Groggy 0·상태 태그 제거, 3.07초 이후 이동 캔슬을 확인했다. 실제 공격으로 Groggy를 채운 경우는 확인하지 않았다.

- [KataHitReactionAbility.h](../../Plugins/KataFramework/Source/KataFramework/Public/HitReaction/KataHitReactionAbility.h): 반응 Ability와 방향 선택.
- [KataHitReactionEffectComponent.h](../../Plugins/KataFramework/Source/KataFramework/Public/Attributes/KataHitReactionEffectComponent.h): 반응 판정과 반응 이벤트.
- [작업 상태](https://github.com/jaykop/Kata/issues/23).
