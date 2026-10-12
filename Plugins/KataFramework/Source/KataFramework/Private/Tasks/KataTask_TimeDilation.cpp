#include "Tasks/KataTask_TimeDilation.h"

#include "Engine/World.h"
#include "Runtime/KataActionInstance.h"
#include "Time/KataTimeDilationSubsystem.h"

UKataTask_TimeDilation::UKataTask_TimeDilation()
{
    // 배율은 구간 동안 걸려 있어야 의미가 있으므로 짧지 않은 기본 구간을 준다.
    Duration = 0.2f;
}

TSubclassOf<UKataTaskInstance> UKataTask_TimeDilation::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_TimeDilation::StaticClass();
}

FName UKataTask_TimeDilation::GetConfigurationError() const
{
    const FName SuperError = Super::GetConfigurationError();
    if (!SuperError.IsNone())
    {
        return SuperError;
    }
    if (bSingleFrame || !(Duration > 0.0f))
    {
        // 요청은 구간이 끝날 때 풀리므로 구간이 없으면 배율이 적용되는 시간도 없다.
        return TEXT("ZeroLengthTimeDilation");
    }
    return NAME_None;
}

FString UKataTask_TimeDilation::DescribeConfigurationError(FName ErrorCode) const
{
    if (ErrorCode == TEXT("ZeroLengthTimeDilation"))
    {
        return TEXT("Time Dilation needs a duration to hold the request; 'Single Frame' and zero 'Duration' are not allowed");
    }
    return Super::DescribeConfigurationError(ErrorCode);
}

void UKataTaskInstance_TimeDilation::OnTaskStarted_Implementation()
{
    const UKataTask_TimeDilation* Definition = Cast<UKataTask_TimeDilation>(GetTaskDefinition());
    const UKataActionInstance* OwningActionInstance = GetActionInstance();
    UWorld* World = GetWorld();
    UKataTimeDilationSubsystem* Subsystem = World != nullptr ? World->GetSubsystem<UKataTimeDilationSubsystem>() : nullptr;
    if (Definition == nullptr || OwningActionInstance == nullptr || Subsystem == nullptr)
    {
        FinishTask();
        return;
    }

    // 요청은 해제할 때까지 유지한다. 구간 끝은 Kata 시각으로 오므로 태스크 종료가 해제 시점이다.
    if (Definition->Target == EKataTimeDilationTarget::World)
    {
        RequestHandle = Subsystem->RequestWorldTimeDilation(Definition->TimeDilation, Definition->Priority);
    }
    else
    {
        // 실행기가 따르는 액터와 같은 액터에 걸어야 액션 타임라인과 몽타주가 함께 느려진다.
        RequestHandle = Subsystem->RequestActorTimeDilation(OwningActionInstance->GetTimeDilationActor(), Definition->TimeDilation, Definition->Priority);
    }
    TimeDilationSubsystem = Subsystem;
}

void UKataTaskInstance_TimeDilation::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    if (UKataTimeDilationSubsystem* Subsystem = TimeDilationSubsystem.Get())
    {
        Subsystem->ReleaseRequest(RequestHandle);
    }
    TimeDilationSubsystem.Reset();
    RequestHandle = INDEX_NONE;

    Super::OnTaskEnded_Implementation(Reason);
}
