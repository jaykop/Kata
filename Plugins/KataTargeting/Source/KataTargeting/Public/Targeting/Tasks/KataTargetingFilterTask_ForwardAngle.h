#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "KataTargetingFilterTask_ForwardAngle.generated.h"

/**
 * 실행 주체의 정면(액터 Forward Vector)과 후보 방향 사이의 끼인각이 MaxAngle 이하인 후보만 남기는 Targeting 필터.
 *
 * 위에서 내려다본 수평면에서 각도를 재므로 후보의 높이 차이는 보지 않는다. 카메라 방향과 관계없이 몸이 향한 방향이 기준이다.
 * 후보가 실행 주체와 수평 위치가 같으면 방향을 정할 수 없으므로 남긴다. 소스 Context가 없으면 거르지 않는다.
 */
UCLASS(meta = (DisplayName = "Kata Filter Forward Angle"))
class KATATARGETING_API UKataTargetingFilterTask_ForwardAngle : public UTargetingFilterTask_BasicFilterTemplate
{
    GENERATED_BODY()

public:
    UKataTargetingFilterTask_ForwardAngle(const FObjectInitializer& ObjectInitializer);

    /** 정면에서 한쪽으로 허용하는 최대 각도(도). 디버그 표시가 부채꼴을 그릴 때 읽는다. */
    float GetMaxAngle() const { return MaxAngle; }

protected:
    virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

    /** 정면에서 한쪽으로 허용하는 최대 각도. 60이면 정면 기준 좌우 60도, 모두 120도 부채꼴 안의 후보만 남긴다. 180이면 거르지 않는다. */
    UPROPERTY(EditAnywhere, Category = "Kata|Targeting", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "Degrees"))
    float MaxAngle = 60.0f;
};
