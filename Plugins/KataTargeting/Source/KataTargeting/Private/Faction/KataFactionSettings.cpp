#include "Faction/KataFactionSettings.h"

#include "KataTargetingLog.h"

namespace
{
    /** 태그 계층 깊이. Faction.Monster.Undead는 3이다. 태그 관리자 없이 이름만으로 센다. */
    int32 GetTagDepth(const FGameplayTag& Tag)
    {
        const FString Name = Tag.ToString();
        int32 Depth = 1;
        for (const TCHAR Character : Name)
        {
            if (Character == TEXT('.'))
            {
                ++Depth;
            }
        }
        return Depth;
    }

    /** 엔진 NoTeam(255)을 피해 쓸 수 있는 팀 번호의 개수. */
    constexpr int32 MaxTeamCount = 255;
}

UKataFactionSettings::UKataFactionSettings()
{
    CategoryName = TEXT("Plugins");
    SectionName = TEXT("Kata Factions");
}

const UKataFactionSettings* UKataFactionSettings::Get()
{
    return GetDefault<UKataFactionSettings>();
}

FGenericTeamId UKataFactionSettings::GetTeamId(const FGameplayTag& Faction) const
{
    if (!Faction.IsValid())
    {
        return FGenericTeamId::NoTeam;
    }

    const int32 Index = Factions.IndexOfByKey(Faction);
    return Index != INDEX_NONE && Index < MaxTeamCount ? FGenericTeamId(static_cast<uint8>(Index)) : FGenericTeamId::NoTeam;
}

FGameplayTag UKataFactionSettings::GetFaction(FGenericTeamId TeamId) const
{
    const int32 Index = TeamId.GetId();
    return TeamId != FGenericTeamId::NoTeam && Factions.IsValidIndex(Index) ? Factions[Index] : FGameplayTag();
}

ETeamAttitude::Type UKataFactionSettings::GetFactionAttitude(const FGameplayTag& FactionA, const FGameplayTag& FactionB) const
{
    if (!FactionA.IsValid() || !FactionB.IsValid())
    {
        return ETeamAttitude::Neutral;
    }

    int32 BestSpecificity = INDEX_NONE;
    ETeamAttitude::Type BestAttitude = ETeamAttitude::Neutral;
    for (const FKataFactionRelation& Relation : Relations)
    {
        if (!Relation.FactionA.IsValid() || !Relation.FactionB.IsValid())
        {
            continue;
        }

        const bool bForward = FactionA.MatchesTag(Relation.FactionA) && FactionB.MatchesTag(Relation.FactionB);
        const bool bReverse = FactionA.MatchesTag(Relation.FactionB) && FactionB.MatchesTag(Relation.FactionA);
        if (!bForward && !bReverse)
        {
            continue;
        }

        // 더 구체적인 태그로 적은 행이 부모 태그 규칙보다 우선한다. 깊이가 같으면 먼저 적은 행을 쓴다.
        const int32 Specificity = GetTagDepth(Relation.FactionA) + GetTagDepth(Relation.FactionB);
        if (Specificity > BestSpecificity)
        {
            BestSpecificity = Specificity;
            BestAttitude = Relation.Attitude;
        }
    }

    if (BestSpecificity != INDEX_NONE)
    {
        return BestAttitude;
    }
    return FactionA == FactionB ? ETeamAttitude::Friendly : ETeamAttitude::Neutral;
}

ETeamAttitude::Type UKataFactionSettings::GetTeamAttitude(FGenericTeamId TeamA, FGenericTeamId TeamB) const
{
    // 엔진 기본 판정은 NoTeam끼리도 같은 번호라 우호로 본다. 팩션이 없는 액터끼리 묶이지 않도록 중립으로 둔다.
    if (TeamA == FGenericTeamId::NoTeam || TeamB == FGenericTeamId::NoTeam)
    {
        return ETeamAttitude::Neutral;
    }

    EnsureAttitudeCache();
    const int32 IndexA = TeamA.GetId();
    const int32 IndexB = TeamB.GetId();
    if (IndexA >= CachedTeamCount || IndexB >= CachedTeamCount)
    {
        return ETeamAttitude::Neutral;
    }
    return static_cast<ETeamAttitude::Type>(AttitudeCache[IndexA * CachedTeamCount + IndexB]);
}

ETeamAttitude::Type UKataFactionSettings::SolveTeamAttitude(FGenericTeamId TeamA, FGenericTeamId TeamB)
{
    const UKataFactionSettings* Settings = Get();
    return Settings != nullptr ? Settings->GetTeamAttitude(TeamA, TeamB) : ETeamAttitude::Neutral;
}

void UKataFactionSettings::PostInitProperties()
{
    Super::PostInitProperties();
    bAttitudeCacheDirty = true;
}

void UKataFactionSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
    Super::PostReloadConfig(PropertyThatWasLoaded);
    bAttitudeCacheDirty = true;
}

#if WITH_EDITOR
void UKataFactionSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    bAttitudeCacheDirty = true;
}
#endif

void UKataFactionSettings::EnsureAttitudeCache() const
{
    if (!bAttitudeCacheDirty)
    {
        return;
    }
    bAttitudeCacheDirty = false;

    if (Factions.Num() > MaxTeamCount)
    {
        UE_LOG(LogKataTargeting, Warning, TEXT("Kata Factions lists %d factions; only the first %d get a team id"),
            Factions.Num(), MaxTeamCount);
    }
    for (int32 Index = 0; Index < Factions.Num(); ++Index)
    {
        if (Factions.IndexOfByKey(Factions[Index]) != Index)
        {
            // 중복 항목은 앞의 팀 번호만 쓰이므로 뒤의 항목은 효과가 없다.
            UE_LOG(LogKataTargeting, Warning, TEXT("Kata Factions lists '%s' more than once; later entries are ignored"),
                *Factions[Index].ToString());
        }
    }

    CachedTeamCount = FMath::Min(Factions.Num(), MaxTeamCount);
    AttitudeCache.SetNumUninitialized(CachedTeamCount * CachedTeamCount);
    for (int32 IndexA = 0; IndexA < CachedTeamCount; ++IndexA)
    {
        for (int32 IndexB = 0; IndexB < CachedTeamCount; ++IndexB)
        {
            AttitudeCache[IndexA * CachedTeamCount + IndexB] =
                static_cast<uint8>(GetFactionAttitude(Factions[IndexA], Factions[IndexB]));
        }
    }
}
