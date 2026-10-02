#include "Targeting/Tasks/KataTargetingTask_ExpandTargetPoints.h"

#include "GameFramework/Actor.h"
#include "Targeting/KataTargetPointComponent.h"
#include "Types/TargetingSystemTypes.h"

UKataTargetingTask_ExpandTargetPoints::UKataTargetingTask_ExpandTargetPoints(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UKataTargetingTask_ExpandTargetPoints::Execute(const FTargetingRequestHandle& TargetingHandle) const
{
    Super::Execute(TargetingHandle);
    SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Executing);

    FTargetingDefaultResultsSet* ResultData = TargetingHandle.IsValid() ? FTargetingDefaultResultsSet::Find(TargetingHandle) : nullptr;
    if (ResultData != nullptr)
    {
        TArray<FTargetingDefaultResultData> Expanded;
        TSet<const AActor*> VisitedActors;
        TInlineComponentArray<UKataTargetPointComponent*> Points;
        for (const FTargetingDefaultResultData& Result : ResultData->TargetResults)
        {
            const AActor* Actor = Result.HitResult.GetActor();
            // 수집 방식에 따라 같은 액터가 여러 번 나올 수 있으므로 액터마다 한 번만 펼친다.
            if (Actor == nullptr || VisitedActors.Contains(Actor))
            {
                continue;
            }
            VisitedActors.Add(Actor);

            Actor->GetComponents(Points);
            for (UKataTargetPointComponent* Point : Points)
            {
                if (!IsValid(Point) || !Point->IsTargetPointEnabled() || !Point->RoleTags.HasAll(RequiredRoleTags))
                {
                    continue;
                }

                FTargetingDefaultResultData& PointResult = Expanded.Add_GetRef(Result);
                const FVector PointLocation = Point->GetComponentLocation();
                PointResult.HitResult.Component = Point;
                PointResult.HitResult.Location = PointLocation;
                PointResult.HitResult.ImpactPoint = PointLocation;
            }
        }
        ResultData->TargetResults = MoveTemp(Expanded);
    }

    SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
}
