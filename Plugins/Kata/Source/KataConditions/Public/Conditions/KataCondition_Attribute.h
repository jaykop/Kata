#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "KataCondition.h"
#include "KataCondition_Attribute.generated.h"

UENUM(BlueprintType)
enum class EKataAttributeMode : uint8
{
    Absolute,
    Ratio
};

UCLASS(meta = (DisplayName = "Kata Condition: Attribute"))
class KATACONDITIONS_API UKataCondition_Attribute : public UKataCondition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
    EKataConditionSubject Subject = EKataConditionSubject::Self;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
    EKataAttributeMode Mode = EKataAttributeMode::Absolute;

    /** Current GAS value in Absolute mode; numerator in Ratio mode. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
    FGameplayAttribute Attribute;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute", meta = (EditCondition = "Mode == EKataAttributeMode::Ratio", EditConditionHides))
    FGameplayAttribute MaxAttribute;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comparison")
    EKataNumericComparison Comparison = EKataNumericComparison::GreaterOrEqual;

    /** Ratios use 0.3 for 30 percent. Neither the value nor the threshold is clamped to [0, 1]. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comparison")
    float CompareValue = 0.0f;

    /** Used only for Equal/NotEqual. Ordering comparisons use their exact inclusive/exclusive operators. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comparison", meta = (ClampMin = "0", EditCondition = "Comparison == EKataNumericComparison::Equal || Comparison == EKataNumericComparison::NotEqual", EditConditionHides))
    float EqualityTolerance = 0.0001f;

protected:
    virtual FKataConditionResult EvaluateCondition_Implementation(const FKataConditionContext& Context) const override;
    virtual FName GetConfigurationError() const override;
};
