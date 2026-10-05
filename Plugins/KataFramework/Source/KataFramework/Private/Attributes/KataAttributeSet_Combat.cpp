#include "Attributes/KataAttributeSet_Combat.h"

void UKataAttributeSet_Combat::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
    if (Attribute == GetAttackPowerAttribute() || Attribute == GetDefenseAttribute())
    {
        // 디버프가 겹쳐도 음수가 되지 않게 한다. 음수 방어력은 비율 식에서 피해를 무한히 키운다.
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}
