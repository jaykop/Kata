#include "Targeting/Tasks/KataTargetingFilterTask_ForwardAngle.h"

#include "GameFramework/Actor.h"
#include "Targeting/Tasks/KataTargetingViewUtils.h"

UKataTargetingFilterTask_ForwardAngle::UKataTargetingFilterTask_ForwardAngle(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

bool UKataTargetingFilterTask_ForwardAngle::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
    FVector TargetLocation;
    if (!KataTargetingView::GetTargetLocation(TargetData, TargetLocation))
    {
        return true;
    }

    const AActor* SourceActor = KataTargetingView::GetSourceActor(TargetingHandle);
    if (SourceActor == nullptr)
    {
        return false;
    }

    const FVector Forward = SourceActor->GetActorForwardVector().GetSafeNormal2D();
    const FVector ToTarget = (TargetLocation - SourceActor->GetActorLocation()).GetSafeNormal2D();
    if (Forward.IsZero() || ToTarget.IsZero())
    {
        return false;
    }

    // 각도 대신 코사인으로 비교해 acos 호출을 피한다. 끼인각이 작을수록 내적이 크다.
    const double MinDot = FMath::Cos(FMath::DegreesToRadians(static_cast<double>(MaxAngle)));
    return FVector::DotProduct(Forward, ToTarget) < MinDot;
}
