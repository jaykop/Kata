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

    /** Maximum deviation from the adjusted forward direction. 45 means a 90-degree total cone. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angle", meta = (ClampMin = "0", ClampMax = "180", Units = "deg"))
    float HalfAngleDegrees = 45.0f;

    /** Positive turns forward toward the actor's right. Rotates the direction, not the origin. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Angle", meta = (UIMin = "-180", UIMax = "180", Units = "deg"))
    float YawOffsetDegrees = 0.0f;

protected:
    virtual FKataConditionResult EvaluateCondition_Implementation(const FKataConditionContext& Context) const override;
    virtual FName GetConfigurationError() const override;
};
