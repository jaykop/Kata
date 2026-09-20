#pragma once

#include "CoreMinimal.h"
#include "KataCondition.h"
#include "KataCondition_Angle.generated.h"

UCLASS(meta = (DisplayName = "Kata Condition: Angle"))
class KATACONDITIONS_API UKataCondition_Angle : public UKataCondition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angle")
    EKataConditionSpace Space = EKataConditionSpace::Plane2D;

    /** 보정된 정면 방향에서 허용하는 최대 각도. 45도이면 전체 판정 범위는 90도다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angle", meta = (ClampMin = "0", ClampMax = "180", Units = "deg"))
    float HalfAngleDegrees = 45.0f;

    /** 양수는 정면을 액터의 오른쪽으로 회전시킨다. 기준 위치는 유지하고 방향만 변경한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angle", meta = (UIMin = "-180", UIMax = "180", Units = "deg"))
    float YawOffsetDegrees = 0.0f;

protected:
    virtual FKataConditionResult EvaluateCondition_Implementation(const FKataConditionContext& Context) const override;
    virtual FName GetConfigurationError() const override;
};
