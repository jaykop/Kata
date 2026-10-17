#include "Attributes/KataAttributeSet_Base.h"

#include "AbilitySystemComponent.h"
#include "Attributes/KataCombatEventSubsystem.h"
#include "Attributes/KataCombatSettings.h"
#include "GameFramework/CheatManagerDefines.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

namespace KataAttributeSet_Base
{
    /** 설정의 대미지 무시 태그를 가졌으면 true. 치트용 태그는 Shipping에서 보지 않는다. */
    bool ShouldIgnoreDamage(const UAbilitySystemComponent* AbilitySystem)
    {
        if (AbilitySystem == nullptr)
        {
            return false;
        }

        const UKataCombatSettings* Settings = UKataCombatSettings::Get();
        if (Settings->IgnoreDamageTag.IsValid() && AbilitySystem->HasMatchingGameplayTag(Settings->IgnoreDamageTag))
        {
            return true;
        }
#if UE_WITH_CHEAT_MANAGER
        if (Settings->CheatIgnoreDamageTag.IsValid() && AbilitySystem->HasMatchingGameplayTag(Settings->CheatIgnoreDamageTag))
        {
            return true;
        }
#endif
        return false;
    }

    /** 피해를 낸 쪽 ASC가 치트 한 방 태그를 가졌으면 true. 치트용 태그는 Shipping에서 보지 않는다. */
    bool IsOneHitKillDamage(const FGameplayEffectSpec& Spec)
    {
#if UE_WITH_CHEAT_MANAGER
        const FGameplayTag& Tag = UKataCombatSettings::Get()->CheatOneHitKillTag;
        const UAbilitySystemComponent* Instigator = Spec.GetContext().GetInstigatorAbilitySystemComponent();
        return Tag.IsValid() && Instigator != nullptr && Instigator->HasMatchingGameplayTag(Tag);
#else
        return false;
#endif
    }
}

void UKataAttributeSet_Base::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    const FGameplayAttribute& Attribute = Data.EvaluatedData.Attribute;
    if (Attribute == GetDamageAttribute())
    {
        float DamageDone = GetDamage();
        SetDamage(0.0f);
        // 한 방 치트는 원래 피해가 있을 때만 남은 Health까지 올린다. 피해 0인 GE(반응 전용 등)로는 죽이지 않는다.
        if (DamageDone > 0.0f && KataAttributeSet_Base::IsOneHitKillDamage(Data.EffectSpec))
        {
            DamageDone = FMath::Max(DamageDone, GetHealth());
        }
        // 대미지 무시는 Health 반영만 막는다. GE는 이미 실행됐으므로 피격 반응 GE 컴포넌트와 Poise·Groggy 누적은 그대로 동작한다.
        if (DamageDone > 0.0f && !KataAttributeSet_Base::ShouldIgnoreDamage(GetOwningAbilitySystemComponent()))
        {
            const float OldHealth = GetHealth();
            SetHealth(OldHealth - DamageDone);
            // 0으로 깎이는 경우 넘친 피해는 빼고 실제로 줄어든 양만 알린다. 이미 0이었으면 알리지 않는다.
            const float HealthLost = OldHealth - GetHealth();
            if (UKataCombatEventSubsystem* CombatEvents = HealthLost > 0.0f ? UKataCombatEventSubsystem::Get(GetOwningActor()) : nullptr)
            {
                CombatEvents->BroadcastDamageApplied(GetOwningAbilitySystemComponent()->GetAvatarActor(),
                    Data.EffectSpec.GetEffectContext().GetOriginalInstigator(), HealthLost);
            }
        }
    }
    else if (Attribute == GetHealingAttribute())
    {
        const float HealingDone = GetHealing();
        SetHealing(0.0f);
        if (HealingDone > 0.0f)
        {
            SetHealth(GetHealth() + HealingDone);
        }
    }
    else if (Attribute != GetHealthAttribute())
    {
        return;
    }

    // Health 기본값 변경은 PreAttributeBaseChange에서 이미 0~MaxHealth로 제한되었다.
    const bool bNowOutOfHealth = GetHealth() <= 0.0f;
    if (bNowOutOfHealth && !bOutOfHealth)
    {
        bOutOfHealth = true;
        OnOutOfHealth.Broadcast(Data.EffectSpec.GetEffectContext().GetOriginalInstigator(), Data.EffectSpec);
    }
    else if (!bNowOutOfHealth)
    {
        bOutOfHealth = false;
    }
}

void UKataAttributeSet_Base::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if (Attribute == GetMaxHealthAttribute())
    {
        AdjustCurrentForMaxChange(GetHealthAttribute(), OldValue, NewValue);
    }
    else if (Attribute == GetMaxStaminaAttribute())
    {
        AdjustCurrentForMaxChange(GetStaminaAttribute(), OldValue, NewValue);
    }
    else if (Attribute == GetMaxManaAttribute())
    {
        AdjustCurrentForMaxChange(GetManaAttribute(), OldValue, NewValue);
    }
}

void UKataAttributeSet_Base::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetStaminaAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
    }
    else if (Attribute == GetManaAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
    }
    else if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxStaminaAttribute() || Attribute == GetMaxManaAttribute()
        || Attribute == GetStaminaRegenRateAttribute() || Attribute == GetManaRegenRateAttribute())
    {
        // 디버프가 겹쳐도 최대값과 회복 속도는 음수가 되지 않는다. 회복을 피해로 바꾸는 효과는 별도 GE로 표현한다.
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}
