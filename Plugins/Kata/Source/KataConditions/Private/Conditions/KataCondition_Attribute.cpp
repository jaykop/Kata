#include "Conditions/KataCondition_Attribute.h"

#include "AbilitySystemComponent.h"
#include "FunctionLibraries/KataFL_Condition.h"

namespace KataAttributeCondition
{
    bool IsUsableAttribute(const FGameplayAttribute& Attribute)
    {
        // AttributeSet을 읽는 조건이므로 임의의 리플렉션 프로퍼티와 ASC 시스템 필드는 거절한다.
        if (!Attribute.IsValid() || !FGameplayAttribute::IsSupportedProperty(Attribute.GetUProperty()))
        {
            return false;
        }
        const UClass* OwnerClass = Cast<UClass>(Attribute.GetUProperty()->GetOwnerStruct());
        return OwnerClass && OwnerClass->IsChildOf(UAttributeSet::StaticClass());
    }
}

FName UKataCondition_Attribute::GetConfigurationError() const
{
    if (Subject != EKataConditionSubject::Self && Subject != EKataConditionSubject::Target)
    {
        return TEXT("InvalidSubject");
    }
    if (Mode != EKataAttributeMode::Absolute && Mode != EKataAttributeMode::Ratio)
    {
        return TEXT("InvalidAttributeMode");
    }
    if (!KataAttributeCondition::IsUsableAttribute(Attribute))
    {
        return TEXT("InvalidAttribute");
    }
    if (Mode == EKataAttributeMode::Ratio && !KataAttributeCondition::IsUsableAttribute(MaxAttribute))
    {
        return TEXT("InvalidMaxAttribute");
    }
    if (!FMath::IsFinite(CompareValue))
    {
        return TEXT("NonFiniteCompareValue");
    }
    return UKataFL_Condition::ValidateComparison(Comparison, EqualityTolerance);
}

FKataConditionResult UKataCondition_Attribute::EvaluateCondition_Implementation(const FKataConditionContext& Context) const
{
    const UAbilitySystemComponent* ASC = Context.GetAbilitySystem(Subject);
    if (!IsValid(ASC))
    {
        return FKataConditionResult::Invalid(TEXT("MissingAbilitySystem"));
    }

    bool bFound = false;
    float Value = ASC->GetGameplayAttributeValue(Attribute, bFound);
    if (!bFound)
    {
        return FKataConditionResult::Invalid(TEXT("MissingAttribute"));
    }
    if (!FMath::IsFinite(Value))
    {
        return FKataConditionResult::Invalid(TEXT("NonFiniteAttribute"));
    }

    if (Mode == EKataAttributeMode::Ratio)
    {
        const float Maximum = ASC->GetGameplayAttributeValue(MaxAttribute, bFound);
        if (!bFound)
        {
            return FKataConditionResult::Invalid(TEXT("MissingMaxAttribute"));
        }
        if (!FMath::IsFinite(Maximum) || Maximum <= 0.0)
        {
            return FKataConditionResult::Invalid(TEXT("InvalidRatioMaximum"));
        }
        Value /= Maximum;
        if (!FMath::IsFinite(Value))
        {
            return FKataConditionResult::Invalid(TEXT("NonFiniteRatio"));
        }
    }

    FName Error;
    const bool bMatches = UKataFL_Condition::CompareValue(
        Value,
        CompareValue,
        Comparison,
        EqualityTolerance,
        Error);
    return Error.IsNone() ? FKataConditionResult::FromBool(bMatches) : FKataConditionResult::Invalid(Error);
}
