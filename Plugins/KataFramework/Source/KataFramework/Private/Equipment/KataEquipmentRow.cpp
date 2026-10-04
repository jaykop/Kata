#include "Equipment/KataEquipmentRow.h"

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
