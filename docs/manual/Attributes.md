# Attribute 사용법

갱신: 2026-10-06  
대상: KataFramework의 AttributeSet, 버프·디버프 GE 작성, 피해 Execution  
적용 기준: [#40 캐릭터 스탯 Attribute](https://github.com/jaykop/Kata/issues/40), UE 5.8 GameplayAbilities  
확인 상태: 2026-10-05 사용자가 PIE에서 BlackKnight(AttackPower 10)가 StarvedHound(Defense 0, MaxHealth 60)를 공격해 타당 10 피해, 6타에 Health 0이 되는 것을 확인했다. 2026-10-06 회복 GE로 Stamina가 회복되는 것을 확인했다. 버프 GE, 프리뷰 셋업은 미확인

## 목적과 준비

캐릭터 스탯을 GAS Attribute로 정의하고, 캐릭터 데이터 테이블 행에서 에셋을 지정해 스탯을 조립한다.
버프·디버프는 Gameplay Effect로 넣고 빼며, 공격 피해는 `UKataDamageExecution`이 계산한다.

준비 조건은 다음과 같다.

- 캐릭터는 `AKataCharacter` 계열이어야 한다. ASC를 가지며, 컴포넌트 초기화 직후 행의 Gameplay Data를 적용한다.
- AttributeSet은 캐릭터 클래스에 고정되어 있지 않다. 행의 Gameplay Data가 ASC에 추가한다.
- 피해 GE를 쓰려면 Project Settings → Plugins → Kata Combat에서 Damage Set By Caller Tag를 지정해야 한다.

## 사용 순서

### 스탯 조립

[Gameplay Data 사용법](Gameplay-Data.md)에 따라 `Kata Gameplay Data`의 Attribute Sets와 Initial Values를 채우고 캐릭터 행에 지정한다.
일반 캐릭터는 `Kata Attribute Set: Base`와 `Kata Attribute Set: Combat`을 넣는다. 최대값(MaxHealth 등)만 넣으면 현재값은 가득 찬 상태로 시작한다.

### 피해 GE 작성

1. Instant GE를 만들고 Executions에 `Kata Damage Execution`을 추가한다.
2. 히트 판정 태스크의 `Apply Gameplay Effect` 처리기에 이 GE를 지정한다.
3. 처리기의 Set By Caller Magnitudes에 Damage Set By Caller Tag(샘플: `SetByCaller.Damage`)와 공격 계수를 넣는다. 약공격 1.0, 강공격 1.5처럼 공격마다 다른 값을 쓴다.

### 버프·디버프 GE 작성

1. 지속 시간이 있으면 Has Duration, 해제할 때까지 유지하면 Infinite GE를 만든다.
2. Modifiers에 대상 Attribute와 아래 규칙표의 ModOp를 지정한다.
3. 적용은 `KataTask_ApplyGameplayEffect`, 장비 행의 Granted Effects, Ability 등 GE를 적용하는 기존 경로를 쓴다. 해제는 GE 핸들이나 태그로 Remove한다.
4. 같은 버프의 중첩·갱신은 GE의 Stacking 설정으로 정한다.

## 주요 설정과 실행 규칙

### AttributeSet

| 세트 | Attribute | 제한 |
|---|---|---|
| `UKataAttributeSet_Base` | Health·MaxHealth, Stamina·MaxStamina·StaminaRegenRate, Mana·MaxMana·ManaRegenRate | 현재값은 0~Max, Max와 회복 속도는 0 이상 |
| `UKataAttributeSet_Base` (메타) | Damage, Healing | 들어오면 Health에서 빼거나 더한 뒤 0으로 되돌린다. Damage는 모디파이어 선택기에 표시하지 않는다 |
| `UKataAttributeSet_Combat` | AttackPower(공격력), Defense(방어력) | 0 이상 |

- 제한은 기본값과 버프가 반영된 최종값에 모두 적용된다.
- Max의 최종값이 바뀌면 현재값의 기본값을 같은 비율로 맞춘다. 예를 들어 Health 50/100에서 MaxHealth가 200이 되면 Health는 100이 된다.
- Health가 0보다 큰 상태에서 0이 되면 `UKataAttributeSet_Base::OnOutOfHealth`가 한 번 알린다. 사망 처리는 이 신호를 받는 쪽이 맡는다.
- 회복 속도는 값만 정의한다. 실제 회복은 이 값을 참조하는 Infinite Periodic GE가 수행한다. 이 GE는 Gameplay Data의 Granted Effects에 넣는다. 사용 직후나 가드 중 회복 정지는 GE의 Ongoing Tag Requirements로 처리한다.

### 버프·디버프 ModOp 규칙

모든 Attribute의 최종값은 GAS Aggregator의 식 하나로 계산된다. Override 모디파이어가 있으면 그 값이 우선한다.

`최종값 = ((Base + AddBase) × MultiplyAdditive ÷ DivideAdditive × MultiplyCompound) + AddFinal`

| 버프 종류 | ModOp | 계산 예 |
|---|---|---|
| 고정 수치 | Add (Base) | 공격력 +10 |
| 퍼센트 (합연산) | Multiply (Additive) | 1.1과 1.2를 함께 걸면 ×1.3 |
| 퍼센트 감소 | Multiply (Additive)에 1보다 작은 값 | 0.8이면 −20%. 다른 퍼센트 버프와 합산된다 |
| 독립 배율 (곱연산) | Multiply (Compound) | 1.1과 1.2를 함께 걸면 ×1.32 |
| 최종 가산 | Add (Final) | 배율 적용 뒤 +5 |
| 고정값 강제 | Override | 버프·디버프에는 쓰지 않는다 |

- Health·Stamina·Mana 같은 현재값에는 Duration·Infinite 모디파이어를 걸지 않는다. 증감은 Instant GE, 피해는 Damage, 회복은 Healing으로 한다. 최대값과 그 밖의 스탯에 버프를 건다.

### 피해 Execution

| 항목 | 내용 |
|---|---|
| 계산식 | `Raw = 공격 계수 × AttackPower`, `Final = Raw × K / (K + Defense)` |
| 공격 계수 | GE 스펙의 SetByCaller 값. 태그는 Kata Combat의 Damage Set By Caller Tag |
| K | Kata Combat의 Defense Constant(기본 100). Defense가 K와 같으면 피해가 절반이 된다 |
| 캡처 시점 | AttackPower는 스펙을 만들 때 스냅샷, Defense는 적용 시점 값. 둘 다 버프가 반영된 최종값 |
| 세트가 없을 때 | 공격한 쪽에 Combat 세트가 없으면 공격 계수를 그대로 Raw로 쓴다(함정 등). 맞은 쪽에 없으면 Defense를 0으로 본다 |
| 출력 | `UKataAttributeSet_Base`의 Damage에 더한다. 맞은 쪽에 Base 세트가 없으면 Health가 바뀌지 않는다 |

`Apply Gameplay Effect` 처리기와 `KataTask_ApplyGameplayEffect`의 Set By Caller Magnitudes는 태그가 비었거나 값이 유한하지 않으면 설정 오류로 표시된다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| 피해가 0이다 | Damage Set By Caller Tag가 비었거나 GE 스펙에 공격 계수가 없다 | Kata Combat 설정과 처리기의 Set By Caller Magnitudes를 확인한다. 로그에 경고가 남는다 |
| 초기값이 적용되지 않는다, 프리뷰에서 스탯이 없다 | Gameplay Data 구성 문제 | [Gameplay Data 사용법](Gameplay-Data.md)의 문제 해결을 따른다 |
| 강인도·그로기 스탯이 없다 | 이번 범위가 아니다 | [#23 피격 반응](https://github.com/jaykop/Kata/issues/23)에서 다룬다 |

## 확인 상태와 근거

2026-10-05 사용자가 PIE 피해 적용(Gameplay Data 행 적용, Damage Execution, 비율형 방어 식의 Defense 0 경우)을 확인했다. 방어력이 있는 대상, 버프·디버프 GE, 회복 GE, 프리뷰 셋업, Max 변경 시 비율 유지는 확인하지 않았다.

- [KataAttributeSet_Base.h](../../Plugins/KataFramework/Source/KataFramework/Public/Attributes/KataAttributeSet_Base.h): 자원 스탯과 메타 Attribute, `OnOutOfHealth`.
- [KataAttributeSet_Combat.h](../../Plugins/KataFramework/Source/KataFramework/Public/Attributes/KataAttributeSet_Combat.h): 공격·방어 스탯.
- [KataDamageExecution.h](../../Plugins/KataFramework/Source/KataFramework/Public/Attributes/KataDamageExecution.h): 피해 계산.
- [KataCombatSettings.h](../../Plugins/KataFramework/Source/KataFramework/Public/Attributes/KataCombatSettings.h): SetByCaller 태그와 K.
- [작업 상태](https://github.com/jaykop/Kata/issues/40).
