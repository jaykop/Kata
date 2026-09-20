#include "FunctionLibraries/KataFL_Condition.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"

FName UKataFL_Condition::ValidateComparison(EKataNumericComparison Comparison, double EqualityTolerance)
{
    switch (Comparison)
    {
    case EKataNumericComparison::LessThan:
    case EKataNumericComparison::LessOrEqual:
    case EKataNumericComparison::GreaterThan:
    case EKataNumericComparison::GreaterOrEqual:
        return NAME_None;
    case EKataNumericComparison::Equal:
    case EKataNumericComparison::NotEqual:
        return FMath::IsFinite(EqualityTolerance) && EqualityTolerance >= 0.0
            ? NAME_None : FName(TEXT("InvalidEqualityTolerance"));
    default:
        return TEXT("InvalidComparison");
    }
}

bool UKataFL_Condition::CompareValue(
    double Value,
    double ReferenceValue,
    EKataNumericComparison Comparison,
    double EqualityTolerance,
    FName& OutError)
{
    OutError = NAME_None;
    if (!FMath::IsFinite(Value))
    {
        OutError = TEXT("NonFiniteValue");
        return false;
    }
    if (!FMath::IsFinite(ReferenceValue))
    {
        OutError = TEXT("NonFiniteCompareValue");
        return false;
    }

    OutError = ValidateComparison(Comparison, EqualityTolerance);
    if (!OutError.IsNone())
    {
        return false;
    }

    switch (Comparison)
    {
    case EKataNumericComparison::LessThan: return Value < ReferenceValue;
    case EKataNumericComparison::LessOrEqual: return Value <= ReferenceValue;
    case EKataNumericComparison::GreaterThan: return Value > ReferenceValue;
    case EKataNumericComparison::GreaterOrEqual: return Value >= ReferenceValue;
    case EKataNumericComparison::Equal: return FMath::Abs(Value - ReferenceValue) <= EqualityTolerance;
    case EKataNumericComparison::NotEqual: return FMath::Abs(Value - ReferenceValue) > EqualityTolerance;
    default:
        OutError = TEXT("InvalidComparison");
        return false;
    }
}

bool UKataFL_Condition::CheckDistance(
    const FVector& FirstLocation,
    const FVector& SecondLocation,
    EKataConditionSpace Space,
    double ReferenceDistance,
    EKataNumericComparison Comparison,
    double EqualityTolerance,
    FName& OutError)
{
    OutError = NAME_None;
    if (Space != EKataConditionSpace::Plane2D && Space != EKataConditionSpace::Spatial3D)
    {
        OutError = TEXT("InvalidSpace");
        return false;
    }
    if (FirstLocation.ContainsNaN() || SecondLocation.ContainsNaN())
    {
        OutError = TEXT("NonFiniteLocation");
        return false;
    }
    if (!FMath::IsFinite(ReferenceDistance) || ReferenceDistance < 0.0)
    {
        OutError = TEXT("InvalidCompareDistance");
        return false;
    }

    const double Distance = Space == EKataConditionSpace::Plane2D
        ? FVector::Dist2D(FirstLocation, SecondLocation)
        : FVector::Dist(FirstLocation, SecondLocation);
    if (!FMath::IsFinite(Distance))
    {
        OutError = TEXT("NonFiniteDistance");
        return false;
    }
    return CompareValue(Distance, ReferenceDistance, Comparison, EqualityTolerance, OutError);
}

bool UKataFL_Condition::CheckAngle(
    AActor* SourceActor,
    AActor* TargetActor,
    EKataConditionSpace Space,
    double HalfAngleDegrees,
    double YawOffsetDegrees,
    FName& OutError)
{
    OutError = NAME_None;
    if (Space != EKataConditionSpace::Plane2D && Space != EKataConditionSpace::Spatial3D)
    {
        OutError = TEXT("InvalidSpace");
        return false;
    }
    if (!FMath::IsFinite(HalfAngleDegrees) || HalfAngleDegrees < 0.0 || HalfAngleDegrees > 180.0)
    {
        OutError = TEXT("InvalidHalfAngle");
        return false;
    }
    if (!FMath::IsFinite(YawOffsetDegrees))
    {
        OutError = TEXT("NonFiniteYawOffset");
        return false;
    }
    if (!IsValid(SourceActor))
    {
        OutError = TEXT("MissingSelfActor");
        return false;
    }
    if (!IsValid(TargetActor))
    {
        OutError = TEXT("MissingTargetActor");
        return false;
    }

    FVector Forward = SourceActor->GetActorForwardVector();
    FVector ToTarget = TargetActor->GetActorLocation() - SourceActor->GetActorLocation();
    FVector YawAxis = SourceActor->GetActorUpVector();
    if (Forward.ContainsNaN() || ToTarget.ContainsNaN() || YawAxis.ContainsNaN())
    {
        OutError = TEXT("NonFiniteDirection");
        return false;
    }

    if (Space == EKataConditionSpace::Plane2D)
    {
        Forward.Z = 0.0;
        ToTarget.Z = 0.0;
        YawAxis = FVector::UpVector;
    }
    if (!Forward.Normalize())
    {
        OutError = TEXT("UndefinedForwardDirection");
        return false;
    }
    if (!ToTarget.Normalize())
    {
        OutError = TEXT("UndefinedTargetDirection");
        return false;
    }

    const double OffsetRadians = FMath::DegreesToRadians(FMath::Fmod(YawOffsetDegrees, 360.0));
    Forward = FQuat(YawAxis, OffsetRadians).RotateVector(Forward);
    const double Dot = FMath::Clamp(FVector::DotProduct(Forward, ToTarget), -1.0, 1.0);
    const double AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));

    // 부동소수점 계산 오차만 보정하며 게임플레이용 판정 범위 조정값으로 사용하지 않는다.
    constexpr double BoundaryToleranceDegrees = 0.0001;
    return AngleDegrees <= HalfAngleDegrees + BoundaryToleranceDegrees;
}

bool UKataFL_Condition::CheckTag(
    UAbilitySystemComponent* AbilitySystem,
    const FGameplayTagContainer& Tags,
    EKataTagMatchMode MatchMode,
    bool bExactMatch,
    FName& OutError)
{
    OutError = NAME_None;
    if (MatchMode != EKataTagMatchMode::Any && MatchMode != EKataTagMatchMode::All)
    {
        OutError = TEXT("InvalidTagMatchMode");
        return false;
    }
    if (Tags.IsEmpty())
    {
        OutError = TEXT("EmptyTags");
        return false;
    }
    if (!IsValid(AbilitySystem))
    {
        OutError = TEXT("MissingAbilitySystem");
        return false;
    }

    const FGameplayTagContainer& OwnedTags = AbilitySystem->GetOwnedGameplayTags();
    return MatchMode == EKataTagMatchMode::Any
        ? (bExactMatch ? OwnedTags.HasAnyExact(Tags) : OwnedTags.HasAny(Tags))
        : (bExactMatch ? OwnedTags.HasAllExact(Tags) : OwnedTags.HasAll(Tags));
}
