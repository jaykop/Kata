#include "FunctionLibraries/KataFL_Faction.h"

#include "Faction/KataFactionSettings.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace
{
    FGenericTeamId GetTeamIdFromAgent(const UObject* Object)
    {
        const IGenericTeamAgentInterface* Agent = Cast<const IGenericTeamAgentInterface>(Object);
        return Agent != nullptr ? Agent->GetGenericTeamId() : FGenericTeamId::NoTeam;
    }
}

FGenericTeamId UKataFL_Faction::GetActorTeamId(const AActor* Actor)
{
    if (Actor == nullptr)
    {
        return FGenericTeamId::NoTeam;
    }

    const FGenericTeamId ActorTeam = GetTeamIdFromAgent(Actor);
    if (ActorTeam != FGenericTeamId::NoTeam)
    {
        return ActorTeam;
    }

    // 엔진 폰은 팀 인터페이스를 구현하지 않고 AIController가 팀을 들고 있는 경우가 많다.
    const APawn* Pawn = Cast<APawn>(Actor);
    return Pawn != nullptr ? GetTeamIdFromAgent(Pawn->GetController()) : FGenericTeamId::NoTeam;
}

FGameplayTag UKataFL_Faction::GetActorFaction(const AActor* Actor)
{
    const UKataFactionSettings* Settings = UKataFactionSettings::Get();
    return Settings != nullptr ? Settings->GetFaction(GetActorTeamId(Actor)) : FGameplayTag();
}

TEnumAsByte<ETeamAttitude::Type> UKataFL_Faction::GetActorAttitude(const AActor* Source, const AActor* Target)
{
    if (Source == nullptr || Target == nullptr)
    {
        return ETeamAttitude::Neutral;
    }
    // AIPerception과 같은 결과를 내도록 전역 관계 판정 함수를 거친다.
    return FGenericTeamId::GetAttitude(GetActorTeamId(Source), GetActorTeamId(Target));
}

bool UKataFL_Faction::IsFriendly(const AActor* Source, const AActor* Target)
{
    return GetActorAttitude(Source, Target) == ETeamAttitude::Friendly;
}

bool UKataFL_Faction::IsNeutral(const AActor* Source, const AActor* Target)
{
    return GetActorAttitude(Source, Target) == ETeamAttitude::Neutral;
}

bool UKataFL_Faction::IsHostile(const AActor* Source, const AActor* Target)
{
    return GetActorAttitude(Source, Target) == ETeamAttitude::Hostile;
}

TEnumAsByte<ETeamAttitude::Type> UKataFL_Faction::GetFactionAttitude(FGameplayTag FactionA, FGameplayTag FactionB)
{
    const UKataFactionSettings* Settings = UKataFactionSettings::Get();
    return Settings != nullptr ? Settings->GetFactionAttitude(FactionA, FactionB) : ETeamAttitude::Neutral;
}

FGenericTeamId UKataFL_Faction::GetFactionTeamId(FGameplayTag Faction)
{
    const UKataFactionSettings* Settings = UKataFactionSettings::Get();
    return Settings != nullptr ? Settings->GetTeamId(Faction) : FGenericTeamId::NoTeam;
}
