#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "KataRuntimeTypes.h"

class UAbilitySystemComponent;
class UKataResolvedAction;

/**
 * Kata 설정이 GAS의 어떤 동작으로 연결되는지 한곳에 모은 계층.
 *
 * Kata는 정책과 설정을 제공하고 실제 효과는 GAS가 적용한다.
 * 호출 Ability는 Kata가 활성화한 쿨다운을 다시 적용하지 않는다.
 */
namespace KataGas
{
    /** ActivationRequiredTags와 ActivationBlockedTags를 ASC의 현재 태그로 판정한다. */
    KATARUNTIME_API EKataStartResult CheckActivationTags(const UKataResolvedAction& Definition, const UAbilitySystemComponent& AbilitySystem);

    /**
     * ASC의 차단 Ability 태그가 KataTags와 맞으면 BlockedByAbilityTags를 돌려준다.
     * GA·GE의 Block Abilities with Tag와 다른 Kata의 BlockedAbilityTags가 Kata 시작에도 적용되게 하는 판정이다.
     *
     * TransitionSource는 그래프 전이에서 교체될 현재 액션의 정의다. 그 액션이 등록한 BlockedAbilityTags는 판정에서 뺀다.
     * ASC는 차단 태그를 누가 등록했는지 공개하지 않으므로, 같은 태그를 다른 출처도 막고 있으면 전이는 통과한다.
     * 일반 시작에는 nullptr를 넘긴다.
     */
    KATARUNTIME_API EKataStartResult CheckBlockedAbilityTags(const UKataResolvedAction& Definition, const UAbilitySystemComponent& AbilitySystem,
        const UKataResolvedAction* TransitionSource);

    /** 공유 그룹 태그 또는 원본 Kata를 기준으로 활성 쿨다운 Effect를 찾는다. */
    KATARUNTIME_API bool IsOnCooldown(const UKataResolvedAction& Definition, const UAbilitySystemComponent& AbilitySystem);

    /**
     * ActiveGrantedTags를 부여한다. 싱글플레이 전용이므로 Loose Tag를 사용한다.
     * 같은 태그를 여러 곳에서 부여할 수 있으므로 자신의 기여분만 카운트로 회수한다.
     */
    KATARUNTIME_API void AddActiveGrantedTags(const UKataResolvedAction& Definition, UAbilitySystemComponent& AbilitySystem);

    KATARUNTIME_API void RemoveActiveGrantedTags(const UKataResolvedAction& Definition, UAbilitySystemComponent& AbilitySystem);

    /** BlockedAbilityTags를 GAS의 Ability 차단 목록에 등록한다. */
    KATARUNTIME_API void BlockAbilities(const UKataResolvedAction& Definition, UAbilitySystemComponent& AbilitySystem);

    KATARUNTIME_API void UnblockAbilities(const UKataResolvedAction& Definition, UAbilitySystemComponent& AbilitySystem);

    /** 설정된 시간과 공유 그룹으로 내부 Duration Gameplay Effect를 적용한다. */
    KATARUNTIME_API FActiveGameplayEffectHandle ApplyCooldown(const UKataResolvedAction& Definition, UAbilitySystemComponent& AbilitySystem);
}
