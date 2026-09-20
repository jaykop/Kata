#include "Runtime/KataTaskInstance.h"

#include "Definition/KataTask.h"
#include "Runtime/KataInstance.h"

UWorld* UKataTaskInstance::GetWorld() const
{
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        return nullptr;
    }
    return KataInstance != nullptr ? KataInstance->GetWorld() : nullptr;
}

UKataInstance* UKataTaskInstance::GetKataInstance() const
{
    return KataInstance;
}

UKataTask* UKataTaskInstance::GetTaskDefinition() const
{
    return TaskDefinition;
}

void UKataTaskInstance::InitializeTaskInstance(UKataInstance* InKataInstance, UKataTask* InTaskDefinition)
{
    KataInstance = InKataInstance;
    TaskDefinition = InTaskDefinition;
    TaskState = EKataTaskState::Pending;
    StartedAtKataTime = 0.0f;
    bEndHandled = false;
}

void UKataTaskInstance::BeginTask(float InKataTime)
{
    if (TaskState == EKataTaskState::Running || TaskState == EKataTaskState::Finished)
    {
        return;
    }

    TaskState = EKataTaskState::Running;
    StartedAtKataTime = InKataTime;
    bEndHandled = false;

    OnTaskStarted();
}

void UKataTaskInstance::TickTask(float DeltaTime, float InKataTime)
{
    if (TaskState != EKataTaskState::Running)
    {
        return;
    }
    OnTaskTick(DeltaTime);
}

void UKataTaskInstance::EndTask(EKataTaskEndReason Reason)
{
    if (bEndHandled)
    {
        return;
    }

    const bool bWasRunning = TaskState == EKataTaskState::Running;
    bEndHandled = true;
    TaskState = EKataTaskState::Finished;

    if (bWasRunning)
    {
        OnTaskEnded(Reason);
    }
}

void UKataTaskInstance::MarkWaitingForDependency()
{
    if (TaskState == EKataTaskState::Pending)
    {
        TaskState = EKataTaskState::WaitingForDependency;
    }
}

void UKataTaskInstance::MarkSkipped()
{
    if (TaskState == EKataTaskState::Running)
    {
        return;
    }
    TaskState = EKataTaskState::Skipped;
}

void UKataTaskInstance::ResetForLoop()
{
    TaskState = EKataTaskState::Pending;
    StartedAtKataTime = 0.0f;
    bEndHandled = false;
}

FString UKataTaskInstance::GetDisplayName() const
{
    return TaskDefinition != nullptr ? TaskDefinition->GetDisplayName() : GetClass()->GetName();
}

FKataTaskId UKataTaskInstance::GetTaskId() const
{
    return TaskDefinition != nullptr ? TaskDefinition->TaskId : FKataTaskId();
}

void UKataTaskInstance::OnTaskStarted_Implementation()
{
}

void UKataTaskInstance::OnTaskTick_Implementation(float DeltaTime)
{
}

void UKataTaskInstance::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
}

void UKataTaskInstance::FinishTask()
{
    if (TaskState != EKataTaskState::Running)
    {
        return;
    }

    if (KataInstance != nullptr)
    {
        // 완료 의존성 재확인은 소유 인스턴스가 담당한다.
        KataInstance->HandleTaskFinished(this, EKataTaskEndReason::Completed);
        return;
    }

    EndTask(EKataTaskEndReason::Completed);
}

FKataContext UKataTaskInstance::GetKataContext() const
{
    return KataInstance != nullptr ? KataInstance->GetContext() : FKataContext();
}
