#include "Tasks/KataTask_AutoDash.h"

#include "Animation/KataRootMotionCurveComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Actor.h"
#include "Runtime/KataActionInstance.h"
#include "Targeting/KataTargetingComponent.h"

namespace
{
    /** 대상 위치 기준일 때 거리에서 뺄 대상 몸통 반지름. 루트가 캡슐이 아니면 0이다. */
    float GetTargetBodyRadius(const AActor& Target)
    {
        const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Target.GetRootComponent());
        return Capsule != nullptr ? Capsule->GetScaledCapsuleRadius() : 0.0f;
    }
}

UKataTask_AutoDash::UKataTask_AutoDash()
{
    // 거리 보정은 전진하는 구간을 차지해야 의미가 있다. 기본값으로 짧은 구간을 준다.
    Duration = 0.3f;
    Phase = EKataTaskPhase::Movement;
}

TSubclassOf<UKataTaskInstance> UKataTask_AutoDash::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_AutoDash::StaticClass();
}

FName UKataTask_AutoDash::GetConfigurationError() const
{
    const FName SuperError = Super::GetConfigurationError();
    if (!SuperError.IsNone())
    {
        return SuperError;
    }
    if (bSingleFrame || !(Duration > 0.0f))
    {
        // 보정 구간이 없으면 남은 이동량을 정할 수 없다.
        return TEXT("ZeroLengthAutoDash");
    }
    if (!(MaxDashDistance >= MinDashDistance))
    {
        return TEXT("InvalidAutoDashDistanceRange");
    }
    return NAME_None;
}

FString UKataTask_AutoDash::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("ZeroLengthAutoDash"))
    {
        return TEXT("Auto Dash needs a duration covering the forward root motion; 'Single Frame' and zero 'Duration' are not allowed");
    }
    if (ErrorCode == TEXT("InvalidAutoDashDistanceRange"))
    {
        return TEXT("'Max Dash Distance' must be greater than or equal to 'Min Dash Distance'");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

void UKataTaskInstance_AutoDash::OnTaskStarted_Implementation()
{
    const UKataTask_AutoDash* Definition = Cast<UKataTask_AutoDash>(GetTaskDefinition());
    const FKataContext Context = GetKataContext();
    AActor* Avatar = Context.GetAvatarActor();
    AActor* Target = Context.GetTargetActor();
    UKataRootMotionCurveComponent* Component = Avatar != nullptr ? Avatar->FindComponentByClass<UKataRootMotionCurveComponent>() : nullptr;
    if (Definition == nullptr || Component == nullptr || !IsValid(Target) || Target == Avatar)
    {
        FinishTask();
        return;
    }

    // 요청은 컴포넌트가 이동 갱신마다 호출하므로 액터와 컴포넌트를 약한 참조로 캡처한다.
    const TWeakObjectPtr<AActor> WeakTarget = Target;
    const TWeakObjectPtr<const UKataTargetingComponent> WeakTargeting = Avatar->FindComponentByClass<UKataTargetingComponent>();

    FKataRootMotionDistanceRequest Request;
    Request.ResolveTarget = [WeakTarget, WeakTargeting](FVector& OutLocation, float& OutTargetRadius)
    {
        AActor* TargetActor = WeakTarget.Get();
        if (!IsValid(TargetActor))
        {
            return false;
        }

        bool bIsTargetPoint = false;
        const UKataTargetingComponent* Targeting = WeakTargeting.Get();
        if (Targeting == nullptr || !Targeting->ResolveApproachLocation(TargetActor, OutLocation, bIsTargetPoint))
        {
            OutLocation = TargetActor->GetActorLocation();
            bIsTargetPoint = false;
        }

        // 부위 지점은 이미 몸통 바깥쪽에 있으므로 대상 반지름을 빼면 너무 일찍 멈춘다.
        OutTargetRadius = bIsTargetPoint ? 0.0f : GetTargetBodyRadius(*TargetActor);
        return true;
    };
    Request.Duration = Definition->Duration;
    Request.StopDistance = Definition->StopDistance;
    Request.MinDistance = Definition->MinDashDistance;
    Request.MaxDistance = Definition->MaxDashDistance;
    Request.bTrackTarget = Definition->bTrackTarget;

    RootMotionComponent = Component;
    CorrectionHandle = Component->BeginDistanceCorrection(MoveTemp(Request));
}

void UKataTaskInstance_AutoDash::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    if (UKataRootMotionCurveComponent* Component = RootMotionComponent.Get())
    {
        Component->EndDistanceCorrection(CorrectionHandle);
    }
    RootMotionComponent.Reset();
    CorrectionHandle = INDEX_NONE;

    Super::OnTaskEnded_Implementation(Reason);
}
