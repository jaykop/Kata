#include "UI/KataHUDSettings.h"

UKataHUDSettings::UKataHUDSettings()
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("Kata HUD");
}

const UKataHUDSettings* UKataHUDSettings::Get()
{
    return GetDefault<UKataHUDSettings>();
}
