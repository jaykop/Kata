# 치트 사용법

갱신: 2026-10-11  
대상: KataFramework·KataAI 치트 확장, KataTargeting 대상 선택자  
적용 기준: [#54](https://github.com/jaykop/Kata/issues/54), [결정 기록](../devlog/2026-10-11-Cheat-Manager.md)  
확인 상태: 2026-10-11 사용자 PIE 확인(모든 명령)

## 목적과 준비

전투·AI를 확인할 때 콘솔 명령으로 PC 생존 상태를 바꾸고, 적을 바로 사망·그로기 상태로 만들고, AI를 멈춘다.

- 치트 명령은 KataFramework와 KataAI 모듈이 시작될 때 모든 CheatManager에 자동으로 붙는다. 별도 CheatManager 클래스를 지정할 필요가 없다.
- PIE와 Standalone Game에서 쓸 수 있다. PIE 중에는 [Kata Debug 탭](Debug-Tab.md)의 치트 패널에서도 같은 기능을 쓴다. CheatManager가 만들어지지 않는 Shipping 구성에서는 쓸 수 없다.
- 무적·대미지 무시는 Kata Combat 설정(프로젝트 설정 > Plugins > Kata Combat)의 Cheat 항목에 태그가 있어야 동작한다. 샘플은 `Cheat.Invincible`·`Cheat.IgnoreDamage`를 지정해 두었다.
- 재생성은 게임 모드가 `AKataGameMode`(또는 파생)여야 한다.

## 사용 순서

1. PIE 중 `` ` `` 키로 콘솔을 연다.
2. 아래 명령을 입력한다. 결과는 콘솔에 한 줄로 나오고 로그(`LogKataFramework`·`LogKataAI`)에도 남는다.
3. 토글 명령은 같은 명령을 다시 입력하면 꺼진다.

```
KataGod             // PC 무적 켜기·끄기
KataKill Self       // PC 사망
KataKill            // 현재 타게팅 대상 사망
KataGroggy Nearest  // 가장 가까운 적을 그로기 상태로
KataPauseAI All 1   // 모든 AI 정지
```

## 명령

| 명령 | 인자 | 동작 |
|---|---|---|
| `KataKill [대상]` | 대상, 기본 `Target` | 사망 컴포넌트의 Kill로 Health 0과 같은 사망 흐름을 탄다 |
| `KataGroggy [대상]` | 대상, 기본 `Target` | Groggy를 Max Groggy로 채우고 Kata Combat의 Groggy Event Tag 이벤트를 보낸다. Stance 세트가 없거나 Max Groggy가 0이면 건너뛴다 |
| `KataGod [0\|1]` | 토글 | PC 무적. Kata 히트 판정이 PC의 히트 처리기를 하나도 실행하지 않아 피해와 피격 반응이 모두 없다 |
| `KataNoDamage [0\|1]` | 토글 | PC 대미지 무시. 피해 GE는 적용되어 피격 반응, Poise·Groggy 누적, 연출은 그대로이고 Health만 줄지 않는다 |
| `KataInfiniteStamina [0\|1]` | 토글 | Stamina가 줄 때마다 Max Stamina로 되돌린다 |
| `KataRespawn` | 없음 | PC를 지연 없이 Player Start에서 새 캐릭터로 시작한다. 살아 있으면 먼저 사망 처리하며 시체는 일반 사망과 같이 정리된다 |
| `KataHideFromAI [0\|1]` | 토글 | KataAI 타게팅이 PC를 보지 못하게 한다. 이미 인지한 적은 시야 기록과 마지막 위치 기억을 바로 잃고, 이후 스폰된 적도 PC를 대상으로 삼지 않는다. 끄면 각 AI의 다음 인지 갱신에서 다시 보인다 |
| `KataPauseAI [대상] [0\|1]` | 대상, 기본 `All` / 토글 | AI의 StateTree를 일시정지하고 이동을 멈춘다. 상태를 비우면 하나라도 실행 중일 때 모두 정지, 모두 정지해 있으면 모두 재개 |

토글 인자는 비우면 반전하고, `1`·`true`·`on`과 `0`·`false`·`off`는 그 값으로 정한다.

### 대상 선택자

| 이름 | 대상 |
|---|---|
| `Self` | 플레이어가 조종하는 Pawn |
| `Target` | 플레이어 Pawn의 타게팅 컴포넌트가 지금 고른 대상(락온 대상 또는 소프트 타겟) |
| `Nearest` | 플레이어 Pawn에서 가장 가까운, ASC를 가진 다른 Pawn |
| `All` | 플레이어 Pawn을 뺀, ASC를 가진 모든 Pawn |

이름은 대소문자를 가리지 않는다. KataFramework 명령은 `Nearest`·`All`에서 Dead Status Tag를 가진 시체를 뺀다. KataAI 명령은 시체를 빼지 않는다.

### PC 토글의 유지

무적, 대미지 무시, 무한 스태미나, AI 인지 제외는 PlayerController마다 켜짐 상태를 기억한다. 사망 뒤 새 캐릭터가 생기면 새 캐릭터에 다시 걸고, 시체에서는 치트 태그를 걷어 낸다.

### 게임플레이 태그와의 구분

| 용도 | 태그 | 막는 위치 |
|---|---|---|
| 게임플레이 무적 | `Status.Invincible` | 피해 GE의 Target Tag Requirements([Attributes](Attributes.md)) |
| 게임플레이 대미지 무시 | Ignore Damage Tag(샘플 `Status.IgnoreDamage`) | AttributeSet의 Health 반영 |
| 치트 무적 | Cheat Invincible Tag(샘플 `Cheat.Invincible`) | Kata 히트 처리기 실행 전 |
| 치트 대미지 무시 | Cheat Ignore Damage Tag(샘플 `Cheat.IgnoreDamage`) | AttributeSet의 Health 반영 |

치트 무적은 피해 GE 설정에 기대지 않으므로 무적 조건이 없는 GE와 이벤트 전송 처리기도 막는다. 치트 태그는 Shipping에서 검사하지 않는다.

## 제한과 문제 해결

| 증상 또는 제한 | 원인·조건 | 사용자가 할 일 |
|---|---|---|
| `KataGod`·`KataNoDamage`를 켜도 피해를 받는다 | Kata Combat의 Cheat 태그가 비어 있다. 명령이 경고를 남긴다 | 설정에 `Cheat.Invincible`·`Cheat.IgnoreDamage`를 지정한다 |
| 치트 무적인데 피해를 받는다 | Kata 히트 판정을 거치지 않고 GE를 직접 적용하는 경로다 | `KataNoDamage`를 함께 켠다 |
| 무적을 켠 직후 같은 공격에 다시 맞지 않는다 | 한 번의 휘두름에서 이미 히트로 기록된 대상이다 | 정상 동작이다 |
| `KataHideFromAI`를 켜도 적이 반응한다 | KataAI 타게팅을 거치지 않고 엔진 Perception을 직접 읽는 로직이다. 엔진 Perception 자체는 그대로 동작한다 | 그 로직이 KataAI 타게팅의 대상을 쓰도록 바꾼다 |
| `KataPauseAI` 뒤에도 적의 공격이 끝까지 나간다 | 이미 재생 중인 Kata 액션은 멈추지 않는다 | 액션이 끝난 뒤 정지 상태를 확인한다 |
| `KataGroggy` 반응 방향이 실제 위치와 다를 수 있다 | 이벤트에 HitResult가 없어 반응 방향을 공격자 위치로만 고른다 | 방향이 중요하면 실제 공격으로 그로기를 만든다 |
| `KataRespawn`이 아무것도 하지 않는다 | 게임 모드가 Kata Game Mode가 아니거나 Pawn에 사망 컴포넌트가 없다 | 게임 모드를 확인한다 |

## 확인 상태와 근거

2026-10-11 사용자가 PIE에서 그로기를 뺀 명령의 동작을 확인했다. 이때 `KataHideFromAI`는 이미 인지한 적과 이후 스폰된 적에 적용되지 않아, 인지 결과를 KataAI 타게팅에서 거르는 방식으로 고쳤다. 같은 날 수정본도 사용자가 확인했다. 그로기는 피격 반응 작업 뒤 같은 날 사용자가 확인했다.

- [KataFrameworkCheatExtension.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Cheats/KataFrameworkCheatExtension.cpp): PC 토글, 사망·그로기·재생성.
- [KataAICheatExtension.cpp](../../Plugins/KataAI/Source/KataAI/Private/Cheats/KataAICheatExtension.cpp): 인지 제외, AI 정지.
- [KataAITargetingComponent.cpp](../../Plugins/KataAI/Source/KataAI/Private/Targeting/KataAITargetingComponent.cpp): 숨긴 대상을 시각 결과에서 거르는 위치.
- [KataFL_CheatTarget.cpp](../../Plugins/KataTargeting/Source/KataTargeting/Private/FunctionLibraries/KataFL_CheatTarget.cpp): 대상 선택자, 토글 인자.
- [KataHitSubsystem.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/HitTrace/KataHitSubsystem.cpp): 치트 무적 판정.
- [KataAttributeSet_Base.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Attributes/KataAttributeSet_Base.cpp): 대미지 무시 판정.
- [작업 상태 #54](https://github.com/jaykop/Kata/issues/54).
