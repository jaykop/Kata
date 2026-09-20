#include "Conditions/KataCondition_Distance.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "FunctionLibraries/KataFL_Condition.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

namespace KataDistanceCondition
{
    FName ValidateLocation(const FKataConditionLocation& Location)
    {
        switch (Location.Mode)
        {
        case EKataLocationMode::ActorLocation: return NAME_None;
        case EKataLocationMode::Socket:
            return Location.SocketName.IsNone() ? FName(TEXT("EmptySocketName")) : NAME_None;
        default: return TEXT("InvalidLocationMode");
        }
    }

    FName ResolveLocation(const AActor* Actor, const FKataConditionLocation& Location, FVector& OutLocation)
    {
        if (Location.Mode == EKataLocationMode::ActorLocation)
        {
            OutLocation = Actor->GetActorLocation();
            return OutLocation.ContainsNaN() ? FName(TEXT("NonFiniteLocation")) : NAME_None;
        }

        const USceneComponent* SocketComponent = nullptr;
        if (Location.ComponentTag.IsNone())
        {
            const ACharacter* Character = Cast<ACharacter>(Actor);
            SocketComponent = Character ? Character->GetMesh() : nullptr;
        }
        else
        {
            TInlineComponentArray<USceneComponent*> Components;
            Actor->GetComponents(Components);
            for (const USceneComponent* Component : Components)
            {
                if (IsValid(Component) && Component->ComponentHasTag(Location.ComponentTag))
                {
                    if (SocketComponent)
                    {
                        return TEXT("AmbiguousComponentTag");
                    }
                    SocketComponent = Component;
                }
            }
        }

        if (!IsValid(SocketComponent))
        {
            return TEXT("MissingSocketComponent");
        }
        if (!SocketComponent->DoesSocketExist(Location.SocketName))
        {
            return TEXT("MissingSocket");
        }

        // GetSocketLocation만 호출하면 없는 Socket에 대해 컴포넌트 원점을 반환할 수 있다.
        OutLocation = SocketComponent->GetSocketLocation(Location.SocketName);
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
    const FName ComparisonError = UKataFL_Condition::ValidateComparison(Comparison, EqualityTolerance);
    if (!ComparisonError.IsNone())
    {
        return ComparisonError;
    }

    const FName SelfError = KataDistanceCondition::ValidateLocation(SelfLocation);
    return SelfError.IsNone() ? KataDistanceCondition::ValidateLocation(TargetLocation) : SelfError;
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
