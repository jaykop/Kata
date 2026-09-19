#include "Conditions/KataCondition_Attribute.h"

#include "AbilitySystemComponent.h"

namespace KataAttributeCondition
{
    bool IsUsableAttribute(const FGameplayAttribute& Attribute)
    {
        // Reject arbitrary reflected properties and ASC system fields; this condition reads AttributeSets.
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
    switch (Comparison)
    {
    case EKataNumericComparison::LessThan:
    case EKataNumericComparison::LessOrEqual:
    case EKataNumericComparison::GreaterThan:
    case EKataNumericComparison::GreaterOrEqual:
        break;
    case EKataNumericComparison::Equal:
    case EKataNumericComparison::NotEqual:
        if (!FMath::IsFinite(EqualityTolerance) || EqualityTolerance < 0.0f)
        {
            return TEXT("InvalidEqualityTolerance");
        }
        break;
    default:
        return TEXT("InvalidComparison");
    }
    return NAME_None;
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

    bool bMatches = false;
    switch (Comparison)
    {
    case EKataNumericComparison::LessThan: bMatches = Value < CompareValue; break;
    case EKataNumericComparison::LessOrEqual: bMatches = Value <= CompareValue; break;
    case EKataNumericComparison::GreaterThan: bMatches = Value > CompareValue; break;
    case EKataNumericComparison::GreaterOrEqual: bMatches = Value >= CompareValue; break;
    case EKataNumericComparison::Equal: bMatches = FMath::Abs(Value - CompareValue) <= EqualityTolerance; break;
    case EKataNumericComparison::NotEqual: bMatches = FMath::Abs(Value - CompareValue) > EqualityTolerance; break;
    default: return FKataConditionResult::Invalid(TEXT("InvalidComparison"));
    }
    return FKataConditionResult::FromBool(bMatches);
}
