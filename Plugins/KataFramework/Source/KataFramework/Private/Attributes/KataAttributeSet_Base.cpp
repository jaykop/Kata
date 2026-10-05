#include "Attributes/KataAttributeSet_Base.h"

#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

void UKataAttributeSet_Base::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    const FGameplayAttribute& Attribute = Data.EvaluatedData.Attribute;
    if (Attribute == GetDamageAttribute())
    {
        const float DamageDone = GetDamage();
        SetDamage(0.0f);
        if (DamageDone > 0.0f)
        {
            SetHealth(GetHealth() - DamageDone);
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
