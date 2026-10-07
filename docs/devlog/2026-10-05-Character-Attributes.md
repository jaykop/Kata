# 캐릭터 스탯 Attribute와 Gameplay Data

작성: 2026-10-05  
갱신: 2026-10-07  
유형: 구현 기록, 결정 기록  
대상: KataFramework `Attributes/`, `Character/KataGameplayData`, 캐릭터 행, 히트 처리기·GE 태스크의 SetByCaller, HurtBox 태그 루트  
기준: #40 작업 트리(미커밋 상태에서 작성)

## 배경과 결론

[#40](https://github.com/jaykop/Kata/issues/40)에서 캐릭터 스탯을 GAS Attribute로 정의했다. 대미지 식의 정의 방식과 초기화·데이터 관리 위치를 함께 결정했다.
기능 카테고리별 AttributeSet(Base·Combat), 캐릭터 행에 배열로 지정하는 `UKataGameplayData`, Execution 기반 피해 계산을 구현했다.
샘플 에셋을 구성한 뒤 사용자가 PIE에서 피해 적용을 확인했다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| AttributeSet | 없음. `UKataCondition_Attribute`만 임의 Attribute를 읽었다 | `UKataAttributeSet_Base`(Health·Stamina·Mana, Max, 회복 속도, 메타 Damage·Healing), `UKataAttributeSet_Combat`(AttackPower·Defense) |
| 스탯 데이터 | 없음 | `UKataGameplayData`(세트 목록 + FScalableFloat 초기값), `FKataCharacterRow::GameplayData` 배열, `UKataPreviewSetup_GameplayData` |
| 피해 계산 | 히트 처리기가 지정 GE를 그대로 적용 | `UKataDamageExecution`: `공격 계수 × AttackPower × K / (K + Defense)`를 메타 Damage로 출력 |
| SetByCaller | 값을 넣을 방법이 없음 | Apply Gameplay Effect 히트 처리기와 `KataTask_ApplyGameplayEffect`에 `SetByCallerMagnitudes`, `UKataCombatSettings` |
| 태그 루트 | HurtBox 태그 선택기가 전체 태그를 보여 줌 | `SetByCaller`, `HurtBox` 루트 추가. 샘플 `SetByCaller.Damage`, `HurtBox.WeakPoint`·`HurtBox.Disabled` |
| 샘플 콘텐츠 | 히트에 Send Gameplay Event만 연결 | `GE_Damage`·`GE_StaminaRegen`, BlackKnight·StarvedHound Gameplay Data와 행 연결, 공격 액션 HitTrace의 처리기를 피해 GE로 교체(사용자 요청으로 Send Gameplay Event 제거) |

## 주요 결정과 이유

사용자 확정(2026-10-05, #40 대화와 [결정 댓글](https://github.com/jaykop/Kata/issues/40#issuecomment-5994699180)):

- **세트 분할 기준은 기능 카테고리다.** PC/NPC 같은 역할로 나누는 안도 검토했다. 하지만 행 단위 조립에서는 세트가 "같이 있거나 없는 묶음"이면 충분하고, 역할 분할은 교차 사용에 약해 채택하지 않았다.
  근접·원거리 세트도 만들지 않는다. 대미지 종류는 GE 태그와 EC 분기로 다룬다. Poise·Groggy는 #23으로 넘겼다. Stagger는 Attribute가 아니라 결과 상태로 본다.
- **데이터는 행에 에셋을 할당해 조립한다.** 클래스 기본 서브오브젝트로 고정하지 않는다. 행 슬롯은 배열이다.
  공용 능력 구성과 캐릭터별 스탯을 나눠 재사용하기 위해서다. Ability·Effect·Tag 섹션은 같은 에셋에 [#34](https://github.com/jaykop/Kata/issues/34)가 추가한다.
- **대미지는 Execution Calculation으로 계산한다.** MMC는 모디파이어 하나의 크기만 계산하므로 다중 캡처·다중 출력·분기가 필요한 피해 계산에 맞지 않는다. Damage는 메타 Attribute로 받아 Health로 옮긴다.
  Damage는 모디파이어 선택기에서 숨겨 방어 식을 거치지 않는 피해 경로를 막았다.
- **방어 식은 비율형이다.** 감산형은 약한 공격이 0이 되고 수치 규모를 맞추기 어렵다. 비율형은 상수 K 하나로 곡선을 조절한다.
- **버프·디버프 연산식은 GAS Aggregator 하나만 쓴다.** UE 5.8 `FAggregatorModChannel::EvaluateWithBase`의 식을 그대로 쓰고, ModOp 사용 규칙만 [Attribute 사용법](../manual/Attributes.md)에 정했다.
- **초기값은 FScalableFloat다.** 커브를 비우면 float와 같다. 나중에 float에서 바꾸면 타입 변경으로 저장된 에셋을 이관해야 하므로 처음부터 이 타입을 쓴다.
- **회복 속도는 Attribute다.** 회복은 이 값을 참조하는 Periodic GE가 수행해 버프가 같은 식을 타게 한다.

구현 세부:

- 초기값은 모든 값을 두 번 넣는다. 어떤 Attribute가 최대값인지 일반적으로 알 수 없어서, 현재값이 이전 Max로 잘리는 순서 문제를 두 번째 설정으로 해소한다.
- Max 최종값이 바뀌면 현재값의 기본값을 같은 비율로 맞춘다. 이전 Max가 0이면 새 Max로 채우므로 Max만 지정해도 가득 찬 상태로 시작한다.
- 세트 적용은 `AKataCharacter::PostInitializeComponents`에서 ASC Actor Info를 초기화한 직후에 한다. 행은 그보다 먼저 OnConstruction에서 적용되므로 에셋을 보관해 두었다가 적용한다.

## 근거

- [KataAttributeSet_Base.h](../../Plugins/KataFramework/Source/KataFramework/Public/Attributes/KataAttributeSet_Base.h): 자원 스탯, 메타 Attribute, `OnOutOfHealth`.
- [KataDamageExecution.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Attributes/KataDamageExecution.cpp): 캡처와 피해 식.
- [KataGameplayData.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataGameplayData.cpp): 세트 추가, 초기값 병합과 두 번 설정.
- [KataCharacter.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Character/KataCharacter.cpp): 행 보관과 적용 시점.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| Editor 빌드 | 2026-10-05 사용자 빌드 보고 | 통과(HurtBox 메타 변경 포함) | Game 타깃 빌드 |
| 피해 적용 | 2026-10-05 사용자 PIE | BlackKnight(AttackPower 10)가 StarvedHound(Defense 0, MaxHealth 60)를 타당 10씩 6타에 Health 0으로 만듦 | Combat 세트가 없는 공격자 |
| 샘플 에셋 구성 | MCP로 설정 후 값 재조회 | 설정값 일치, 저장 완료 | — |
| 회복 GE | 2026-10-06 사용자 PIE([Gameplay Data 확인 기록](2026-10-06-Gameplay-Data-Grants.md)) | `GE_StaminaRegen`으로 Stamina 회복 | — |
| 버프·디버프 GE, Defense가 있는 대상, Max 비율 유지 | 2026-10-07 사용자 PIE, 테스트 GE 4개(`GE_Test_AttackBuff`·`DefenseBuff`·`MaxHealthBuff`·`HealthDrain`) | 기대 결과와 일치([Attribute 사용법](../manual/Attributes.md#확인-상태와-근거)) | — |
| HurtBox 선택기 UI | 2026-10-07 사용자 에디터 확인 | `HurtBox` 루트만 표시 | — |
| 프리뷰 셋업 | 제외 | — | GE·Ability를 프리뷰 월드에서 확인할 필요가 없다는 사용자 판단 |

## 남은 제한과 후속 작업

- `GE_StaminaRegen`은 #34에서 Gameplay Data의 Granted Effects로 연결했다. 회복 정지 태그는 아직 정하지 않았다.
- Health 0 이후의 사망 처리는 #41에서 `OnOutOfHealth`에 연결한다.
- Poise·Groggy 세트는 #23에서 다룬다.
- 초기값 레벨은 1로 고정이다. 레벨을 정하는 곳은 후속 결정이다.
- `Event.Hit` 태그는 샘플 액션에서 사용처가 없어졌지만 정의는 유지했다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#40](https://github.com/jaykop/Kata/issues/40) | 결정 댓글 게시. 결과 댓글·라벨은 사용자 확인 후 게시 |
| [Attribute 사용법](../manual/Attributes.md) | 신규 |
| [캐릭터 데이터 테이블 사용법](../manual/Character-Data.md) | Gameplay Data 슬롯 추가 |
| [게임플레이 태그 사용법](../manual/Gameplay-Tags.md) | `HurtBox`·`SetByCaller` 루트 추가 |
| [액션 비용 정책 계획](../plan/Cost-Policy-Plan.md) | 영향 없음. 비용 GE가 이번 Attribute를 대상으로 쓸 수 있다 |
