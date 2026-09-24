#pragma once

#include "CoreMinimal.h"
#include "KataCondition.h"
#include "KataCondition_Distance.generated.h"

/**
 * 거리 측정 기준점. SocketName이 비어 있으면 Actor 위치를 사용한다.
 * 값이 있으면 ACharacter::GetMesh()의 Socket 위치를 사용하며, 다른 컴포넌트는 선택할 수 없다.
 */
USTRUCT(BlueprintType)
struct KATACONDITIONS_API FKataConditionLocation
{
    GENERATED_BODY()

    /** 비어 있으면 Actor 위치를 사용한다. 지정하면 Actor가 ACharacter여야 하며 그 기본 Mesh에 Socket이 있어야 한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
    FName SocketName = NAME_None;
};

UCLASS(meta = (DisplayName = "Kata Condition: Distance"))
class KATACONDITIONS_API UKataCondition_Distance : public UKataCondition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
    FKataConditionLocation SelfLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
    FKataConditionLocation TargetLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance")
    EKataConditionSpace Space = EKataConditionSpace::Plane2D;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comparison")
    EKataNumericComparison Comparison = EKataNumericComparison::LessOrEqual;

    /** 계산한 거리를 이 기준 거리와 비교한다. 단위는 cm다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comparison", meta = (ClampMin = "0", Units = "cm"))
    float CompareDistance = 200.0f;

    /** Equal과 NotEqual에만 적용하는 거리 허용 오차. 단위는 cm다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comparison", meta = (ClampMin = "0", Units = "cm", EditCondition = "Comparison == EKataNumericComparison::Equal || Comparison == EKataNumericComparison::NotEqual", EditConditionHides))
    float EqualityTolerance = 1.0f;

protected:
    virtual FKataConditionResult EvaluateCondition_Implementation(const FKataConditionContext& Context) const override;
    virtual FName GetConfigurationError() const override;
};
