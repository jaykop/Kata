#include "KataConditionTypes.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"

AActor* FKataConditionContext::GetActor(EKataConditionSubject Subject) const
{
    switch (Subject)
    {
    case EKataConditionSubject::Self: return SelfActor.Get();
    case EKataConditionSubject::Target: return TargetActor.Get();
    default: return nullptr;
    }
}

UAbilitySystemComponent* FKataConditionContext::GetAbilitySystem(EKataConditionSubject Subject) const
{
    UAbilitySystemComponent* Override = nullptr;
    switch (Subject)
    {
    case EKataConditionSubject::Self: Override = SelfAbilitySystem.Get(); break;
    case EKataConditionSubject::Target: Override = TargetAbilitySystem.Get(); break;
    default: return nullptr;
    }

    if (IsValid(Override))
    {
        return Override;
    }

    const AActor* Actor = GetActor(Subject);
    return IsValid(Actor) ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor) : nullptr;
}

FKataConditionResult FKataConditionResult::FromBool(bool bSatisfied)
{
    FKataConditionResult Result;
    Result.Status = bSatisfied ? EKataConditionStatus::Pass : EKataConditionStatus::Fail;
    Result.Reason = bSatisfied ? NAME_None : FName(TEXT("ConditionNotMet"));
    return Result;
}

FKataConditionResult FKataConditionResult::Invalid(FName InReason)
{
    FKataConditionResult Result;
    Result.Reason = InReason;
    return Result;
}
