#include "Attributes/KataAttributeSet.h"

void UKataAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UKataAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UKataAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
}

void UKataAttributeSet::AdjustCurrentForMaxChange(const FGameplayAttribute& CurrentAttribute, float OldMaxValue, float NewMaxValue) const
{
    UAbilitySystemComponent* AbilitySystem = GetOwningAbilitySystemComponent();
    if (AbilitySystem == nullptr || FMath::IsNearlyEqual(OldMaxValue, NewMaxValue))
    {
        return;
    }

    const float CurrentValue = CurrentAttribute.GetNumericValue(this);
    const float NewCurrentValue = OldMaxValue > 0.0f ? CurrentValue * NewMaxValue / OldMaxValue : NewMaxValue;
    AbilitySystem->SetNumericAttributeBase(CurrentAttribute, NewCurrentValue);
}
