#include "Conditions/KataCondition_Tag.h"

#include "AbilitySystemComponent.h"
#include "FunctionLibraries/KataFL_Condition.h"

FName UKataCondition_Tag::GetConfigurationError() const
{
    if (Subject != EKataConditionSubject::Self && Subject != EKataConditionSubject::Target)
    {
        return TEXT("InvalidSubject");
    }
    if (MatchMode != EKataTagMatchMode::Any && MatchMode != EKataTagMatchMode::All)
    {
        return TEXT("InvalidTagMatchMode");
    }
    return Tags.IsEmpty() ? FName(TEXT("EmptyTags")) : NAME_None;
}

FKataConditionResult UKataCondition_Tag::EvaluateCondition_Implementation(const FKataConditionContext& Context) const
{
    FName Error;
    const bool bMatches = UKataFL_Condition::CheckTag(
        Context.GetAbilitySystem(Subject),
        Tags,
        MatchMode,
        bExactMatch,
        Error);
    return Error.IsNone() ? FKataConditionResult::FromBool(bMatches) : FKataConditionResult::Invalid(Error);
}
