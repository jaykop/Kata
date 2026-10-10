#include "Conditions/KataCondition_MoveDirection.h"

#include "GameFramework/Actor.h"
#include "Targeting/KataTargetingComponent.h"

namespace
{
    EKataMoveDirection ClassifyMoveDirection(const AActor& Actor, const FVector& WorldDirection)
    {
        // 실행 주체의 Yaw만 기준으로 삼아 경사면에서 몸이 기울어도 구간이 바뀌지 않게 한다.
        const FRotator YawRotation(0.0f, Actor.GetActorRotation().Yaw, 0.0f);
        const FVector Local = YawRotation.UnrotateVector(WorldDirection);
        const float AngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
        if (FMath::Abs(AngleDegrees) <= 45.0f)
        {
            return EKataMoveDirection::Forward;
        }
        if (FMath::Abs(AngleDegrees) >= 135.0f)
        {
            return EKataMoveDirection::Backward;
        }
        // UE 좌표계에서 액터 기준 +Y가 오른쪽이다.
        return AngleDegrees > 0.0f ? EKataMoveDirection::Right : EKataMoveDirection::Left;
    }
}

FKataConditionResult UKataCondition_MoveDirection::EvaluateCondition_Implementation(const FKataConditionContext& Context) const
{
    const AActor* Actor = Context.GetActor(EKataConditionSubject::Self);
    const UKataTargetingComponent* Targeting = Actor != nullptr ? Actor->FindComponentByClass<UKataTargetingComponent>() : nullptr;
    if (Targeting == nullptr)
    {
        return FKataConditionResult::Invalid(TEXT("MissingTargetingComponent"));
    }

    FVector MoveDirection;
    EKataMoveDirection Current = EKataMoveDirection::None;
    if (Targeting->ResolveMoveDirection(MoveDirection))
    {
        // Blueprint 재정의가 수평이 아니거나 정규화되지 않은 벡터를 돌려줄 수 있으므로 다시 정리한다.
        MoveDirection.Z = 0.0f;
        if (MoveDirection.Normalize())
        {
            Current = bUnlockedInputIsForward && !Targeting->IsLockOnActive()
                ? EKataMoveDirection::Forward
                : ClassifyMoveDirection(*Actor, MoveDirection);
        }
    }
    return FKataConditionResult::FromBool(Current == Direction);
}
