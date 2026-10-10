# 피격 반응 사용법

갱신: 2026-10-10  
대상: 피격 반응을 만드는 캐릭터·전투 설정 담당 / KataFramework `UKataHitReactionAbility`, `UKataHitReactionGameplayEffectComponent`  
적용 기준: [피격 반응 계획](../plan/Hit-Reaction-Plan.md), [#23](https://github.com/jaykop/Kata/issues/23)  
확인 상태: 반응 판정(`Kata Hit Reaction` 컴포넌트)은 2026-10-10 사용자 PIE 로그로 확인했다. 반응 Ability는 소스 기준이며 빌드·실행 확인 전이다

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

- HitResult의 `TraceStart`→`TraceEnd`(접촉한 서브스텝의 무기 이동)에서 수평 성분이 Kata Combat의 Min Horizontal Direction Ratio(기본 0.5) 이상이면 이동의 반대쪽을 피격 방향으로 본다. 왼쪽에서 오른쪽으로 베면 Left다.
- 수평 성분이 부족하거나(내려찍기) 이동이 없으면 공격자가 선 쪽으로 고른다. 공격자도 없으면 Front다.
- 맞은 쪽의 앞·오른쪽 방향과 비교해 더 가까운 축의 방향 하나를 고른다.

실행 규칙:

- Ability는 액터별 인스턴스이며, 같은 반응 이벤트가 다시 오면 진행 중인 반응을 끝내고 다시 시작한다.
- `Kata Action` 모드는 `UAbilityTask_PlayKataAction`으로 재생하며 공격자를 Kata의 대상으로 넘긴다. 반응 Kata는 현재 액션을 끊을 수 있도록 Blocking Policy의 Can Interrupt Active Kata를 켠다.
  반응 Kata가 끝나거나 끊기거나 시작이 거절되면 Ability가 끝나고 상태 태그가 사라진다.
- 반응 Kata가 현재 액션을 끊으면 PC 콤보 그래프는 끝나고 AI StateTree Task는 Failed를 반환한다.
- `Additive Montage` 모드는 엔진의 Play Montage And Wait로 재생하고 몽타주가 끝나면 Ability가 끝난다. 액션 슬롯을 쓰지 않으므로 현재 액션이 이어진다. 공격 몽타주와 다른 Slot Group을 쓴다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 판정 로그는 나오는데 반응이 없다 | 이벤트 태그에 맞는 반응 Ability가 부여되지 않았거나 Activation Blocked Tags에 걸렸다 | Gameplay Data의 Granted Abilities와 Trigger Tag를 확인한다 |
| 반응 Kata가 재생되지 않고 바로 끝난다 | 현재 액션의 차단 정책에 걸려 시작이 거절됐다 | 반응 Kata의 Can Interrupt Active Kata와 현재 액션의 Blocked Kata Tags를 확인한다. Verbose 로그에 거절 결과가 남는다 |
| 가산 반응 중 공격이 끊긴다 | 가산 몽타주가 공격 몽타주와 같은 Slot Group에 있다 | 가산 몽타주의 Slot을 다른 Group으로 옮긴다 |
| 샘플 반응이 없다 | 샘플 반응 Kata·가산 몽타주와 Ability 자식은 아직 없다 | [#23](https://github.com/jaykop/Kata/issues/23)의 다음 단계에서 추가한다 |

## 확인 상태와 근거

반응 판정은 2026-10-10 사용자가 보스 흑기사를 상대로 한 PIE 로그로 Flinch, Light와 Poise 채움, Groggy, 사망 후 판정 없음을 확인했다.
반응 Ability와 방향 선택은 소스 기준이며 빌드·실행 확인 전이다.

- [KataHitReactionAbility.h](../../Plugins/KataFramework/Source/KataFramework/Public/HitReaction/KataHitReactionAbility.h): 반응 Ability와 방향 선택.
- [KataHitReactionEffectComponent.h](../../Plugins/KataFramework/Source/KataFramework/Public/Attributes/KataHitReactionEffectComponent.h): 반응 판정과 반응 이벤트.
- [작업 상태](https://github.com/jaykop/Kata/issues/23).
