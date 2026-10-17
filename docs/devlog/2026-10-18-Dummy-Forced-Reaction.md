# 더미 반응 고정과 Die On Hit

작성: 2026-10-18  
갱신: 2026-10-18  
유형: 구현 기록  
대상: KataFramework 피격 반응 판정(`UKataHitReactionGameplayEffectComponent`), 새 인터페이스 `IKataHitReactionOverride`, 스포너 더미(`FKataDummySettings`, `UKataDummyComponent`), Base AttributeSet 피해 처리, Kata Combat 설정  
기준: 커밋 `fec4d3d` 이후 작업 트리의 #133 변경. 같은 트리에 있는 다른 작업의 미커밋 변경(`Config/DefaultEditor.ini`, `docs/README.md`의 기획 문서 줄)은 대상에서 제외했다

## 배경과 결론

샘플 확인 레벨의 피격 반응 구역을 반응 종류별 섹션(Flinch·Light·Medium·Heavy·Knock.Back·Knock.Down·Groggy·사망)으로 나누려 했다. 섹션마다 더미 하나가 맞으면 그 반응만 방향별로 내야 한다.
진단해 보니 반응 종류는 판정 컴포넌트가 Poise·Groggy와 공격의 Impact 등급만으로 정해, 맞는 쪽이 바꿀 길이 없었다. 맞는 쪽 기준으로 한 번에 죽이는 경로도 없었다.
더미 설정에 고정 반응(Forced Reaction Event Tag)과 보호 값 Die On Hit을 추가해 두 문제를 풀었다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 또는 발견 내용 | 반영 결과 또는 필요한 조치 |
|---|---|---|
| 반응 판정 | Stance 세트·Health 확인 뒤 Groggy → Super Armor → Poise 순서로만 정했다 | Stance·Health 확인 바로 뒤에 맞은 쪽의 `IKataHitReactionOverride`에 묻는다. 태그가 오면 판정 없이 그 이벤트를 보내고 Poise를 채우지 않는다. Verbose 로그 이유는 `forced` |
| 고정 반응 인터페이스 | 없음 | `IKataHitReactionOverride::GetForcedHitReactionEventTag(const FGameplayEffectSpec&)`. 판정 컴포넌트가 Avatar 액터 자신을 먼저 보고, 없으면 첫 구현 컴포넌트를 쓴다 |
| 더미 설정 | 보호(None·Invincible·Ignore Damage), 재생성, 반복, 피격 정보 | `Forced Reaction Event Tag`(`Event` 루트)를 추가하고 `UKataDummyComponent`가 인터페이스를 구현한다. 보호 enum 끝에 `Die On Hit`을 추가했다 |
| 맞는 쪽 한 방 사망 | `KataOneHit`은 공격자 쪽 치트라 모든 대상에 걸렸다. `UKataDeathComponent::Kill`은 HitResult를 남기지 않아 방향별 사망을 고르지 못한다 | Kata Combat에 `Die On Hit Tag`를 두고, Base AttributeSet이 한 방 치트와 같은 자리에서 맞은 쪽이 이 태그를 가지면 양수 피해를 남은 Health 이상으로 올린다 |
| 샘플 설정 | — | `Status.DieOnHit` 태그(`Config/Tags/Status.ini`)를 정의하고 `DefaultGame.ini`의 Die On Hit Tag에 지정했다 |
| 런타임 스포너 패널 | 보호 이름 목록 3개 고정, 저장값이 Ignore Damage를 넘으면 None으로 되돌린다 | 코드를 바꾸지 않았다. Die On Hit과 고정 반응은 패널에 나오지 않는다(이슈 범위 제외) |

## 주요 결정과 이유

- **판정 컴포넌트와 더미를 인터페이스로 잇는다.** 사용자가 직접 조회 방식 대신 인터페이스 방식을 골랐다. 피격 판정 코드가 디버그용 더미를 직접 알지 않게 하고, 다른 기능(보스 패턴 등)도 같은 자리를 쓸 수 있게 하기 위해서다. 스펙을 인자로 넘겨 나중에 공격별로 고를 여지를 남겼다.
- **Stance 세트 없음·Health 0 확인은 고정 반응보다 앞에 둔다.** 경직하지 않는 대상과 죽은 대상에는 반응을 보내지 않는 기존 규칙을 그대로 지킨다.
- **고정 반응이면 Poise를 채우지 않는다.** 판정을 건너뛰었으므로 Poise 상태를 바꿀 이유가 없다. 피해 GE는 이미 실행되어 Poise·Groggy 누적은 그대로 반영된다.
- **사망은 `Kill`이 아니라 정상 피해 경로로 일으킨다.** 사망 컴포넌트가 마지막 피해 GE의 HitResult를 받아야 사망 Ability가 방향별 사망 Kata를 고를 수 있다.
- **Die On Hit은 양수 피해에서만 죽인다.** 반응 전용 0 피해 GE와 피해 없는 반응 이벤트로는 죽이지 않는 한 방 치트의 규칙과 맞췄다. Ignore Damage Tag가 함께 있으면 Health는 줄지 않는다.
- **Die On Hit 태그는 Shipping에서도 검사한다.** 치트가 아니라 Ignore Damage Tag와 같은 게임플레이 설정이기 때문이다.
- **새 enum 값은 끝에 추가했다.** 저장된 보호 값(0~2)과 런타임 스포너 패널의 인덱스 매핑을 바꾸지 않는다.

## 근거

- [KataHitReactionOverride.h](../../Plugins/KataFramework/Source/KataFramework/Public/HitReaction/KataHitReactionOverride.h): 인터페이스와 반환 계약.
- [KataHitReactionEffectComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Attributes/KataHitReactionEffectComponent.cpp): `FindForcedReactionEventTag`와 판정 순서.
- [KataAttributeSet_Base.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Attributes/KataAttributeSet_Base.cpp): `IsDieOnHitTarget`과 피해 상향 조건.
- [KataDummyComponent.h](../../Plugins/KataFramework/Source/KataFramework/Public/Spawning/KataDummyComponent.h): `ForcedReactionEventTag`, `EKataDummyProtection::DieOnHit`.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| Editor 빌드 | 사용자 빌드 보고(2026-10-18) | 통과 | Game 타깃 빌드 |
| 고정 반응·Die On Hit | 사용자 PIE(2026-10-18), 샘플 확인 레벨의 피격 반응 구역(BlackKnight 더미 8개: 고정 반응 7, Die On Hit 1) | 통과 보고 | 항목별 결과(방향별 반응, 다운·그로기 중 재반응 없음, 방향별 사망·재생성, 회귀 항목)는 따로 보고되지 않았다 |
| 설정 확인 | 에이전트가 unreal-mcp로 배치한 스포너의 설정값을 다시 읽음 | 8개 모두 지정한 보호·고정 반응·재생성 값 | — |

## 남은 제한과 후속 작업

- 런타임 스포너 패널에서는 Die On Hit과 고정 반응을 고를 수 없다. 필요해지면 후속 이슈로 다룬다.
- 고정 반응도 그 이벤트로 활성화되는 반응 Ability가 없거나 Activation Blocked Tags에 걸리면 재생되지 않는다. 이는 기존 반응 규칙과 같다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [#133](https://github.com/jaykop/Kata/issues/133) | 구현·사용자 확인 완료. 결과 기록과 닫기는 사용자 확인 후 게시 |
| [스포너 사용법](../manual/Spawner.md) | Protection 행, Forced Reaction Event Tag 행, Die On Hit 설명, 반응 고정 규칙, 확인 상태 |
| [피격 반응 사용법](../manual/Hit-Reaction.md) | 실행 규칙의 고정 반응 경로, 확인 상태 |
| [Attribute 사용법](../manual/Attributes.md) | Die On Hit Tag 사용법, 판정 순서 표 |
| [GameplayTag 사용법](../manual/Gameplay-Tags.md) | `Status` 루트에 Die On Hit Tag와 샘플 태그 |
| [런타임 스포너 사용법](../manual/Runtime-Spawner.md) | 패널에서 고를 수 없는 설정 |
| [사망 사용법](../manual/Death.md) | 영향 없음. Die On Hit은 기존 Health 0 사망 경로를 그대로 쓴다 |
