#include "Attributes/KataAttributeSet_Stance.h"

#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

void UKataAttributeSet_Stance::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    const FGameplayAttribute& Attribute = Data.EvaluatedData.Attribute;
    if (Attribute == GetPoiseDamageAttribute())
    {
        const float PoiseDamageDone = GetPoiseDamage();
        SetPoiseDamage(0.0f);
        if (PoiseDamageDone > 0.0f)
        {
            SetPoise(GetPoise() - PoiseDamageDone);
        }
    }
    else if (Attribute == GetGroggyDamageAttribute())
    {
        const float GroggyDamageDone = GetGroggyDamage();
        SetGroggyDamage(0.0f);
        if (GroggyDamageDone > 0.0f)
        {
            SetGroggy(GetGroggy() + GroggyDamageDone);
        }
    }
}

void UKataAttributeSet_Stance::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if (Attribute == GetMaxPoiseAttribute())
    {
        AdjustCurrentForMaxChange(GetPoiseAttribute(), OldValue, NewValue);
    }
    else if (Attribute == GetMaxGroggyAttribute())
    {
        // Groggy는 0에서 쌓이는 게이지라 비율 조정을 쓰지 않는다. 비율 조정은 이전 Max가 0이면 가득 채워 시작하자마자 그로기가 된다.
        UAbilitySystemComponent* AbilitySystem = GetOwningAbilitySystemComponent();
        if (AbilitySystem != nullptr && GetGroggy() > NewValue)
        {
            AbilitySystem->SetNumericAttributeBase(GetGroggyAttribute(), NewValue);
        }
    }
}

void UKataAttributeSet_Stance::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
    if (Attribute == GetPoiseAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxPoise());
    }
    else if (Attribute == GetGroggyAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxGroggy());
    }
    else if (Attribute == GetMaxPoiseAttribute() || Attribute == GetMaxGroggyAttribute()
        || Attribute == GetGroggyDecayRateAttribute() || Attribute == GetPoiseDamageTakenMultiplierAttribute())
    {
        // 음수 배율은 Poise 피해를 회복으로 바꾸므로 막는다.
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}
