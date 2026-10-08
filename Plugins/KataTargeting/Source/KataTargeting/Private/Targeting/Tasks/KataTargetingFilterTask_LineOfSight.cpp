#include "Targeting/Tasks/KataTargetingFilterTask_LineOfSight.h"

#include "GameFramework/Actor.h"
#include "Targeting/Tasks/KataTargetingViewUtils.h"

UKataTargetingFilterTask_LineOfSight::UKataTargetingFilterTask_LineOfSight(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

bool UKataTargetingFilterTask_LineOfSight::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
    FVector TargetLocation;
    if (!KataTargetingView::GetTargetLocation(TargetData, TargetLocation))
    {
        return true;
    }

    // 시점을 구할 수 없으면 가림을 판정할 수 없으므로 후보를 남긴다. 다른 필터의 판정에 맡긴다.
    const AActor* SourceActor = KataTargetingView::GetSourceActor(TargetingHandle);
    FVector ViewLocation;
    FRotator ViewRotation;
    if (!KataTargetingView::GetViewPoint(SourceActor, ViewLocation, ViewRotation))
    {
        return false;
    }

    return !KataTargetingView::HasLineOfSight(SourceActor, TargetData.HitResult.GetActor(), ViewLocation, TargetLocation, TraceChannel);
}
