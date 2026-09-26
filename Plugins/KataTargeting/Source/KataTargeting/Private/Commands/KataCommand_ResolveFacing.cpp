#include "Commands/KataCommand_ResolveFacing.h"

#include "GameFramework/Actor.h"
#include "Runtime/KataActionInstance.h"
#include "Targeting/KataTargetingComponent.h"

void UKataCommand_ResolveFacing::Execute_Implementation(UKataActionInstance* Instance)
{
    if (Instance == nullptr)
    {
        return;
    }

    const FKataContext& Context = Instance->GetContextRef();
    AActor* Actor = Context.GetAvatarActor();
    const UKataTargetingComponent* Targeting = Actor != nullptr ? Actor->FindComponentByClass<UKataTargetingComponent>() : nullptr;
    if (Targeting == nullptr)
    {
        return;
    }

    FVector Direction;
    if (!Targeting->ResolveFacingDirection(Context.GetTargetActor(), Direction))
    {
        return;
    }

    // Blueprint 재정의가 수평이 아니거나 정규화되지 않은 벡터를 돌려줄 수 있으므로 다시 정리한다.
    Direction.Z = 0.0f;
    if (!Direction.Normalize())
    {
        return;
    }

    FRotator Rotation = Actor->GetActorRotation();
    Rotation.Yaw = Direction.Rotation().Yaw;
    Actor->SetActorRotation(Rotation);
}
