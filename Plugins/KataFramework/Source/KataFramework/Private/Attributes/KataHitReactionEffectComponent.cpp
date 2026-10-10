#include "Attributes/KataHitReactionEffectComponent.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "Attributes/KataAttributeSet_Base.h"
#include "Attributes/KataAttributeSet_Stance.h"
#include "Attributes/KataCombatSettings.h"
#include "GameplayEffect.h"
#include "KataFrameworkLog.h"

void UKataHitReactionGameplayEffectComponent::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer,
    FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
    UAbilitySystemComponent* AbilitySystem = ActiveGEContainer.Owner;
    if (AbilitySystem == nullptr)
    {
        return;
    }

    // 경직하지 않는 대상은 Stance 세트를 넣지 않는다. 피해와 사망만 처리하고 반응은 보내지 않는다.
    const UKataAttributeSet_Stance* Stance = AbilitySystem->GetSet<UKataAttributeSet_Stance>();
    if (Stance == nullptr)
    {
        return;
    }

    // 사망은 모든 반응보다 우선한다. 사망 처리는 OnOutOfHealth를 받는 쪽이 맡는다.
    const UKataAttributeSet_Base* Base = AbilitySystem->GetSet<UKataAttributeSet_Base>();
    if (Base != nullptr && Base->IsOutOfHealth())
    {
        return;
    }

    const UKataCombatSettings* Settings = UKataCombatSettings::Get();
    FGameplayTagContainer AssetTags;
    GESpec.GetAllAssetTags(AssetTags);

    FGameplayTag EventTag;
    const TCHAR* Reason = TEXT("");
    if (Stance->GetMaxGroggy() > 0.0f && Stance->GetGroggy() >= Stance->GetMaxGroggy())
    {
        // Groggy는 그로기 반응이 끝날 때 초기화하므로, 반응 중에 다시 맞아도 여기로 온다. 새 반응은 반응 GA의 차단 태그가 막는다.
        EventTag = Settings->GroggyEventTag;
        Reason = TEXT("groggy full");
    }
    else if (Settings->SuperArmorTag.IsValid() && AbilitySystem->HasMatchingGameplayTag(Settings->SuperArmorTag))
    {
        EventTag = Settings->FlinchEventTag;
        Reason = TEXT("super armor");
    }
    else if (Stance->GetPoise() > 0.0f)
    {
        EventTag = Settings->FlinchEventTag;
        Reason = TEXT("poise held");
    }
    else if (const FKataImpactResponse* Response = Settings->FindImpactResponseInTags(AssetTags))
    {
        EventTag = Response->ReactionEventTag;
        Reason = TEXT("poise broken");
        if (EventTag.IsValid())
        {
            // 경직한 뒤 다음 피격을 다시 버티도록 무너진 Poise를 바로 채운다. 반응이 없는 등급이면 채우지 않는다.
            AbilitySystem->SetNumericAttributeBase(UKataAttributeSet_Stance::GetPoiseAttribute(), Stance->GetMaxPoise());
        }
    }

    UE_LOG(LogKataFramework, Verbose, TEXT("Kata hit reaction on '%s': %s -> %s (Poise %.1f/%.1f, Groggy %.1f/%.1f, effect '%s')"),
        *GetNameSafe(AbilitySystem->GetAvatarActor()), Reason, *EventTag.ToString(),
        Stance->GetPoise(), Stance->GetMaxPoise(), Stance->GetGroggy(), Stance->GetMaxGroggy(), *GetNameSafe(GESpec.Def));

    if (!EventTag.IsValid())
    {
        return;
    }

    const FGameplayEffectContextHandle& EffectContext = GESpec.GetEffectContext();
    FGameplayEventData Payload;
    Payload.EventTag = EventTag;
    Payload.Instigator = EffectContext.GetOriginalInstigator();
    Payload.Target = AbilitySystem->GetAvatarActor();
    Payload.ContextHandle = EffectContext;
    Payload.InstigatorTags = AssetTags;
    AbilitySystem->HandleGameplayEvent(EventTag, &Payload);
}
