#include "Attributes/KataCombatSettings.h"

UKataCombatSettings::UKataCombatSettings()
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("Kata Combat");
}

const UKataCombatSettings* UKataCombatSettings::Get()
{
    return GetDefault<UKataCombatSettings>();
}
