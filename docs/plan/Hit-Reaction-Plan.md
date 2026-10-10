# 피격 반응 계획

작성: 2026-10-10  
갱신: 2026-10-10  
연결 이슈: [#23 피격 반응과 타격감](https://github.com/jaykop/Kata/issues/23), 실행 경로 결정은 [#46 Kata 실행을 범용 실행 GA로 모을지 결정](https://github.com/jaykop/Kata/issues/46)  
현재 상태 근거: [#23](https://github.com/jaykop/Kata/issues/23), [Hit Trace 사용법](../manual/Hit-Trace.md), [Attribute 사용법](../manual/Attributes.md), [런타임 사용법](../manual/Runtime-Usage.md)  
대체 관계: [액션 게임 기반 시스템 계획](Action-Game-Systems-Plan.md)의 "피격 반응 표현" 결정 필요 항목과 [기본 태스크 확장 계획](Base-Task-Plan.md)의 히트스톱 시계 정책을 이 문서가 이어받는다.

## 목적과 현재 상태

맞은 캐릭터가 공격의 강도와 방향에 맞게 흔들리거나 경직되거나 넘어지게 한다. 맞는 순간의 히트스톱과 연출(VFX·SFX·카메라 흔들림·패드 피드백)도 함께 다룬다.

이미 있는 것:

- 피해 경로: Hit Trace의 `UKataHitHandler_ApplyGameplayEffect` → `GE_Damage` → `UKataDamageExecution` → `UKataAttributeSet_Base::PostGameplayEffectExecute`. Health가 0이 되면 `OnOutOfHealth`를 보낸다.
- 처리기가 Effect Context에 HitResult와 출처 태스크를 넣는다. 반응 쪽은 이 Context에서 공격자와 히트 정보를 읽을 수 있다.
- Hit Trace는 프레임 사이를 `MaxStepDistance`·`MaxStepAngle` 기준의 substep으로 나누고 애니메이션 호를 다시 샘플링해 판정한다.
- `UKataActionComponent`는 한 번에 액션 하나만 실행한다. 외부 액션에 Interrupted로 끊기면 PC 그래프는 `EndGraph`로 끝나고 AI StateTree Task는 Failed를 반환한다.
- `UAbilityTask_PlayKataAction`은 즉시 종료도 실제 종료 사유로 알린다(#46 작업).

없는 것: Poise·Groggy 수치(Stance 세트), 반응 판정, 반응 이벤트, 반응 GA, 반응 Kata·가산 몽타주, 상태 태그, 히트스톱, 피격 연출 Cue.
샘플 공격 에셋은 Apply Gameplay Effect 처리기만 사용하며 `Event.Hit`을 보내는 곳은 없다.
`UKataExecutionWorldSubsystem`은 월드의 `DeltaSeconds`를 그대로 인스턴스에 넘기므로 액터의 Custom Time Dilation이 Kata 타임라인에 반영되지 않는다.

## 범위

- 포함: Stance 세트(Poise·Groggy), 반응 판정과 반응 이벤트, 반응 GA 기반 클래스와 반응 종류, 방향 선택, 무적·슈퍼아머·하이퍼아머, 재피격 규칙, 상태·Impact 태그, Hit Trace의 Impact 태그 전달과 무기 이동 방향 기록, 원본 가산 클립 변환, 샘플 반응 콘텐츠, 히트스톱, 맞는 순간의 연출 Cue.
- 제외: 히트 판정 자체([#6](https://github.com/jaykop/Kata/issues/6)), 액션 타임라인의 VFX·SFX 태스크([#7](https://github.com/jaykop/Kata/issues/7)), 입력 버퍼([#8](https://github.com/jaykop/Kata/issues/8)), 사망 처리(#41), 가드·가드 붕괴·패링, 그로기 처형, 넉업.

## 반응 흐름

1. 공격 액션의 Hit Trace가 `GE_Damage`를 적용한다. 처리기는 공격의 Impact 태그를 스펙의 동적 Asset Tag로 넣고, HitResult의 `TraceStart`·`TraceEnd`에 접촉이 일어난 substep의 시작·끝 위치를 담는다.
2. 맞은 쪽에 `Status.Invincible`이 있으면 `GE_Damage`의 Application Tag Requirements가 적용을 막는다. 피해·Poise·Groggy·연출·히트스톱이 함께 막힌다.
3. 피해가 적용되면 Impact에 대응하는 피격 연출 Cue를 실행하고 공격자·피격자에게 공격별 히트스톱을 건다. Stance 세트가 없는 대상도 연출과 히트스톱은 받는다.
4. 피해 Execution과 Stance Execution이 Health·Poise·Groggy를 줄인다.
5. Stance 세트가 결과를 확정한 뒤 반응 이벤트를 한 번 보낸다. 판정 순서는 아래 표를 따른다. Stance 세트가 없는 대상은 이벤트를 보내지 않는다.
6. 이벤트 태그로 반응 GA가 활성화된다. GA는 Payload의 Context에서 방향을 고르고 반응 Kata나 가산 몽타주를 재생한다.
7. 반응 Kata는 기존 교체 규칙으로 현재 액션을 끊는다. 반응이 끝나면 GA가 끝나고 상태 태그가 사라진다.

| 순서 | 조건 | 결과 |
|---|---|---|
| 1 | Health 0 | 사망(#41). 반응 이벤트를 보내지 않는다 |
| 2 | Groggy가 가득 참 | `Groggy` |
| 3 | `Status.SuperArmor` | `Flinch` |
| 4 | Poise 유지(하이퍼아머 보정 포함) | `Flinch` |
| 5 | Poise 붕괴 | Impact 태그에 대응하는 반응(`Light`·`Heavy`·`Knock.Back`·`Knock.Down`). Impact 태그가 없으면 설정의 Default Impact를 쓴다. Poise는 즉시 가득 채운다 |

## 확정 사항과 미확정 사항

결정일은 모두 2026-10-10이며 사용자 결정이다.

| 항목 | 구분 | 내용과 근거 또는 필요한 결정 |
|---|---|---|
| 실행 경로 | 확정 | 입력·그래프·StateTree·프리뷰는 `UKataActionComponent`를 직접 호출한다. 이벤트로 시작하는 반응은 반응 GA가 `UAbilityTask_PlayKataAction`으로 Kata를 재생한다. 모든 실행을 범용 GA로 모으지 않는다. GA 태그가 클래스에 고정되어 Kata별 태그를 표현할 수 없기 때문이다([#46](https://github.com/jaykop/Kata/issues/46)) |
| 반응 이벤트 출처 | 확정 | Stance 세트가 결과 확정 뒤 한 번 보낸다. Hit Trace 처리기에서 따로 보내면 무적으로 피해가 막혀도 반응이 나가는 불일치가 생긴다 |
| 반응 이벤트 설정 | 확정 | `UKataCombatSettings`에 "Impact 태그 → 반응 이벤트 태그" 표와 `Flinch`·`Groggy` 이벤트 필드를 둔다. 이름은 `Event.HitReaction.*`. 새 등급은 태그·표 항목·GA 자식만 추가하며 코드는 바꾸지 않는다 |
| 반응 GA 구조 | 확정 | KataFramework에 C++ 기반 GA 하나를 두고 Kata 재생 모드와 가산 몽타주 모드를 제공한다. 반응 종류마다 Blueprint 자식이 Trigger 이벤트, `ActivationOwnedTags`, 반응 데이터를 채운다 |
| 상태 태그 | 확정 | `Status.HitReaction.Flinch`·`Light`·`Heavy`·`Knock.Back`·`Knock.Down`·`Groggy`, `Status.SuperArmor`, `Status.Invincible`. 반응 GA의 `ActivationOwnedTags`가 붙이며 반응 Kata의 `ActiveGrantedTags`에는 넣지 않는다 |
| 부모 태그 의미 | 확정 | `Status.HitReaction`은 `Flinch`를 포함한다. 제어권 상실 판정은 부모 태그가 아니라 `Flinch`를 뺀 하위 태그를 나열해 확인한다 |
| `Status.Invisible` | 확정 | 읽는 기능이 없어 이번에 추가하지 않는다. 타게팅·퍼셉션이 필요할 때 추가한다 |
| Impact 등급 | 확정 | `Light`·`Heavy`·`Knock.Back`·`Knock.Down`. 서서 받는 경직은 2단계로 시작하고, 샘플 애니메이션 8000번대가 세 묶음으로 구분되면 `Medium`을 추가한다 |
| Impact 전달 | 확정 | `UKataHitHandler_ApplyGameplayEffect`에 Impact 태그 필드를 두고 `FGameplayEffectSpec::AddDynamicAssetTag`로 넣는다. `GE_Damage` 하나를 재사용한다 |
| Impact 태그가 없는 공격 | 확정 | `UKataCombatSettings`의 Default Impact 태그를 쓴다. 샘플 설정은 `Impact.Light`이며, 비우면 반응하지 않는다 |
| 슈퍼아머·하이퍼아머 | 확정 | `Status.SuperArmor`는 경직 없음이다. 일반 공격의 버티기는 기존 Apply Gameplay Effect 태스크로 Stance 세트의 `PoiseDamageTakenMultiplier`(받는 Poise 피해 배율)를 낮추는 GE를 걸어 표현한다. 현재 Poise를 직접 바꾸지 않으므로 구간이 끝나도 Poise가 튀지 않는다. 두 경우 모두 피해와 Poise 누적은 받는다 |
| Stance 세트가 없는 대상 | 확정 | 피해·사망·연출·히트스톱만 처리하고 반응하지 않는다. 항상 경직해야 하면 Stance 세트를 붙이고 `MaxPoise`를 0으로 둔다 |
| Poise 회복 | 확정 | 마지막 피격 뒤 지연 시간이 지나면 한 번에 가득 찬다. Poise가 무너져 반응하면 즉시 가득 찬다 |
| Groggy 감소 | 확정 | 마지막 피격 뒤 지연 시간이 지나면 일정 속도로 줄어든다. `Groggy` 반응이 끝나면 0으로 초기화한다 |
| 회복 지연 구현 | 확정 | 프로젝트의 회복 방식(Infinite Periodic GE와 Ongoing Tag Requirements)을 따른다. 피해 GE의 Additional Effects가 `Status.Stance.RecoveryDelay`를 붙이는 Duration GE를 적용하고, Poise 회복·Groggy 감소 GE는 이 태그가 있는 동안 멈춘다. Poise·Groggy는 같은 지연을 쓴다 |
| `Knock.Down` 범위 | 확정 | 넘어짐, 누움, 기상까지 하나의 반응이다. 기상 구간에만 `Status.Invincible`을 붙인다. 넉업을 추가하면 착지 뒤 `Knock.Down` 반응으로 잇는다 |
| 재피격 | 확정 | `Light`·`Heavy`·`Knock.Back` 중에는 새 반응으로 다시 시작한다. `Knock.Down`·`Groggy` 중에는 반응 GA의 `ActivationBlockedTags`로 새 반응을 막고 피해·Poise는 받는다 |
| 방향 기준 | 확정 | 무기 이동 방향을 쓴다. 이동 방향의 수평 성분이 50% 미만이거나 방향이 없으면 공격자 위치 기준으로 바꾼다. 기준값은 `UKataCombatSettings`에서 바꾼다 |
| 방향 기록 | 확정 | HitResult의 `TraceStart`·`TraceEnd`에 접촉이 일어난 substep의 시작·끝 위치를 넣는다. 프레임 전체 구간을 쓰면 호를 직선으로 잇는 오차가 커지기 때문이다. substep 기준이면 오차는 `MaxStepAngle`의 절반 이하다. 이전 위치가 없으면 두 값이 같아 대체 규칙을 쓴다 |
| 가산 흔들림 | 확정 | `Flinch`는 가산 몽타주 모드로 재생하고 현재 액션을 유지한다. 공격 몽타주와 다른 Slot Group을 쓴다. 같은 그룹의 몽타주는 서로를 멈추기 때문이다 |
| 가산 에셋 | 확정 | 은기사 가산 009 계열의 원본 기준 자세와 공간 규칙을 조사해 UE Additive로 변환한 뒤 샘플을 만든다. 현재 009 계열은 기준 자세에 합성한 미리보기다 |
| 반응 중 입력 | 확정 | 반응 Kata 끝부분 Cancel Window만 사용하고 창 밖 입력은 버린다 |
| 히트스톱 | 확정 | 공격자와 피격자의 Custom Time Dilation을 잠깐 낮춘다. 시간은 공격별 값으로 처리기에 둔다. Kata 실행기가 소유 액터의 Custom Time Dilation을 따르도록 코어 `KataRuntime`을 고친다 |
| 피격 연출 | 확정 | 엔진의 `UGameplayCueNotify_Burst` 하나에 VFX·SFX·카메라 흔들림·렌즈 효과·포스 피드백·입력 장치 속성·데칼을 담는다. `UKataCombatSettings`의 "Impact 태그 → Cue 태그" 표로 고른다. 히트스톱은 게임 진행에 영향을 주므로 Cue에 넣지 않는다 |
| 연출 범위 나누기 | 확정 | 맞는 순간의 연출은 이 Cue가 맡고, 휘두르는 소리 같은 타임라인 연출만 [#7](https://github.com/jaykop/Kata/issues/7)의 태스크로 남긴다 |
| 연출을 받는 플레이어 | 결정 필요 | Cue 효과는 효과마다 공격자나 피격자 중 한쪽의 로컬 플레이어만 고른다. 때릴 때와 맞을 때 모두 흔들려면 Cue를 둘로 나눌지, 한 Cue에서 확장할지 HR10 착수 때 정한다 |
| 히트스톱·Cue 실행 위치 | 결정 필요 | 피해가 실제로 적용된 뒤 한 번 실행해야 한다. 처리기에서 적용 결과를 확인할지, 피해 처리(`PostGameplayEffectExecute`)에서 실행할지 HR9 착수 때 정한다 |
| 듀얼센스 연동 | 결정 필요 | 입력 장치 속성 효과가 [#44](https://github.com/jaykop/Kata/issues/44)의 서드파티 플러그인에서 동작하는지 확인한다 |

DS3의 같은 구조는 참고로만 쓴다. 공격 파라미터가 피격 등급, Poise 피해, 밀어내기 거리, 히트스톱 시간을 갖고, NPC 파라미터가 Poise 총량·회복 보정·붕괴 시 밀림 거리를 갖는다. 등급과 애니메이션 ID의 대응은 공개 자료로 확인하지 못했다.

## 작업 순서와 완료 조건

| ID | 우선순위 | 작업 | 선행 조건 | 완료 조건 |
|---|---|---|---|---|
| HR1 | 높음 | 상태·Impact·반응 이벤트·피격 Cue 태그를 프로젝트 Config에 추가하고 `UKataCombatSettings`에 SuperArmor 태그, Default Impact, 반응 이벤트 표, Cue 표, 방향 기준값, Poise·Groggy SetByCaller 태그를 추가한다 | 없음 | 설정 화면에서 값을 고를 수 있고 [게임플레이 태그 사용법](../manual/Gameplay-Tags.md)에 루트가 적혀 있다 |
| HR2 | 높음 | `UKataHitHandler_ApplyGameplayEffect`에 Impact 태그 필드를 추가하고, 접촉이 일어난 substep의 시작·끝을 HitResult에 기록한다 | HR1 | 피해 스펙에 Impact 태그가 있고 SocketTrace·ShapeSweep 히트 모두 이동 방향을 읽을 수 있다 |
| HR3 | 높음 | KataFramework에 Stance 세트와 Stance Execution을 추가한다. `GE_Damage`에 Stance Execution과 Invincible 요구 조건을 넣는다 | HR1 | 피격 시 Poise·Groggy가 줄고 정해진 규칙대로 회복·감소하며, 무적이면 모두 변하지 않는다 |
| HR4 | 높음 | Stance 세트가 판정 순서대로 반응을 정하고 반응 이벤트를 한 번 보낸다 | HR3 | 사망 시 이벤트가 없고, 같은 피격에 이벤트가 한 번만 간다 |
| HR5 | 높음 | 반응 GA 기반 클래스: 이벤트 Trigger, 방향 선택과 대체 규칙, Kata 재생·가산 몽타주 모드, 종료 처리 | HR2, HR4 | Kata 반응은 현재 액션을 끊고 재생되며, 가산 반응은 현재 액션을 유지한다. 반응이 끝나면 상태 태그가 사라진다 |
| HR6 | 높음 | 은기사 가산 009 계열의 원본 기준 자세와 공간 규칙을 찾아 UE Additive 에셋으로 변환한다 | 없음 | 변환한 가산 클립을 기본 자세에 더했을 때 원본과 같은 흔들림이 보인다 |
| HR7 | 높음 | 샘플: `Light` 4방향 반응 Kata와 GA 자식, `Flinch` 가산 몽타주와 GA 자식, 캐릭터 Gameplay Data의 Ability 부여 | HR5, HR6 | PC·AI 모두 약공격에 방향별로 경직되고, 하이퍼아머 공격 중에는 흔들림만 보인다 |
| HR8 | 보통 | `Heavy`·`Knock.Back`·`Knock.Down`(기상 무적 포함)·`Groggy` 반응과 재피격 차단 | HR7 사용자 확인 | 재피격 규칙대로 동작하고 다운·그로기 중 새 반응이 나지 않는다 |
| HR9 | 보통 | 히트스톱: Kata 실행기가 소유 액터의 Custom Time Dilation을 따르게 하고, 처리기의 공격별 시간으로 공격자·피격자를 잠깐 멈춘다 | 히트스톱 실행 위치 결정 | 맞은 순간 두 캐릭터만 멈추고, 몽타주와 Kata 타임라인이 어긋나지 않는다 |
| HR10 | 보통 | 피격 연출 Cue: Impact별 `GameplayCueNotify_Burst` 샘플과 실행 연결 | HR9 실행 위치 결정, 연출을 받는 플레이어 결정 | Impact에 따라 VFX·SFX·카메라 흔들림·패드 진동이 다르게 재생되고 무적이면 재생되지 않는다 |

## 영향과 제한

- 반응 관련 코드는 KataFramework에 둔다. Stance 세트는 Base·Combat 세트와 같은 Attribute 계층이고, 반응 GA는 Hit Trace와 Kata를 함께 쓰기 때문이다.
- 코어 `KataRuntime`은 HR9에서 실행기의 시간 계산만 바꾼다. 엔진 기능만 쓰는 변경이라 코어의 의존 규칙을 지킨다. 액터의 Custom Time Dilation을 쓰는 기존 콘텐츠가 있으면 Kata 타임라인 속도도 함께 바뀐다.
- 플러그인은 태그를 정의하지 않고 설정으로 받는다. 실제 태그는 프로젝트 `Config/Tags`에 둔다.
- `UKataHitHandler_ApplyGameplayEffect`에 직렬화 필드가 생긴다. 기본값이 비어 있으면 Impact 태그를 넣지 않으므로 기존 공격 에셋의 피해 동작은 바뀌지 않는다.
- HR2는 Hit Trace의 `TraceStart`·`TraceEnd` 의미를 바꾼다. SocketTrace와 ShapeSweep 시작 겹침 히트에서 두 값이 같다고 가정한 코드가 있으면 함께 확인한다.
- 반응 Kata가 현재 액션을 끊으면 PC 콤보 그래프는 끝나고 AI StateTree Task는 Failed를 반환한다. 반응 뒤 AI의 다음 행동은 StateTree 전이가 맡는다.
- 피격 경직·넉다운은 AI 이탈의 Cancel Window 원칙에서 예외인 강제 중단이다([AI 그래프 트리거 발신 계획](AI-Graph-Trigger-Plan.md)).
- `Event.Hit` 태그의 설명은 "Hit Trace 처리기가 보낸다"로 되어 있다. 반응 이벤트와 역할이 겹치지 않게 HR1에서 설명을 고치거나 정리한다.
- [게임플레이 태그 사용법](../manual/Gameplay-Tags.md)의 예시 `Combat.Hit.Light`는 상태 루트에 Combat 하위 계층을 두지 않는 규칙과 맞지 않는다. HR1에서 바꾼다.

## 사용자 확인 항목

- HR2~HR5, HR9는 사용자 빌드 확인이 필요하다.
- HR6은 변환한 가산 클립을 에디터 프리뷰에서 원본과 비교한다.
- HR7은 PC와 AI 각각에서 방향별 경직, 하이퍼아머 흔들림, 무적 회피 중 무피해를 실행으로 확인한다.
- HR8은 재피격 규칙과 기상 무적을, HR9는 히트스톱 중 타임라인 동기를, HR10은 패드 진동을 포함한 연출을 실행으로 확인한다.
- 이 목록만으로 에이전트가 빌드·테스트를 수행하지 않는다.

## 완료 시 갱신할 문서

- [#23](https://github.com/jaykop/Kata/issues/23): 단계별 구현·확인 상태. [#46](https://github.com/jaykop/Kata/issues/46): 실행 경로 결정 결과. [#7](https://github.com/jaykop/Kata/issues/7): 맞는 순간 연출을 이 계획으로 옮긴 범위.
- [Attribute 사용법](../manual/Attributes.md): Stance 세트와 Stance Execution.
- [Hit Trace 사용법](../manual/Hit-Trace.md): Impact 태그, 이동 방향, 히트스톱 시간.
- [게임플레이 태그 사용법](../manual/Gameplay-Tags.md): Status·Impact·반응 이벤트·Cue 루트와 부모 태그 판정 규칙.
- [런타임 사용법](../manual/Runtime-Usage.md): 실행기의 Custom Time Dilation 반영.
- 새 피격 반응 사용법 manual: 반응 GA 자식 만들기, 반응 Kata·가산 몽타주·연출 Cue 설정.
- 결정 이유를 담은 devlog, [액션 게임 기반 시스템 계획](Action-Game-Systems-Plan.md)과 [기본 태스크 확장 계획](Base-Task-Plan.md)의 해당 항목.
- [문서 목록](../README.md): 이 계획과 새 manual 링크.
