#include "Conditions/KataCondition_Group.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FName UKataCondition_Group::GetConfigurationError() const
{
    if (Mode != EKataConditionGroupMode::All && Mode != EKataConditionGroupMode::Any)
    {
        return TEXT("InvalidGroupMode");
    }
    if (Conditions.IsEmpty())
    {
        return TEXT("EmptyConditions");
    }

    for (const TObjectPtr<UKataCondition>& Child : Conditions)
    {
        const UKataCondition* Condition = Child.Get();
        if (!IsValid(Condition))
        {
            return TEXT("NullCondition");
        }
        if (Condition == this)
        {
            return TEXT("SelfReferencingCondition");
        }
    }
    return NAME_None;
}

FKataConditionResult UKataCondition_Group::EvaluateCondition_Implementation(const FKataConditionContext& Context) const
{
    // 자식 평가는 부작용이 없으므로 결과가 확정되는 즉시 중단한다.
    for (const TObjectPtr<UKataCondition>& Child : Conditions)
    {
        const UKataCondition* Condition = Child.Get();
        if (!IsValid(Condition) || Condition == this)
        {
            return FKataConditionResult::Invalid(TEXT("NullCondition"));
        }

        // Invert를 포함한 자식의 최종 판정을 얻기 위해 Evaluate 진입점을 사용한다.
        const FKataConditionResult ChildResult = Condition->Evaluate(Context);
        if (ChildResult.Status == EKataConditionStatus::Invalid)
        {
            return ChildResult;
        }

        if (Mode == EKataConditionGroupMode::All)
        {
            // 실패한 자식의 Reason을 그대로 전파해 진단에 사용한다.
            if (!ChildResult.IsSatisfied())
            {
                return ChildResult;
            }
        }
        else if (ChildResult.IsSatisfied())
        {
            return ChildResult;
        }
    }

    if (Mode == EKataConditionGroupMode::All)
    {
        return FKataConditionResult::FromBool(true);
    }

    FKataConditionResult Result;
    Result.Status = EKataConditionStatus::Fail;
    Result.Reason = TEXT("NoConditionSatisfied");
    return Result;
}

#if WITH_EDITOR
EDataValidationResult UKataCondition_Group::IsDataValid(FDataValidationContext& Context) const
{
    // 인라인 자식은 자동으로 검사되지 않으므로 직접 순회한다.
    EDataValidationResult Result = Super::IsDataValid(Context);
    for (const TObjectPtr<UKataCondition>& Child : Conditions)
    {
        const UKataCondition* Condition = Child.Get();
        if (!IsValid(Condition) || Condition == this)
        {
            continue;
        }
        if (Condition->IsDataValid(Context) == EDataValidationResult::Invalid)
        {
            Result = EDataValidationResult::Invalid;
        }
    }
    return Result;
}
#endif
