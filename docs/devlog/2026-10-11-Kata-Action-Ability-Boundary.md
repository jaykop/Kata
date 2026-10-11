# Kata 액션과 Gameplay Ability 경계

작성: 2026-10-11  
갱신: 2026-10-11  
유형: 결정 기록, 구현 기록  
대상: KataRuntime(`UKataActionComponent`, `KataGas`), KataGraph(`UKataGraphComponent`, `UKataGraphInstance`), KataFramework(`UKataDeathComponent`, `AKataCharacter`)  
기준: 2026-10-11 작업 트리. `KataActionComponent.cpp`에 함께 있던 #47 측정용 TRACE 스코프는 이 기록의 대상이 아니다

## 배경과 결론

2026-10-10 사용자가 "GA는 거의 없고 Component만 늘어난다"는 구조 의문을 제기했다. 진단 결과 기존 Component는 대상의 수명 동안 유지되는 상태, 실행 인프라, 씬 프리미티브라 Component가 맞았다. GA가 적은 이유는 `UKataAction`이 GA의 활성화 규칙(활성 태그, 차단, 쿨다운, 시작 조건)과 겹치는 행동 단위를 맡기 때문이며, 이는 의도한 설계였다. 문제는 두 계층의 경계 규칙이 없었고, GAS의 취소·차단이 직접 재생한 Kata와 그래프에 닿지 않는다는 점이었다.

#46에는 2026-10-10 결정(입력·그래프·StateTree는 범용 실행 GA로 모으지 않는다)이 이미 있었다. 이번에 범위를 경계 규칙으로 넓혀 세 선택지를 비교했고, 2026-10-11 사용자가 절충안 (c)를 택했다. 이번에 구현한 것은 공용 중단 진입점, ASC 차단 태그의 Kata 적용, 액션 컴포넌트의 Tick 정리, 사망 정리 전환이다.

## 변경 또는 진단 내용

| 항목 | 이전 상태 | 반영 결과 |
|---|---|---|
| GAS 취소 | `CancelAbilities`는 GA만 취소했다. 사망 처리는 Ability 취소 뒤 `StopGraph`·`StopKata`를 따로 불렀다 | `UKataActionComponent::CancelAbilitiesAndKata`가 Ability, Kata, 그래프를 함께 끝낸다. 사망 처리가 이 함수를 쓴다 |
| GAS 차단 | Kata 시작 판정은 보유 태그만 읽었다. GA·GE의 Block Abilities with Tag는 Kata를 막지 못했다 | 시작 판정이 ASC의 차단 Ability 태그를 KataTags로 확인하고, 맞으면 `BlockedByAbilityTags`로 거절한다 |
| 액션 컴포넌트 Tick | 재생 여부와 관계없이 매 프레임 Tick했다. Game·PIE에서는 Subsystem이 진행하므로 빈 Tick이었다 | Subsystem이 없는 월드에서 인스턴스가 실행되는 동안에만 Tick한다 |
| 실행 방식 플래그 | 전이 중 늦게 끝난 이전 인스턴스의 등록 해제가 현재 인스턴스의 플래그를 지웠다 | 현재 인스턴스가 없거나 끝나는 인스턴스가 현재 인스턴스일 때만 플래그와 Tick을 되돌린다 |
| 경계 규칙 | 문서에 없었다 | [런타임 사용법](../manual/Runtime-Usage.md#gas와의-경계)에 옮겼다 |

## 주요 결정과 이유

### 엔진 동작 확인

UE 5.8 GameplayAbilities 소스로 다음을 확인했다.

- `UAbilitySystemComponent::CancelAbilities`는 가상 함수가 아니며, 부여된 Spec마다 `Spec.Ability->GetAssetTags()`(GA 클래스에 고정된 태그)로 대상을 고른다.
- GE의 Cancel Abilities with Tags 컴포넌트(`UCancelAbilityTagsGameplayEffectComponent`)도 내부에서 `CancelAbilities`를 부른다.
- GA 활성화 때 취소·차단을 적용하는 가상 함수는 `ApplyAbilityBlockAndCancelTags`뿐이다. GA의 `CancelAbilitiesWithTag`·`BlockAbilitiesWithTag` 멤버는 protected다.
- ASC의 차단 태그 카운트(`BlockedAbilityTags`, `FGameplayTagCountContainer`)는 protected이고, 공개 API는 명시 태그 목록(`GetBlockedAbilityTags`)과 `AreAbilityTagsBlocked`뿐이다.

### 선택지

| 선택지 | 내용 | 판단 |
|---|---|---|
| (a) 현 구조 유지와 규칙 문서화 | 실행 경로를 두고 공용 중단 진입점과 규칙만 추가한다 | 이득이 바로 생기고 비용이 작다. (c)의 첫 단계와 같다 |
| (b) 모든 Kata를 범용 GA 안에서 실행 | 범용 GA 하나를 부여하고 입력·그래프·StateTree가 그 GA를 활성화한다 | 채택하지 않았다. 태그 없는 전체 취소만 닿고, 태그로 고르는 취소·차단은 범용 GA의 고정 태그 하나로만 표현된다. 실행 수명이 GA와 Kata 인스턴스로 두 겹이 되고, 시작 결과·`Branched`·그래프 거절 정책을 GA 경계 너머로 전달해야 한다. 싱글플레이라 GA의 예측·복제 이점도 없다 |
| (c) Kata를 GAS 차단·취소 규칙에 참여 | 실행 경로를 두고 KataTags가 GA의 Asset Tags 역할을 하게 연결한다. C-1 차단, C-2 공용 중단, C-3 GA 활성화 연동, C-4 GE 연동 | 채택. 같은 GAS 설정을 에셋별 태그로 적용하므로 (b)보다 세밀하고, 실행 수명이 Kata 인스턴스 하나로 유지된다 |

액션마다 GA Blueprint 클래스를 만들고 Kata 타임라인을 그 안의 태스크로만 쓰는 방식은 클래스 단위 액션 정의를 다시 도입하는 것이라 비교에서 뺐다.

성능은 결정 요인이 아니었다. GA(Instanced Per Actor, 복제 안 함)는 부여할 때 인스턴스를 만들고 활성화마다 UObject를 만들지 않으며, Ability Task는 태스크마다 하나다. GA로 감싸면 활성화마다 UObject 하나와 GA 활성화 처리가 더해지지만, 현재 Kata 해석 비용([#47](https://github.com/jaykop/Kata/issues/47))에 비하면 작다. (c)가 더하는 비용은 시작 판정의 태그 비교 한 번이다.

### 지금 구현할 범위

C-1·C-3·C-4를 미리 만들 때 손해가 있는지 사용자와 항목별로 따졌다.

- C-3(GA 활성화 연동)·C-4(GE 연동)는 새 기능을 더하는 것이라 나중에 추가해도 비용이 같다. 미리 만들면 쓰이지 않는 코드가 검증 없이 남고 설계를 실제 사용처 없이 정하게 된다. 첫 사용처가 생길 때 추가한다.
- C-1(차단 연동)은 기존 설정 `BlockedAbilityTags`의 의미를 넓힌다. 같은 값이 GA뿐 아니라 KataTags가 맞는 다른 Kata도 막게 된다. 액션 에셋이 늘수록 점검 범위가 커지므로 에셋이 적은 지금 넣는다. 에디터 MCP로 샘플 액션 16개와 Template 1개를 확인한 결과, 모두 KataTags와 BlockedAbilityTags가 비어 있어 기존 동작은 바뀌지 않는다.

### 세부 결정

- 공용 중단은 코어 KataRuntime의 `UKataActionComponent`에 둔다. GAS에만 의존하므로 코어 경계를 지킨다. 그래프는 KataRuntime이 알 수 없으므로 C++ 알림 `OnCancelAllRequested`를 두고 `UKataGraphComponent`가 구독한다.
- 첫 액션을 기다리는 그래프는 필터가 없는 전체 중단에서만 끝낸다. 실행 중인 액션이 없어 태그로 고를 대상이 없기 때문이다.
- `IgnoreAbility`가 재생한 Kata는 `FKataContext::OwningAbility`로 구분해 남긴다.
- 그래프 전이는 현재 액션의 차단 정책을 우회하는 기존 규칙을 따라, 현재 액션이 등록한 `BlockedAbilityTags`를 판정에서 뺀다. ASC가 출처별 카운트를 공개하지 않으므로 그 태그를 통째로 빼는 근사를 쓴다. 같은 태그를 다른 출처도 막고 있으면 전이가 통과한다.
- `BlockedKataTags`는 유지한다. 실행 중인 Kata 하나만 비교하는지, ASC 전체 차단인지가 다르다. 정리 여부는 나중에 다시 정한다.
- 거절 사유 `BlockedByAbilityTags`는 직렬화 값이 바뀌지 않도록 `EKataStartResult` 끝에 추가했다. 보유 태그로 막힌 `BlockedByTags`와 구분해야 원인을 찾기 쉽다.
- 사망 컴포넌트는 소유자에 `UKataActionComponent`가 없으면 지금처럼 `CancelAllAbilities`만 부른다. 캐릭터가 아닌 피해 대상을 위해서다.

## 근거

- [KataActionComponent.cpp](../../Plugins/Kata/Source/KataRuntime/Private/Runtime/KataActionComponent.cpp): `CancelAbilitiesAndKata`, 시작 판정 순서, 대체 Tick과 등록 해제 조건.
- [KataGasBridge.cpp](../../Plugins/Kata/Source/KataRuntime/Private/GAS/KataGasBridge.cpp): `KataGas::CheckBlockedAbilityTags`와 전이 예외.
- [KataGraphComponent.cpp](../../Plugins/Kata/Source/KataGraph/Private/KataGraphComponent.cpp): 전체 중단 알림 구독.
- [KataGraphInstance.cpp](../../Plugins/Kata/Source/KataGraph/Private/KataGraphInstance.cpp): `BlockedByAbilityTags`를 게임플레이 거절로 분류.
- [KataDeathComponent.cpp](../../Plugins/KataFramework/Source/KataFramework/Private/Death/KataDeathComponent.cpp): 사망 정리의 공용 중단 호출.
- UE 5.8 `AbilitySystemComponent.h`·`AbilitySystemComponent_Abilities.cpp`·`CancelAbilityTagsGameplayEffectComponent.cpp`·`GameplayAbility.h`: 위 엔진 동작.

## 확인 범위와 결과

| 대상 | 확인 방법·수행자 | 결과 | 미확인 범위 |
|---|---|---|---|
| 샘플 액션의 KataTags·BlockedAbilityTags | 에디터 MCP로 에셋 속성 읽기, 에이전트(2026-10-11) | 16개 액션과 Template 1개 모두 비어 있음 | 없음 |
| 전체 변경 | 사용자 Editor 빌드(2026-10-11) | 통과 | Game 타깃 빌드 |
| 동작 | 사용자 PIE(2026-10-11) | 재생하지 않을 때 Tick 꺼짐, 공격·피격 반응·콤보 도중 사망, 콤보·피격·캔슬·AI 행동과 액션 편집기 프리뷰 회귀 모두 이상 없음 | BlockedByAbilityTags 거절이 실제로 일어나는 경우와 태그 필터를 준 `CancelAbilitiesAndKata`(샘플에 KataTags가 없음) |

에이전트는 빌드·테스트를 실행하지 않았다.

## 남은 제한과 후속 작업

- C-3: 다른 행동을 태그로 끊는 GA가 처음 생길 때 ASC 파생 클래스(현재 제안은 코어의 선택 사항인 `UKataAbilitySystemComponent`)로 `ApplyAbilityBlockAndCancelTags`를 재정의해 Cancel Abilities with Tag를 Kata에 적용한다.
- C-4: 기절 GE처럼 GE로 행동을 끊는 첫 사용처가 생길 때 Kata용 GE 취소 컴포넌트를 추가한다. 엔진 컴포넌트와 같은 설정을 받고 `CancelAbilitiesAndKata`를 부른다.
- 실행 Subsystem은 매 프레임 등록 목록을 지역 배열로 복사한다. 멤버 배열 재사용은 이번 범위에서 뺐다.
- GA가 자기 Block Abilities with Tag로 자기가 재생할 Kata의 KataTags를 막으면 그 Kata가 거절된다. GA의 차단 태그 멤버가 protected라 예외로 뺄 수 없다.
- 직접 재생한 Kata는 `showdebug abilitysystem`과 GAS Inspector에 보이지 않는다. 필요하면 Kata 쪽 디버그 기능으로 따로 다룬다.
- `UKataInputHandlerComponent::Jump`는 `IsPlayingKata()`로 점프를 막으므로 Kata를 재생하지 않는 GA가 활성인 동안에는 점프가 막히지 않는다. 이번 범위에서 바꾸지 않았다.

## 연관 문서 반영

| 문서 | 반영 내용 또는 미반영 사유 |
|---|---|
| [연결 이슈 #46](https://github.com/jaykop/Kata/issues/46) | 결정 요약과 구현·확인 결과 댓글 초안을 준비했다. 게시는 사용자 확인 후 |
| [런타임 사용법](../manual/Runtime-Usage.md) | "GAS와의 경계" 항목, 공용 중단, `BlockedByAbilityTags`, 대체 Tick, BlockingPolicy 두 항목의 차이 |
| [사망 사용법](../manual/Death.md) | 처리 순서 2번을 공용 중단 호출로 바꾸고 확인 결과를 더했다 |
| 경계 계획(`docs/plan/Kata-Action-Ability-Boundary-Plan.md`) | #46을 닫으며 결정과 선택지 비교를 이 기록으로 옮기고 삭제했다 |
