#include "Targeting/Tasks/KataTargetingSortTask_ScreenCenter.h"

#include "GameFramework/Actor.h"
#include "Targeting/Tasks/KataTargetingViewUtils.h"
#include "Types/TargetingSystemTypes.h"

UKataTargetingSortTask_ScreenCenter::UKataTargetingSortTask_ScreenCenter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bHigherIsBetter = false;
}

float UKataTargetingSortTask_ScreenCenter::GetRawScore(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
    const AActor* Target = TargetData.HitResult.GetActor();
    FVector ViewLocation;
    FRotator ViewRotation;
    if (Target == nullptr || !KataTargetingView::GetViewPoint(KataTargetingView::GetSourceActor(TargetingHandle), ViewLocation, ViewRotation))
    {
        // 판정할 수 없는 후보는 가장 뒤로 보낸다.
        return 180.0f;
    }

    const FVector ToTarget = (Target->GetActorLocation() - ViewLocation).GetSafeNormal();
    const double CosAngle = FVector::DotProduct(ViewRotation.Vector(), ToTarget);
    return static_cast<float>(FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(CosAngle, -1.0, 1.0))));
}
