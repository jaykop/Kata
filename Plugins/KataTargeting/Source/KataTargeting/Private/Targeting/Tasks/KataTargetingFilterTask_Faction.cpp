#include "Targeting/Tasks/KataTargetingFilterTask_Faction.h"

#include "FunctionLibraries/KataFL_Faction.h"
#include "Targeting/Tasks/KataTargetingViewUtils.h"

UKataTargetingFilterTask_Faction::UKataTargetingFilterTask_Faction(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AllowedAttitudes.Add(ETeamAttitude::Hostile);
}

bool UKataTargetingFilterTask_Faction::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
    const AActor* Target = TargetData.HitResult.GetActor();
    if (Target == nullptr)
    {
        return true;
    }

    const TEnumAsByte<ETeamAttitude::Type> Attitude =
        UKataFL_Faction::GetActorAttitude(KataTargetingView::GetSourceActor(TargetingHandle), Target);
    return !AllowedAttitudes.Contains(Attitude);
}
