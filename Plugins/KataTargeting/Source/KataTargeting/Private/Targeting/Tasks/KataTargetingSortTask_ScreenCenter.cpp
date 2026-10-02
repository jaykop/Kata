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
    FVector TargetLocation;
    FVector ViewLocation;
    FRotator ViewRotation;
    if (!KataTargetingView::GetTargetLocation(TargetData, TargetLocation)
        || !KataTargetingView::GetViewPoint(KataTargetingView::GetSourceActor(TargetingHandle), ViewLocation, ViewRotation))
    {
        // 판정할 수 없는 후보는 가장 뒤로 보낸다.
        return 180.0f;
    }

    // 타겟 지점 결과면 지점 위치로 판정해 같은 액터의 부위끼리도 화면 중심 순서가 갈린다.
    const FVector ToTarget = (TargetLocation - ViewLocation).GetSafeNormal();
    const double CosAngle = FVector::DotProduct(ViewRotation.Vector(), ToTarget);
    return static_cast<float>(FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(CosAngle, -1.0, 1.0))));
}
