#include "Conditions/KataCondition_Distance.h"

#include "Components/SkeletalMeshComponent.h"
#include "FunctionLibraries/KataFL_Condition.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

namespace KataDistanceCondition
{
    FName ResolveLocation(const AActor* Actor, const FKataConditionLocation& Location, FVector& OutLocation)
    {
        if (Location.SocketName.IsNone())
        {
            OutLocation = Actor->GetActorLocation();
            return OutLocation.ContainsNaN() ? FName(TEXT("NonFiniteLocation")) : NAME_None;
        }

        // 코어는 무기·장비 같은 부착 컴포넌트를 알지 않으므로 기준 컴포넌트는 캐릭터 기본 Mesh로 고정한다.
        const ACharacter* Character = Cast<ACharacter>(Actor);
        const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
        if (!IsValid(Mesh))
        {
            return TEXT("MissingCharacterMesh");
        }
        if (!Mesh->DoesSocketExist(Location.SocketName))
        {
            return TEXT("MissingSocket");
        }

        // GetSocketLocation만 호출하면 없는 Socket에 대해 컴포넌트 원점을 반환할 수 있다.
        OutLocation = Mesh->GetSocketLocation(Location.SocketName);
        return OutLocation.ContainsNaN() ? FName(TEXT("NonFiniteLocation")) : NAME_None;
    }
}

FName UKataCondition_Distance::GetConfigurationError() const
{
    if (Space != EKataConditionSpace::Plane2D && Space != EKataConditionSpace::Spatial3D)
    {
        return TEXT("InvalidSpace");
    }
    if (!FMath::IsFinite(CompareDistance) || CompareDistance < 0.0f)
    {
        return TEXT("InvalidCompareDistance");
    }
    return UKataFL_Condition::ValidateComparison(Comparison, EqualityTolerance);
}

FKataConditionResult UKataCondition_Distance::EvaluateCondition_Implementation(const FKataConditionContext& Context) const
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

    FVector SelfPoint;
    FVector TargetPoint;
    const FName SelfError = KataDistanceCondition::ResolveLocation(Self, SelfLocation, SelfPoint);
    if (!SelfError.IsNone())
    {
        return FKataConditionResult::Invalid(SelfError);
    }
    const FName TargetError = KataDistanceCondition::ResolveLocation(Target, TargetLocation, TargetPoint);
    if (!TargetError.IsNone())
    {
        return FKataConditionResult::Invalid(TargetError);
    }

    FName Error;
    const bool bMatches = UKataFL_Condition::CheckDistance(
        SelfPoint,
        TargetPoint,
        Space,
        CompareDistance,
        Comparison,
        EqualityTolerance,
        Error);
    return Error.IsNone() ? FKataConditionResult::FromBool(bMatches) : FKataConditionResult::Invalid(Error);
}
