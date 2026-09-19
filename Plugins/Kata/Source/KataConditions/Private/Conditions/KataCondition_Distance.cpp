#include "Conditions/KataCondition_Distance.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
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

        // GetSocketLocation alone may silently return the component origin for a missing socket.
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
    if (!FMath::IsFinite(MinDistance) || !FMath::IsFinite(MaxDistance)
        || MinDistance < 0.0f || MaxDistance < MinDistance)
    {
        return TEXT("InvalidDistanceRange");
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

    const double Distance = Space == EKataConditionSpace::Plane2D
        ? FVector::Dist2D(SelfPoint, TargetPoint) : FVector::Dist(SelfPoint, TargetPoint);
    if (!FMath::IsFinite(Distance))
    {
        return FKataConditionResult::Invalid(TEXT("NonFiniteDistance"));
    }
    return FKataConditionResult::FromBool(Distance >= MinDistance && Distance <= MaxDistance);
}
