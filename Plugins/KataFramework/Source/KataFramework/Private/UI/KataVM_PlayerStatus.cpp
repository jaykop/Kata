#include "UI/KataVM_PlayerStatus.h"

namespace
{
    float SafeRatio(float Current, float Max)
    {
        return Max > 0.0f ? FMath::Clamp(Current / Max, 0.0f, 1.0f) : 0.0f;
    }
}

float UKataVM_PlayerStatus::GetHealthRatio() const
{
    return SafeRatio(Health, MaxHealth);
}

float UKataVM_PlayerStatus::GetStaminaRatio() const
{
    return SafeRatio(Stamina, MaxStamina);
}

float UKataVM_PlayerStatus::GetManaRatio() const
{
    return SafeRatio(Mana, MaxMana);
}

void UKataVM_PlayerStatus::SetHealth(float NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(Health, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthRatio);
    }
}

void UKataVM_PlayerStatus::SetMaxHealth(float NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetHealthRatio);
    }
}

void UKataVM_PlayerStatus::SetStamina(float NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(Stamina, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetStaminaRatio);
    }
}

void UKataVM_PlayerStatus::SetMaxStamina(float NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(MaxStamina, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetStaminaRatio);
    }
}

void UKataVM_PlayerStatus::SetMana(float NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(Mana, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetManaRatio);
    }
}

void UKataVM_PlayerStatus::SetMaxMana(float NewValue)
{
    if (UE_MVVM_SET_PROPERTY_VALUE(MaxMana, NewValue))
    {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetManaRatio);
    }
}

void UKataVM_PlayerStatus::SetHasStatus(bool bNewValue)
{
    UE_MVVM_SET_PROPERTY_VALUE(bHasStatus, bNewValue);
}
