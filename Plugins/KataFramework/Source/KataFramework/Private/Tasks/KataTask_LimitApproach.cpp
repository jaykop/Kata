#include "Tasks/KataTask_LimitApproach.h"

#include "Animation/KataRootMotionCurveComponent.h"
#include "GameFramework/Actor.h"
#include "HitTrace/KataHurtBoxComponent.h"
#include "Movement/KataFL_Approach.h"
#include "Runtime/KataActionInstance.h"
#include "TargetingSystem/TargetingPreset.h"

UKataTask_LimitApproach::UKataTask_LimitApproach()
{
    // 전진 제한은 공격 동작 전체처럼 구간을 차지해야 한다. 기본값으로 짧지 않은 구간을 준다.
    Duration = 0.5f;
    Phase = EKataTaskPhase::Movement;
}

TSubclassOf<UKataTaskInstance> UKataTask_LimitApproach::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_LimitApproach::StaticClass();
}

FName UKataTask_LimitApproach::GetConfigurationError() const
{
    const FName SuperError = Super::GetConfigurationError();
    if (!SuperError.IsNone())
    {
        return SuperError;
    }
    if (bSingleFrame || !(Duration > 0.0f))
    {
        // 제한은 이동이 일어나는 구간 동안 걸려 있어야 한다.
        return TEXT("ZeroLengthLimitApproach");
    }
    return NAME_None;
}

FString UKataTask_LimitApproach::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("ZeroLengthLimitApproach"))
    {
        return TEXT("Limit Approach needs a duration covering the movement to limit; 'Single Frame' and zero 'Duration' are not allowed");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

void UKataTaskInstance_LimitApproach::OnTaskStarted_Implementation()
{
    const UKataTask_LimitApproach* Definition = Cast<UKataTask_LimitApproach>(GetTaskDefinition());
    const FKataContext Context = GetKataContext();
    AActor* Avatar = Context.GetAvatarActor();
    AActor* Target = Context.GetTargetActor();
    UKataRootMotionCurveComponent* Component = Avatar != nullptr ? Avatar->FindComponentByClass<UKataRootMotionCurveComponent>() : nullptr;
    if (Definition == nullptr || Component == nullptr)
    {
        FinishTask();
        return;
    }

    TArray<TWeakObjectPtr<const UKataHurtBoxComponent>> HurtBoxes;
    KataFL::GatherApproachHurtBoxes(*Avatar, Target, Definition->SoftLockHurtBoxPreset, HurtBoxes);

    const TWeakObjectPtr<AActor> WeakTarget = Target != Avatar ? Target : nullptr;
    if (HurtBoxes.IsEmpty() && !WeakTarget.IsValid())
    {
        FinishTask();
        return;
    }

    FKataRootMotionApproachLimitRequest Request;
    Request.ResolveNearestPoint = [HurtBoxes = MoveTemp(HurtBoxes), WeakTarget](const FVector& SegmentStart, const FVector& SegmentEnd,
        FVector& OutPoint, FVector& OutSegmentPoint, float& OutDistance)
    {
        if (KataFL::FindClosestHurtBoxPoint(HurtBoxes, SegmentStart, SegmentEnd, OutPoint, OutSegmentPoint, OutDistance))
        {
            return true;
        }

        // 후보 HurtBox가 모두 사라졌거나 처음부터 없으면 대상 몸통(캡슐) 표면을 쓴다.
        const AActor* TargetActor = WeakTarget.Get();
        if (!IsValid(TargetActor))
        {
            return false;
        }
        OutDistance = KataFL::GetClosestBodyPointToSegment(*TargetActor, SegmentStart, SegmentEnd, OutPoint, OutSegmentPoint);
        return true;
    };
    Request.LimitDistance = Definition->LimitDistance;

    RootMotionComponent = Component;
    LimitHandle = Component->BeginApproachLimit(MoveTemp(Request));
}

void UKataTaskInstance_LimitApproach::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    if (UKataRootMotionCurveComponent* Component = RootMotionComponent.Get())
    {
        Component->EndApproachLimit(LimitHandle);
    }
    RootMotionComponent.Reset();
    LimitHandle = INDEX_NONE;

    Super::OnTaskEnded_Implementation(Reason);
}
