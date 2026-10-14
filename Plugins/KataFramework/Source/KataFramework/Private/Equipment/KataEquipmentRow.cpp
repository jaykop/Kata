#include "Equipment/KataEquipmentRow.h"

#include "Equipment/KataEquipmentSetup.h"
#include "GameplayEffect.h"

void FKataEquipmentRow::GatherAssetsToLoad(TArray<FSoftObjectPath>& OutPaths) const
{
    for (const FKataEquipmentPart& Part : Parts)
    {
        if (!Part.Mesh.IsNull())
        {
            OutPaths.AddUnique(Part.Mesh.ToSoftObjectPath());
        }
    }
    for (const TSoftClassPtr<UGameplayEffect>& Effect : GrantedEffects)
    {
        if (!Effect.IsNull())
        {
            OutPaths.AddUnique(Effect.ToSoftObjectPath());
        }
    }
}

FGameplayTagContainer FKataEquipmentRow::GetOccupiedSlots(const FGameplayTag& TargetSlot) const
{
    FGameplayTagContainer Occupied;
    if (!TargetSlot.IsValid() || !AllowedSlots.HasTagExact(TargetSlot))
    {
        return Occupied;
    }

    Occupied.AddTag(TargetSlot);
    for (const FKataEquipmentPart& Part : Parts)
    {
        if (Part.Slot.IsValid())
        {
            Occupied.AddTag(Part.Slot);
        }
    }
    return Occupied;
}

FGameplayTag FKataEquipmentRow::ResolveTargetSlot(const FGameplayTag& RequestedSlot, const UKataEquipmentSetup* Setup) const
{
    if (RequestedSlot.IsValid())
    {
        return RequestedSlot;
    }
    const FGameplayTag DefaultSlot = Setup != nullptr ? Setup->DefaultSlot : FGameplayTag();
    return DefaultSlot.IsValid() && AllowedSlots.HasTagExact(DefaultSlot) ? DefaultSlot : AllowedSlots.First();
}
