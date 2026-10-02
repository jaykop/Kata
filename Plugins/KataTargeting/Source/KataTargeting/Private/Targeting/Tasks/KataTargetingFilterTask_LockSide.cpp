#include "Targeting/Tasks/KataTargetingFilterTask_LockSide.h"

#include "GameFramework/Actor.h"
#include "Targeting/KataPlayerTargetingComponent.h"
#include "Targeting/KataTargetPointComponent.h"
#include "Targeting/Tasks/KataTargetingViewUtils.h"

UKataTargetingFilterTask_LockSide::UKataTargetingFilterTask_LockSide(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

bool UKataTargetingFilterTask_LockSide::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
    FVector TargetLocation;
    if (!KataTargetingView::GetTargetLocation(TargetData, TargetLocation))
    {
        return true;
    }

    const AActor* SourceActor = KataTargetingView::GetSourceActor(TargetingHandle);
    const UKataPlayerTargetingComponent* Targeting =
        SourceActor != nullptr ? SourceActor->FindComponentByClass<UKataPlayerTargetingComponent>() : nullptr;
    const UKataTargetPointComponent* LockPoint = Targeting != nullptr ? Targeting->GetLockPoint() : nullptr;
    if (LockPoint == nullptr)
    {
        return false;
    }

    // 지점 결과면 현재 지점만 빼고 같은 액터의 다른 부위는 후보로 남긴다. 지점으로 펼치지 않은 액터 결과면 락온 중인 액터를 뺀다.
    const UKataTargetPointComponent* CandidatePoint = KataTargetingView::GetTargetPoint(TargetData);
    if (CandidatePoint != nullptr ? CandidatePoint == LockPoint : TargetData.HitResult.GetActor() == LockPoint->GetOwner())
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
    const FVector ToLock = (LockPoint->GetComponentLocation() - ViewLocation).GetSafeNormal2D();
    const FVector ToTarget = (TargetLocation - ViewLocation).GetSafeNormal2D();
    const double SideSign = FVector::CrossProduct(ToLock, ToTarget).Z;
    return Side == EKataLockSwitchSide::Right ? SideSign <= 0.0 : SideSign >= 0.0;
}
