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

const FKataImpactResponse* UKataCombatSettings::FindImpactResponse(const FGameplayTag& ImpactTag) const
{
    const FGameplayTag SearchTag = ImpactTag.IsValid() ? ImpactTag : DefaultImpactTag;
    if (!SearchTag.IsValid())
    {
        return nullptr;
    }

    // Impact.Knock이 Impact.Knock.Down을 대신하면 등급이 섞이므로 계층 일치가 아니라 정확 일치로 찾는다.
    return ImpactResponses.FindByPredicate([&SearchTag](const FKataImpactResponse& Response)
    {
        return Response.ImpactTag == SearchTag;
    });
}
