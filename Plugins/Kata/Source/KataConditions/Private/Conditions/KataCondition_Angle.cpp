#include "Conditions/KataCondition_Angle.h"

#include "FunctionLibraries/KataFL_Condition.h"

FName UKataCondition_Angle::GetConfigurationError() const
{
    if (Space != EKataConditionSpace::Plane2D && Space != EKataConditionSpace::Spatial3D)
    {
        return TEXT("InvalidSpace");
    }
    if (!FMath::IsFinite(HalfAngleDegrees) || HalfAngleDegrees < 0.0f || HalfAngleDegrees > 180.0f)
    {
        return TEXT("InvalidHalfAngle");
    }
    return FMath::IsFinite(YawOffsetDegrees) ? NAME_None : FName(TEXT("NonFiniteYawOffset"));
}

FKataConditionResult UKataCondition_Angle::EvaluateCondition_Implementation(const FKataConditionContext& Context) const
{
    FName Error;
    const bool bMatches = UKataFL_Condition::CheckAngle(
        Context.GetActor(EKataConditionSubject::Self),
        Context.GetActor(EKataConditionSubject::Target),
        Space,
        HalfAngleDegrees,
        YawOffsetDegrees,
        Error);
    return Error.IsNone() ? FKataConditionResult::FromBool(bMatches) : FKataConditionResult::Invalid(Error);
}
