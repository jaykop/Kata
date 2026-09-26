#include "HitTrace/KataHitBoxPreset.h"

#include "CollisionShape.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

void UKataHitBoxPreset::GetRequiredSockets(TArray<FName, TInlineAllocator<8>>& OutSockets) const
{
    OutSockets.Reset();
    if (Mode == EKataHitBoxMode::SocketTrace)
    {
        OutSockets.Append(Sockets);
    }
    else
    {
        OutSockets.Add(Socket);
    }
}

FTransform UKataHitBoxPreset::MakeShapeTransform(const FTransform& SocketTransform) const
{
    FTransform Result = RelativeTransform * SocketTransform;
    Result.SetScale3D(FVector::OneVector);
    return Result;
}

FCollisionShape UKataHitBoxPreset::MakeCollisionShape() const
{
    if (Mode == EKataHitBoxMode::SocketTrace)
    {
        return FCollisionShape();
    }

    switch (Shape)
    {
    case EKataHitBoxShape::Capsule:
        return FCollisionShape::MakeCapsule(Radius, FMath::Max(Radius, CapsuleHalfHeight));
    case EKataHitBoxShape::Box:
        return FCollisionShape::MakeBox(BoxExtent.ComponentMax(FVector(0.1f)));
    case EKataHitBoxShape::Sphere:
    default:
        return FCollisionShape::MakeSphere(Radius);
    }
}

bool UKataHitBoxPreset::MatchesHurtBoxTags(const FGameplayTagContainer& Tags) const
{
    // 빈 FGameplayTagQuery의 Matches는 false를 돌려주므로 "조건 없음"을 따로 처리한다.
    return HurtBoxTagQuery.IsEmpty() || HurtBoxTagQuery.Matches(Tags);
}

FString UKataHitBoxPreset::GetConfigurationError() const
{
    if (Mode == EKataHitBoxMode::SocketTrace)
    {
        if (Sockets.Num() < 2)
        {
            return TEXT("Socket Trace needs at least two 'Sockets' along the blade");
        }
        for (int32 Index = 0; Index < Sockets.Num(); ++Index)
        {
            if (Sockets[Index].IsNone())
            {
                return FString::Printf(TEXT("Socket Trace 'Sockets' entry %d is empty"), Index);
            }
            // 이웃한 두 소켓이 같으면 그 구간의 판정 면이 선으로 퇴화한다.
            if (Index > 0 && Sockets[Index] == Sockets[Index - 1])
            {
                return FString::Printf(TEXT("Socket Trace 'Sockets' entries %d and %d are the same socket"), Index - 1, Index);
            }
        }
    }
    else if (Socket.IsNone())
    {
        return TEXT("Shape Sweep needs a 'Socket'");
    }
    return FString();
}

#if WITH_EDITOR
EDataValidationResult UKataHitBoxPreset::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    const FString Error = GetConfigurationError();
    if (!Error.IsEmpty())
    {
        Context.AddError(FText::FromString(Error));
        Result = EDataValidationResult::Invalid;
    }
    return Result;
}
#endif
