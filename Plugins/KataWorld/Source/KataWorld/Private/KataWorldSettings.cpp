#include "KataWorldSettings.h"

UKataWorldSettings::UKataWorldSettings()
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("Kata World");
}

const UKataWorldSettings* UKataWorldSettings::Get()
{
    return GetDefault<UKataWorldSettings>();
}
