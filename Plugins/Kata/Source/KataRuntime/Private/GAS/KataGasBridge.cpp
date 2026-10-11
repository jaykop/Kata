#include "GAS/KataGasBridge.h"

#include "AbilitySystemComponent.h"
#include "Action/KataAction.h"
#include "Action/KataResolvedAction.h"
#include "GAS/KataCooldownGameplayEffect.h"
#include "GameplayEffect.h"
#include "KataRuntimeLog.h"

namespace
{
    /** 태그를 공유하지 않는 쿨다운을 구분할 안정적인 원본 객체를 반환한다. */
    const UObject* GetCooldownSource(const UKataResolvedAction& Definition)
    {
        return static_cast<const UObject*>(Definition.SourceAction.Get());
    }
}

namespace KataGas
{
    EKataStartResult CheckActivationTags(const UKataResolvedAction& Definition, const UAbilitySystemComponent& AbilitySystem)
    {
        if (!Definition.ActivationRequiredTags.IsEmpty() && !AbilitySystem.HasAllMatchingGameplayTags(Definition.ActivationRequiredTags))
        {
            return EKataStartResult::MissingRequiredTags;
        }
        if (!Definition.ActivationBlockedTags.IsEmpty() && AbilitySystem.HasAnyMatchingGameplayTags(Definition.ActivationBlockedTags))
        {
            return EKataStartResult::BlockedByTags;
        }
        return EKataStartResult::Started;
    }

    EKataStartResult CheckBlockedAbilityTags(const UKataResolvedAction& Definition, const UAbilitySystemComponent& AbilitySystem,
        const UKataResolvedAction* TransitionSource)
    {
        if (Definition.KataTags.IsEmpty())
        {
            return EKataStartResult::Started;
        }

        const FGameplayTagContainer& SourceBlockedTags = TransitionSource != nullptr
            ? TransitionSource->BlockingPolicy.BlockedAbilityTags
            : FGameplayTagContainer::EmptyContainer;
        if (SourceBlockedTags.IsEmpty())
        {
            return AbilitySystem.AreAbilityTagsBlocked(Definition.KataTags)
                ? EKataStartResult::BlockedByAbilityTags
                : EKataStartResult::Started;
        }

        // 차단 카운트는 ASC 내부에 숨겨져 있어 현재 액션의 몫만 덜 수 없다. 현재 액션이 건 태그를 통째로 빼는 근사를 쓴다.
        FGameplayTagContainer BlockedTags = AbilitySystem.GetBlockedAbilityTags();
        BlockedTags.RemoveTags(SourceBlockedTags);
        return Definition.KataTags.HasAny(BlockedTags)
            ? EKataStartResult::BlockedByAbilityTags
            : EKataStartResult::Started;
    }

    bool IsOnCooldown(const UKataResolvedAction& Definition, const UAbilitySystemComponent& AbilitySystem)
    {
        const FKataCooldownPolicy& Policy = Definition.CooldownPolicy;
        if (!Policy.bEnabled || Policy.Duration <= 0.0f)
        {
            return false;
        }

        if (!Policy.GroupTags.IsEmpty())
        {
            return AbilitySystem.HasAnyMatchingGameplayTags(Policy.GroupTags);
        }

        const UObject* Source = GetCooldownSource(Definition);
        if (Source == nullptr)
        {
            return false;
        }

        FGameplayEffectQuery Query;
        Query.EffectSource = Source;
        Query.EffectDefinition = UKataCooldownGameplayEffect::StaticClass();
        return !AbilitySystem.GetActiveEffects(Query).IsEmpty();
    }

    void AddActiveGrantedTags(const UKataResolvedAction& Definition, UAbilitySystemComponent& AbilitySystem)
    {
        if (Definition.ActiveGrantedTags.IsEmpty())
        {
            return;
        }
        AbilitySystem.AddLooseGameplayTags(Definition.ActiveGrantedTags, 1);
    }

    void RemoveActiveGrantedTags(const UKataResolvedAction& Definition, UAbilitySystemComponent& AbilitySystem)
    {
        if (Definition.ActiveGrantedTags.IsEmpty())
        {
            return;
        }
        AbilitySystem.RemoveLooseGameplayTags(Definition.ActiveGrantedTags, 1);
    }

    void BlockAbilities(const UKataResolvedAction& Definition, UAbilitySystemComponent& AbilitySystem)
    {
        if (Definition.BlockingPolicy.BlockedAbilityTags.IsEmpty())
        {
            return;
        }
        AbilitySystem.BlockAbilitiesWithTags(Definition.BlockingPolicy.BlockedAbilityTags);
    }

    void UnblockAbilities(const UKataResolvedAction& Definition, UAbilitySystemComponent& AbilitySystem)
    {
        if (Definition.BlockingPolicy.BlockedAbilityTags.IsEmpty())
        {
            return;
        }
        AbilitySystem.UnBlockAbilitiesWithTags(Definition.BlockingPolicy.BlockedAbilityTags);
    }

    FActiveGameplayEffectHandle ApplyCooldown(const UKataResolvedAction& Definition, UAbilitySystemComponent& AbilitySystem)
    {
        const FKataCooldownPolicy& Policy = Definition.CooldownPolicy;
        if (!Policy.bEnabled || Policy.Duration <= 0.0f)
        {
            return FActiveGameplayEffectHandle();
        }

        const UObject* Source = GetCooldownSource(Definition);
        if (Source == nullptr)
        {
            UE_LOG(LogKata, Warning, TEXT("Kata cooldown has no stable source object"));
            return FActiveGameplayEffectHandle();
        }

        FGameplayEffectContextHandle EffectContext = AbilitySystem.MakeEffectContext();
        EffectContext.AddSourceObject(Source);

        const FGameplayEffectSpecHandle SpecHandle = AbilitySystem.MakeOutgoingSpec(
            UKataCooldownGameplayEffect::StaticClass(), 1.0f, EffectContext);
        if (!SpecHandle.IsValid())
        {
            UE_LOG(LogKata, Warning, TEXT("Failed to create cooldown spec for '%s'"),
                *Source->GetName());
            return FActiveGameplayEffectHandle();
        }

        SpecHandle.Data->SetDuration(Policy.Duration, true);
        SpecHandle.Data->DynamicGrantedTags.AppendTags(Policy.GroupTags);
        return AbilitySystem.ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
    }
}
