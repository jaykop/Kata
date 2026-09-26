#include "Tasks/KataTask_RotateToFacing.h"

#include "GameFramework/Actor.h"
#include "Runtime/KataActionInstance.h"
#include "Targeting/KataTargetingComponent.h"

UKataTask_RotateToFacing::UKataTask_RotateToFacing()
{
    // 회전은 구간을 차지해야 과정이 보인다. 기본값으로 짧은 구간을 준다.
    Duration = 0.2f;
}

TSubclassOf<UKataTaskInstance> UKataTask_RotateToFacing::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_RotateToFacing::StaticClass();
}

FName UKataTask_RotateToFacing::GetConfigurationError() const
{
    const FName SuperError = Super::GetConfigurationError();
    if (!SuperError.IsNone())
    {
        return SuperError;
    }
    if (bSingleFrame)
    {
        // 한 프레임 Tick은 DeltaTime이 0일 수 있어 전혀 돌지 않을 수 있다.
        return TEXT("SingleFrameRotation");
    }
    if (!(Duration > 0.0f))
    {
        return TEXT("ZeroLengthRotation");
    }
    if (!(RotationRate > 0.0f))
    {
        return TEXT("InvalidRotationRate");
    }
    return NAME_None;
}

FString UKataTask_RotateToFacing::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("SingleFrameRotation"))
    {
        return TEXT("'Single Frame' cannot be used for a rotation; give it a duration or use the Resolve Facing command");
    }
    if (ErrorCode == TEXT("ZeroLengthRotation"))
    {
        return TEXT("'Duration' must be greater than zero or the task never rotates; use the Resolve Facing command for an instant turn");
    }
    if (ErrorCode == TEXT("InvalidRotationRate"))
    {
        return TEXT("'Rotation Rate' must be greater than zero");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

void UKataTaskInstance_RotateToFacing::OnTaskStarted_Implementation()
{
    AActor* Actor = GetKataContext().GetAvatarActor();
    const UKataTargetingComponent* Component = Actor != nullptr ? Actor->FindComponentByClass<UKataTargetingComponent>() : nullptr;
    if (Component == nullptr)
    {
        FinishTask();
        return;
    }

    RotatingActor = Actor;
    Targeting = Component;
    bHasTargetYaw = false;

    const UKataTask_RotateToFacing* Definition = Cast<UKataTask_RotateToFacing>(GetTaskDefinition());
    const bool bUpdateEveryTick = Definition != nullptr && Definition->bUpdateDirectionEveryTick;

    // 고정 방향이면 지금 구한 방향이 전부다. 구하지 못하면 돌 곳이 없으므로 바로 끝낸다.
    if (!UpdateTargetYaw() && !bUpdateEveryTick)
    {
        FinishTask();
    }
}

void UKataTaskInstance_RotateToFacing::OnTaskTick_Implementation(float DeltaTime)
{
    AActor* Actor = RotatingActor.Get();
    const UKataTask_RotateToFacing* Definition = Cast<UKataTask_RotateToFacing>(GetTaskDefinition());
    if (Actor == nullptr || Definition == nullptr)
    {
        FinishTask();
        return;
    }

    if (Definition->bUpdateDirectionEveryTick)
    {
        UpdateTargetYaw();
    }
    if (!bHasTargetYaw || DeltaTime <= 0.0f)
    {
        return;
    }

    FRotator Rotation = Actor->GetActorRotation();
    Rotation.Yaw = FMath::FixedTurn(Rotation.Yaw, TargetYaw, Definition->RotationRate * DeltaTime);
    Actor->SetActorRotation(Rotation);

    // 방향을 따라가는 설정은 목표가 계속 바뀔 수 있으므로 구간 끝까지 유지한다.
    if (!Definition->bUpdateDirectionEveryTick
        && FMath::IsNearlyZero(FMath::FindDeltaAngleDegrees(Rotation.Yaw, TargetYaw), KINDA_SMALL_NUMBER))
    {
        FinishTask();
    }
}

void UKataTaskInstance_RotateToFacing::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    RotatingActor.Reset();
    Targeting.Reset();
    bHasTargetYaw = false;

    Super::OnTaskEnded_Implementation(Reason);
}

bool UKataTaskInstance_RotateToFacing::UpdateTargetYaw()
{
    const UKataTargetingComponent* Component = Targeting.Get();
    if (Component == nullptr)
    {
        return false;
    }

    FVector Direction;
    if (!Component->ResolveFacingDirection(GetKataContext().GetTargetActor(), Direction))
    {
        return false;
    }

    // Blueprint 재정의가 수평이 아니거나 정규화되지 않은 벡터를 돌려줄 수 있으므로 다시 정리한다.
    Direction.Z = 0.0f;
    if (!Direction.Normalize())
    {
        return false;
    }

    TargetYaw = Direction.Rotation().Yaw;
    bHasTargetYaw = true;
    return true;
}
