#include "Conditions/KataCondition_Tag.h"

#include "AbilitySystemComponent.h"

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
    const UAbilitySystemComponent* ASC = Context.GetAbilitySystem(Subject);
    if (!IsValid(ASC))
    {
        return FKataConditionResult::Invalid(TEXT("MissingAbilitySystem"));
    }

    const FGameplayTagContainer& OwnedTags = ASC->GetOwnedGameplayTags();
    const bool bMatches = MatchMode == EKataTagMatchMode::Any
        ? (bExactMatch ? OwnedTags.HasAnyExact(Tags) : OwnedTags.HasAny(Tags))
        : (bExactMatch ? OwnedTags.HasAllExact(Tags) : OwnedTags.HasAll(Tags));
    return FKataConditionResult::FromBool(bMatches);
}
