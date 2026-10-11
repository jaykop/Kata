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

    /** Absolute 모드에서는 현재 GAS 값으로, Ratio 모드에서는 분자로 사용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
    FGameplayAttribute Attribute;

    /** Ratio 모드에서 분모로 사용할 Attribute. 현재 값이 0 이하이거나 조회할 수 없으면 Invalid를 반환한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute", meta = (EditCondition = "Mode == EKataAttributeMode::Ratio", EditConditionHides))
    FGameplayAttribute MaxAttribute;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comparison")
    EKataNumericComparison Comparison = EKataNumericComparison::GreaterOrEqual;

    /** 비교할 기준값. Ratio 모드에서 0.3은 30%를 뜻하며, 계산한 비율과 기준값을 0~1로 제한하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comparison")
    float CompareValue = 0.0f;

    /** Equal과 NotEqual에만 사용한다. 대소 비교는 각 연산자의 경계 포함 여부를 그대로 따른다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Comparison", meta = (ClampMin = "0", EditCondition = "Comparison == EKataNumericComparison::Equal || Comparison == EKataNumericComparison::NotEqual", EditConditionHides))
    float EqualityTolerance = 0.0001f;

protected:
    virtual FKataConditionResult EvaluateCondition_Implementation(const FKataConditionContext& Context) const override;
    virtual FName GetConfigurationError() const override;
};
