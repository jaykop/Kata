#include "HitTrace/KataHitTraceSettings.h"

UKataHitTraceSettings::UKataHitTraceSettings()
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("Kata Hit Trace");
}

const UKataHitTraceSettings* UKataHitTraceSettings::Get()
{
    return GetDefault<UKataHitTraceSettings>();
}

FName UKataHitTraceSettings::GetValidHurtBoxProfileName() const
{
    FCollisionResponseTemplate Template;
    const FName ProfileName = HurtBoxCollisionProfile.Name;
    return !ProfileName.IsNone() && UCollisionProfile::Get()->GetProfileTemplate(ProfileName, Template) ? ProfileName : NAME_None;
}

ECollisionChannel UKataHitTraceSettings::GetHurtBoxObjectType() const
{
    FCollisionResponseTemplate Template;
    const FName ProfileName = HurtBoxCollisionProfile.Name;
    if (!ProfileName.IsNone() && UCollisionProfile::Get()->GetProfileTemplate(ProfileName, Template))
    {
        return Template.ObjectType;
    }
    return ECC_WorldDynamic;
}
