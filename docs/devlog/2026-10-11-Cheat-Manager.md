# 치트 매니저 확장

작성: 2026-10-11  
갱신: 2026-10-11  
유형: 구현 기록, 결정 기록, 진단  
대상: KataFramework·KataAI 치트 확장, KataTargeting 대상 선택자, Kata Combat 설정 태그  
기준: main 9873497(치트 구현), 406fcf9(액터 단위 API), 5d44cb2(Kata Debug 탭). [#54](https://github.com/jaykop/Kata/issues/54) 종료 시점의 계획 문서 내용을 옮겼다.

## 배경과 결론

전투·AI를 확인할 때 PC 생존 제어, 대상 즉시 사망·그로기, AI 정지·인지 제외를 바로 쓰고 싶다는 요청이 있었다.
엔진 `UCheatManager`의 `God`은 `bCanBeDamaged`만 바꾸므로 GE로 들어오는 Kata 피해를 막지 못한다. `DestroyTarget`도 사망 흐름을 거치지 않는다.
그래서 기능을 구현한 플러그인마다 `UCheatManagerExtension`을 두고 콘솔 명령을 제공했다. 같은 함수를 [Kata Debug 탭](../manual/Debug-Tab.md)의 치트 패널도 부른다([#55](https://github.com/jaykop/Kata/issues/55)).

## 변경 또는 진단 내용

| 치트 | 둘 곳 | 연결 지점 |
|---|---|---|
| `KataKill` | KataFramework | `UKataDeathComponent::Kill`. Health 0과 같은 사망 흐름 |
| `KataGod` | KataFramework | `UKataHitSubsystem::DispatchPendingHits`에서 치트 무적 대상의 히트 처리기를 모두 건너뜀 |
| `KataNoDamage` | KataFramework | `UKataAttributeSet_Base::PostGameplayEffectExecute`에서 Health 반영만 막음. GE는 실행되어 피격 반응이 남음 |
| `KataInfiniteStamina` | KataFramework | PC ASC의 Stamina 변경 델리게이트에서 Max Stamina로 되돌림 |
| `KataRespawn` | KataFramework | `AKataGameMode::RestartPlayerNow`. 살아 있으면 먼저 사망 처리하고 예약된 재시작을 취소한 뒤 바로 시작 |
| `KataGroggy` | KataFramework | Groggy를 Max Groggy로 올리고 Kata Combat의 Groggy Event Tag 이벤트 전송 |
| `KataHideFromAI` | KataAI | `UKataAITargetingComponent`가 시각 결과를 반영하기 전에 숨긴 액터를 거르고, 켤 때 각 AI의 시야 기록과 기억 위치를 지움 |
| `KataPauseAI` | KataAI | AIController Brain의 `PauseLogic`·`ResumeLogic`과 이동 정지 |
| 대상 선택자·토글 인자 | KataTargeting | `KataFL::ResolveCheatTargets`(Self·Target·Nearest·All), `KataFL::ParseCheatToggle` |

### 진단: 엔진 자극원 해제로는 AI 인지 제외가 적용되지 않음

처음에는 `UAIPerceptionSystem::UnregisterSource`로 PC를 시각 자극원에서 뺐다. 사용자 확인에서 이미 PC를 인지한 적과 치트를 켠 뒤 스폰된 적에게 적용되지 않았다.
엔진 해제는 인지 기억을 지우지 않는다. 또 시각 감각이 처음 만들어질 때 모든 Pawn이 다시 등록될 수 있다(뒤의 원인은 실행으로 확인하지 않았다).
그래서 인지 결과를 받는 KataAI 타게팅에서 거르는 방식으로 바꿨고, 사용자가 수정본을 확인했다.

## 주요 결정과 이유

- **로직은 기능별 플러그인의 확장에 둔다.** 모듈 시작 시 `RegisterForOnCheatManagerCreated`로 등록하므로 CheatManager 클래스를 지정할 필요가 없다. 치트 코드는 `UE_WITH_CHEAT_MANAGER`로 Shipping에서 뺀다.
- **대상 선택자는 KataTargeting에 둔다.** KataAI는 KataFramework에 의존할 수 없다(위성 → 통합 역참조 금지). 그래서 두 플러그인이 모두 의존하는 KataTargeting에 둔다.
- **게임플레이용과 치트용 태그를 분리한다(사용자 결정).**
  - 게임플레이 무적은 기존 `Status.Invincible` GE 적용 조건을 그대로 쓴다.
  - Kata Combat 설정에는 `Ignore Damage Tag`(게임플레이), `Cheat Invincible Tag`, `Cheat Ignore Damage Tag`를 추가했다. 플러그인은 태그를 정의하지 않고, 샘플이 `Status.IgnoreDamage`·`Cheat.*`를 정의한다.
  - 치트 태그 검사만 Shipping에서 뺀다.
  - 치트 무적은 히트 처리기 앞에서 끊는다. 그래서 무적 조건이 없는 GE와 이벤트 전송 처리기도 막는다.
- **PC 토글 상태는 PlayerController별 확장 인스턴스가 기억한다.** 사망 후 재생성되면 새 Pawn에 다시 건다. 효과는 Loose Tag나 델리게이트 구독으로 걸어 공유 에셋에 실행 상태를 두지 않는다.
- **쿨다운 무시는 범위에서 뺐다(사용자 결정).** Kata 쿨다운은 `KataGas::IsOnCooldown`의 자체 판정이라 엔진의 `AbilitySystem.IgnoreCooldowns`가 듣지 않는다는 점도 확인했다.
- **부활 대신 GameMode 재생성을 쓴다(사용자 결정).** 사망 컴포넌트에 부활 API를 넣으려면 사망 태그·Hurt Box·이동·AI를 모두 되돌려야 해서 범위가 크다.
- **디버그 UI용 액터 단위 공개 함수를 추가했다.** `KillActor`, `MakeActorGroggy`, `Set*Enabled`, `RespawnPlayer`, `SetHiddenFromAI`, `SetAIPaused`, `IsAIPaused`이다. 콘솔 명령도 이 함수를 거친다.

## 근거

- [KataFrameworkCheatExtension.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Cheats/KataFrameworkCheatExtension.cpp)
- [KataAICheatExtension.cpp](../../Plugins/KataAI/Source/KataAI/Private/Cheats/KataAICheatExtension.cpp), [KataAICheatState.cpp](../../Plugins/KataAI/Source/KataAI/Private/Cheats/KataAICheatState.cpp)
- [KataFL_CheatTarget.cpp](../../Plugins/KataTargeting/Source/KataTargeting/Private/Cheats/KataFL_CheatTarget.cpp)
- [KataCombatSettings.h](../../Plugins/KataFramework/Source/KataFramework/Public/Attributes/KataCombatSettings.h), [KataGameMode.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Player/KataGameMode.cpp)
- [당시 계획 문서(삭제됨)](https://github.com/jaykop/Kata/blob/5d44cb2/docs/plan/Cheat-Manager-Plan.md)

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 그로기 외 콘솔 명령 | 사용자 PIE 확인(2026-10-11) | 통과 | - |
| `KataHideFromAI` 수정본, `KataRespawn` | 사용자 PIE 확인(2026-10-11) | 통과 | - |
| `KataGroggy` | 사용자 확인 보고(2026-10-11, 피격 반응 작업 후) | 통과 | 반응 방향의 상세 결과 미보고 |
| Kata Debug 탭 치트 패널 | 사용자 PIE 확인(2026-10-11) | 통과 | - |

에이전트는 빌드·테스트·실행을 하지 않았다.

## 남은 제한과 후속 작업

- 무한 마나, 대미지·Poise 배율, 적 무적, 장비 교체, 태그·GE 부여는 첫 범위에서 뺐다. 필요하면 새 이슈로 다룬다.
- `KataHideFromAI`는 KataAI 타게팅에서만 거른다. 엔진 Perception을 직접 읽는 로직이 생기면 적용되지 않는다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#54](https://github.com/jaykop/Kata/issues/54) | 결과 댓글 후 종료 |
| [치트 사용법](../manual/Cheats.md) | 그로기 확인 반영, 적용 기준을 이 기록으로 변경 |
| [Gameplay Tags](../manual/Gameplay-Tags.md), [Attributes](../manual/Attributes.md) | 구현 커밋에서 반영 |
| 치트 매니저 계획 | 이슈 종료와 함께 삭제 |
