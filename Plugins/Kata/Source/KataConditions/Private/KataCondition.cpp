#include "KataCondition.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FKataConditionResult UKataCondition::Evaluate(const FKataConditionContext& Context) const
{
    const FName ConfigurationError = GetConfigurationError();
    if (!ConfigurationError.IsNone())
    {
        return FKataConditionResult::Invalid(ConfigurationError);
    }

    FKataConditionResult Result = EvaluateCondition(Context);
    if (Result.Status == EKataConditionStatus::Invalid)
    {
        return Result;
    }
    if (Result.Status != EKataConditionStatus::Pass && Result.Status != EKataConditionStatus::Fail)
    {
        return FKataConditionResult::Invalid(TEXT("InvalidResultStatus"));
    }

    if (bInvert)
    {
        const bool bWasSatisfied = Result.IsSatisfied();
        Result.Status = bWasSatisfied ? EKataConditionStatus::Fail : EKataConditionStatus::Pass;
        Result.Reason = bWasSatisfied ? FName(TEXT("InvertedCondition")) : NAME_None;
    }
    return Result;
}

bool UKataCondition::IsSatisfied(const FKataConditionContext& Context) const
{
    return Evaluate(Context).IsSatisfied();
}

FKataConditionResult UKataCondition::EvaluateCondition_Implementation(const FKataConditionContext& Context) const
{
    return FKataConditionResult::Invalid(TEXT("NotImplemented"));
}

FName UKataCondition::GetConfigurationError() const
{
    return NAME_None;
}

#if WITH_EDITOR
EDataValidationResult UKataCondition::IsDataValid(FDataValidationContext& Context) const
{
    const EDataValidationResult SuperResult = Super::IsDataValid(Context);
    const FName Error = GetConfigurationError();
    if (!Error.IsNone())
    {
        Context.AddError(FText::Format(
            NSLOCTEXT("KataConditions", "InvalidConfiguration", "Invalid condition configuration: {0}"),
            FText::FromName(Error)));
        return EDataValidationResult::Invalid;
    }
    return SuperResult == EDataValidationResult::Invalid ? SuperResult : EDataValidationResult::Valid;
}
#endif
