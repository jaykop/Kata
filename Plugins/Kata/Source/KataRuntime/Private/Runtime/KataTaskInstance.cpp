#include "Runtime/KataTaskInstance.h"

#include "Action/KataTask.h"
#include "Runtime/KataActionInstance.h"

UWorld* UKataTaskInstance::GetWorld() const
{
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        return nullptr;
    }
    return ActionInstance != nullptr ? ActionInstance->GetWorld() : nullptr;
}

UKataActionInstance* UKataTaskInstance::GetActionInstance() const
{
    return ActionInstance;
}

UKataTask* UKataTaskInstance::GetTaskDefinition() const
{
    return TaskDefinition;
}

void UKataTaskInstance::InitializeTaskInstance(UKataActionInstance* InActionInstance, UKataTask* InTaskDefinition)
{
    ActionInstance = InActionInstance;
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

void UKataTaskInstance::ResetForExecution()
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

    if (ActionInstance != nullptr)
    {
        // 완료 의존성 재확인은 소유 인스턴스가 담당한다.
        ActionInstance->HandleTaskFinished(this, EKataTaskEndReason::Completed);
        return;
    }

    EndTask(EKataTaskEndReason::Completed);
}

FKataContext UKataTaskInstance::GetKataContext() const
{
    return ActionInstance != nullptr ? ActionInstance->GetContext() : FKataContext();
}
