#include "Conditions/KataCondition_Angle.h"

#include "GameFramework/Actor.h"

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
    const AActor* Self = Context.GetActor(EKataConditionSubject::Self);
    const AActor* Target = Context.GetActor(EKataConditionSubject::Target);
    if (!IsValid(Self))
    {
        return FKataConditionResult::Invalid(TEXT("MissingSelfActor"));
    }
    if (!IsValid(Target))
    {
        return FKataConditionResult::Invalid(TEXT("MissingTargetActor"));
    }

    FVector Forward = Self->GetActorForwardVector();
    FVector ToTarget = Target->GetActorLocation() - Self->GetActorLocation();
    FVector YawAxis = Self->GetActorUpVector();
    if (Forward.ContainsNaN() || ToTarget.ContainsNaN() || YawAxis.ContainsNaN())
    {
        return FKataConditionResult::Invalid(TEXT("NonFiniteDirection"));
    }

    if (Space == EKataConditionSpace::Plane2D)
    {
        Forward.Z = 0.0;
        ToTarget.Z = 0.0;
        YawAxis = FVector::UpVector;
    }
    if (!Forward.Normalize())
    {
        return FKataConditionResult::Invalid(TEXT("UndefinedForwardDirection"));
    }
    if (!ToTarget.Normalize())
    {
        return FKataConditionResult::Invalid(TEXT("UndefinedTargetDirection"));
    }

    const double OffsetRadians = FMath::DegreesToRadians(FMath::Fmod(static_cast<double>(YawOffsetDegrees), 360.0));
    Forward = FQuat(YawAxis, OffsetRadians).RotateVector(Forward);
    const double Dot = FMath::Clamp(FVector::DotProduct(Forward, ToTarget), -1.0, 1.0);
    const double AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));

    // 부동소수점 계산 오차만 보정하며 게임플레이용 판정 범위 조정값으로 사용하지 않는다.
    constexpr double BoundaryToleranceDegrees = 0.0001;
    return FKataConditionResult::FromBool(AngleDegrees <= HalfAngleDegrees + BoundaryToleranceDegrees);
}
