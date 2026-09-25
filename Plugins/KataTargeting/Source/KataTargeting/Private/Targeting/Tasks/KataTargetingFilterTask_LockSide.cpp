#include "Targeting/Tasks/KataTargetingFilterTask_LockSide.h"

#include "GameFramework/Actor.h"
#include "Targeting/KataPlayerTargetingComponent.h"
#include "Targeting/Tasks/KataTargetingViewUtils.h"

UKataTargetingFilterTask_LockSide::UKataTargetingFilterTask_LockSide(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

bool UKataTargetingFilterTask_LockSide::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
    const AActor* Target = TargetData.HitResult.GetActor();
    if (Target == nullptr)
    {
        return true;
    }

    const AActor* SourceActor = KataTargetingView::GetSourceActor(TargetingHandle);
    const UKataPlayerTargetingComponent* Targeting =
        SourceActor != nullptr ? SourceActor->FindComponentByClass<UKataPlayerTargetingComponent>() : nullptr;
    const AActor* LockTarget = Targeting != nullptr ? Targeting->GetLockTarget() : nullptr;
    if (LockTarget == nullptr)
    {
        return false;
    }
    if (Target == LockTarget)
    {
        return true;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    if (!KataTargetingView::GetViewPoint(SourceActor, ViewLocation, ViewRotation))
    {
        return false;
    }

    // UE는 X가 앞, Y가 오른쪽, Z가 위다. 락온 방향에서 후보 방향으로의 외적 Z가 양수이면 후보가 오른쪽에 있다.
    const FVector ToLock = (LockTarget->GetActorLocation() - ViewLocation).GetSafeNormal2D();
    const FVector ToTarget = (Target->GetActorLocation() - ViewLocation).GetSafeNormal2D();
    const double SideSign = FVector::CrossProduct(ToLock, ToTarget).Z;
    return Side == EKataLockSwitchSide::Right ? SideSign <= 0.0 : SideSign >= 0.0;
}
