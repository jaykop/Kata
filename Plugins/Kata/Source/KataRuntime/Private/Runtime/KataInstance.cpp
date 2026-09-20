#include "Runtime/KataInstance.h"

#include "AbilitySystemComponent.h"
#include "Definition/KataResolvedDefinition.h"
#include "Definition/KataAsset.h"
#include "Definition/KataTask.h"
#include "GAS/KataGasBridge.h"
#include "GameFramework/Actor.h"
#include "KataRuntimeLog.h"
#include "Runtime/KataTaskInstance.h"

UKataAsset* UKataInstance::GetKataAsset() const
{
    return ResolvedDefinition ? ResolvedDefinition->SourceAsset.Get() : nullptr;
}

UWorld* UKataInstance::GetWorld() const
{
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        return nullptr;
    }
    if (const AActor* Owner = Context.GetOwnerActor())
    {
        return Owner->GetWorld();
    }
    return GetOuter() != nullptr ? GetOuter()->GetWorld() : nullptr;
}

EKataStartResult UKataInstance::InitializeInstance(UKataResolvedDefinition* InResolvedDefinition, const FKataContext& InContext)
{
    if (InResolvedDefinition == nullptr)
    {
        return EKataStartResult::InvalidDefinition;
    }
    if (InResolvedDefinition->HasErrors())
    {
        InResolvedDefinition->LogDiagnostics();
        return EKataStartResult::ResolveFailed;
    }
    if (!InContext.HasValidOwner())
    {
        return EKataStartResult::InvalidContext;
    }

    ResolvedDefinition = InResolvedDefinition;
    Context = InContext;
    Scheduler.Build(ResolvedDefinition);

    // 실행별 태스크 상태는 여기에서만 만든다. 공유 정의에는 아무것도 쓰지 않는다.
    TaskInstances.Reset(Scheduler.Num());
    for (const FKataScheduledTask& Scheduled : Scheduler.GetTasks())
    {
        UKataTask* Task = Scheduled.Task.Get();
        TSubclassOf<UKataTaskInstance> InstanceClass = Task != nullptr ? Task->GetTaskInstanceClass() : nullptr;
        if (InstanceClass == nullptr)
        {
            InstanceClass = UKataTaskInstance::StaticClass();
        }

        UKataTaskInstance* TaskInstance = NewObject<UKataTaskInstance>(this, InstanceClass);
        TaskInstance->InitializeTaskInstance(this, Task);
        TaskInstances.Add(TaskInstance);
    }

    TaskEndTimes.Init(0.0f, TaskInstances.Num());

    InstanceState = EKataInstanceState::Created;
    CurrentTime = 0.0f;
    LoopIteration = 0;
    return EKataStartResult::Started;
}

void UKataInstance::StartInstance()
{
    if (InstanceState != EKataInstanceState::Created)
    {
        return;
    }

    InstanceState = EKataInstanceState::Running;
    CurrentTime = 0.0f;
    LoopIteration = 0;

    ApplyGasActivationState();

    // 시각 0의 경계를 먼저 처리한다. 첫 프레임의 즉시 태스크가 누락되지 않게 한다.
    AdvanceTo(0.0f, 0.0f, true);

    if (InstanceState != EKataInstanceState::Running)
    {
        return;
    }
    if (bEndRequested)
    {
        EndInstance(PendingEndReason);
        return;
    }

    if (Scheduler.GetTimelineDuration() <= UE_KINDA_SMALL_NUMBER && !ShouldLoopAgain())
    {
        // 순간 태스크만 있는 타임라인은 시작과 같은 프레임에 끝난다.
        EndInstance(EKataEndReason::Completed);
    }
}

void UKataInstance::TickInstance(float DeltaTime)
{
    if (InstanceState != EKataInstanceState::Running)
    {
        return;
    }
    if (!Context.HasValidOwner())
    {
        EndInstance(EKataEndReason::OwnerInvalid);
        return;
    }
    if (!(DeltaTime > 0.0f))
    {
        return;
    }

    const float Duration = Scheduler.GetTimelineDuration();
    const int32 MaxIterationsPerTick = ResolvedDefinition != nullptr
        ? FMath::Max(1, ResolvedDefinition->LoopPolicy.MaxIterationsPerTick)
        : 1;

    float Remaining = DeltaTime;
    int32 LoopsThisTick = 0;

    // 큰 DeltaTime도 구간을 잘라 가며 모든 경계를 처리한다.
    while (InstanceState == EKataInstanceState::Running)
    {
        const float StepStart = CurrentTime;
        const float StepEnd = FMath::Min(CurrentTime + Remaining, Duration);

        if (StepEnd > StepStart)
        {
            AdvanceTo(StepStart, StepEnd, false);
            Remaining -= (StepEnd - StepStart);
        }
        CurrentTime = FMath::Max(StepStart, StepEnd);

        if (InstanceState != EKataInstanceState::Running)
        {
            break;
        }
        if (bEndRequested)
        {
            EndInstance(PendingEndReason);
            break;
        }
        if (CurrentTime < Duration - UE_KINDA_SMALL_NUMBER)
        {
            break;
        }

        if (!ShouldLoopAgain())
        {
            EndInstance(EKataEndReason::Completed);
            break;
        }

        ++LoopsThisTick;
        if (LoopsThisTick > MaxIterationsPerTick)
        {
            // 한 프레임에서 과도하게 반복하면 진행을 보장할 수 없으므로 종료한다.
            UE_LOG(LogKata, Warning, TEXT("Kata '%s' exceeded %d loop iterations in a single tick and was stopped"),
                ResolvedDefinition != nullptr && ResolvedDefinition->SourceClass != nullptr ? *ResolvedDefinition->SourceClass->GetName() : TEXT("None"),
                MaxIterationsPerTick);
            EndInstance(EKataEndReason::ContractError);
            break;
        }

        BeginNextLoop();

        if (Remaining <= UE_SMALL_NUMBER)
        {
            break;
        }
    }

    if (InstanceState == EKataInstanceState::Running)
    {
        TickActiveTasks(DeltaTime);
    }
}

void UKataInstance::AdvanceTo(float FromTime, float ToTime, bool bIncludeFromTime)
{
    TArray<FKataTimelineBoundary> Boundaries;
    Scheduler.CollectBoundaries(FromTime, ToTime, bIncludeFromTime, Boundaries);

    for (const FKataTimelineBoundary& Boundary : Boundaries)
    {
        if (InstanceState != EKataInstanceState::Running || bEndRequested)
        {
            break;
        }

        // 태스크 콜백이 현재 시각을 읽을 수 있도록 경계 시각까지 진행한 것으로 본다.
        CurrentTime = FMath::Max(CurrentTime, Boundary.Time);

        // 같은 시각에서는 기존 구간을 먼저 끝내고 새 구간을 시작한다.
        FinishElapsedTasks();

        if (Boundary.Type == EKataBoundaryType::Start)
        {
            TryStartTask(Boundary.TaskIndex);
        }
    }

    if (InstanceState == EKataInstanceState::Running && !bEndRequested)
    {
        // 경계 목록에 없는 실제 종료 시각은 구간 끝에서 처리한다.
        CurrentTime = FMath::Max(CurrentTime, ToTime);
        FinishElapsedTasks();
    }
}

void UKataInstance::FinishElapsedTasks()
{
    // 같은 시각에 끝나는 태스크는 실행 순서대로 처리한다.
    TArray<int32> Snapshot = ActiveTaskIndices;
    Snapshot.Sort();

    for (int32 TaskIndex : Snapshot)
    {
        if (InstanceState != EKataInstanceState::Running || bEndRequested)
        {
            break;
        }
        if (!ActiveTaskIndices.Contains(TaskIndex) || !TaskEndTimes.IsValidIndex(TaskIndex))
        {
            continue;
        }
        if (CurrentTime + UE_KINDA_SMALL_NUMBER < TaskEndTimes[TaskIndex])
        {
            continue;
        }
        if (TaskInstances.IsValidIndex(TaskIndex))
        {
            HandleTaskFinished(TaskInstances[TaskIndex], EKataTaskEndReason::Completed);
        }
    }
}

void UKataInstance::TryStartTask(int32 TaskIndex)
{
    if (!TaskInstances.IsValidIndex(TaskIndex))
    {
        return;
    }

    UKataTaskInstance* TaskInstance = TaskInstances[TaskIndex];
    if (!IsValid(TaskInstance) || ActiveTaskIndices.Contains(TaskIndex))
    {
        return;
    }
    if (TaskInstance->GetTaskState() == EKataTaskState::Running || TaskInstance->GetTaskState() == EKataTaskState::Finished)
    {
        return;
    }

    if (!AreCompletionPrerequisitesMet(TaskIndex))
    {
        // 완료 대기를 묵시적 성공으로 처리하지 않고 명시적으로 지연한다.
        DeferredTaskIndices.AddUnique(TaskIndex);
        TaskInstance->MarkWaitingForDependency();
        return;
    }

    StartTaskNow(TaskIndex);
}

void UKataInstance::StartTaskNow(int32 TaskIndex)
{
    if (!TaskInstances.IsValidIndex(TaskIndex))
    {
        return;
    }

    UKataTaskInstance* TaskInstance = TaskInstances[TaskIndex];
    if (!IsValid(TaskInstance))
    {
        return;
    }

    const TArray<FKataScheduledTask>& ScheduledTasks = Scheduler.GetTasks();
    if (!ScheduledTasks.IsValidIndex(TaskIndex))
    {
        return;
    }
    const FKataScheduledTask& Scheduled = ScheduledTasks[TaskIndex];

    // 의존성으로 늦게 시작해도 자신의 지속 시간만큼 실행하도록 실제 시작 시각을 기준으로 잡는다.
    const float TaskDuration = FMath::Max(0.0f, Scheduled.EndTime - Scheduled.StartTime);
    const float ActualEndTime = CurrentTime + TaskDuration;
    if (TaskEndTimes.IsValidIndex(TaskIndex))
    {
        TaskEndTimes[TaskIndex] = ActualEndTime;
    }

    if (!Scheduled.bInstant && ActualEndTime > Scheduler.GetTimelineDuration() + UE_KINDA_SMALL_NUMBER)
    {
        // 늦은 시작으로 타임라인 길이를 넘기면 Kata 종료 시점에 잘린다. 조용히 넘기지 않는다.
        UE_LOG(LogKata, Warning,
            TEXT("Kata task '%s' started late at %.3fs; its end %.3fs exceeds the timeline duration %.3fs and it will be cut short"),
            *TaskInstance->GetDisplayName(), CurrentTime, ActualEndTime, Scheduler.GetTimelineDuration());
    }

    DeferredTaskIndices.Remove(TaskIndex);
    ActiveTaskIndices.AddUnique(TaskIndex);
    TaskInstance->BeginTask(CurrentTime);

    if (Scheduled.bInstant && TaskInstance->IsRunning())
    {
        HandleTaskFinished(TaskInstance, EKataTaskEndReason::Completed);
    }
}

bool UKataInstance::AreCompletionPrerequisitesMet(int32 TaskIndex) const
{
    const TArray<FKataScheduledTask>& ScheduledTasks = Scheduler.GetTasks();
    if (!ScheduledTasks.IsValidIndex(TaskIndex))
    {
        return true;
    }

    for (int32 PrerequisiteIndex : ScheduledTasks[TaskIndex].CompletionPrerequisites)
    {
        if (!CompletedTaskIndices.Contains(PrerequisiteIndex))
        {
            return false;
        }
    }
    return true;
}

void UKataInstance::TryStartDeferredTasks()
{
    if (bResolvingDeferredTasks)
    {
        // 콜백 안에서 다시 들어온 경우다. 바깥 루프가 한 번 더 확인한다.
        bDeferredTasksDirty = true;
        return;
    }

    bResolvingDeferredTasks = true;

    do
    {
        bDeferredTasksDirty = false;

        const TArray<int32> Snapshot = DeferredTaskIndices;
        for (int32 TaskIndex : Snapshot)
        {
            if (InstanceState != EKataInstanceState::Running || bEndRequested)
            {
                break;
            }
            if (!DeferredTaskIndices.Contains(TaskIndex))
            {
                continue;
            }
            if (!AreCompletionPrerequisitesMet(TaskIndex))
            {
                continue;
            }
            StartTaskNow(TaskIndex);
        }
    }
    while (bDeferredTasksDirty && InstanceState == EKataInstanceState::Running);

    bResolvingDeferredTasks = false;
}

void UKataInstance::HandleTaskFinished(UKataTaskInstance* TaskInstance, EKataTaskEndReason Reason)
{
    if (!IsValid(TaskInstance))
    {
        return;
    }

    const int32 TaskIndex = TaskInstances.IndexOfByKey(TaskInstance);
    if (TaskIndex == INDEX_NONE)
    {
        return;
    }

    const int32 RemovedCount = ActiveTaskIndices.Remove(TaskIndex);
    if (RemovedCount == 0 && !TaskInstance->IsRunning())
    {
        return;
    }

    TaskInstance->EndTask(Reason);

    if (Reason == EKataTaskEndReason::Completed)
    {
        CompletedTaskIndices.Add(TaskIndex);
        TryStartDeferredTasks();
    }
}

void UKataInstance::TickActiveTasks(float DeltaTime)
{
    // 콜백이 활성 목록을 바꿀 수 있으므로 사본을 순회하고 매번 유효성을 다시 확인한다.
    const TArray<int32> Snapshot = ActiveTaskIndices;
    for (int32 TaskIndex : Snapshot)
    {
        if (InstanceState != EKataInstanceState::Running || bEndRequested)
        {
            break;
        }
        if (!ActiveTaskIndices.Contains(TaskIndex) || !TaskInstances.IsValidIndex(TaskIndex))
        {
            continue;
        }
        if (UKataTaskInstance* TaskInstance = TaskInstances[TaskIndex]; IsValid(TaskInstance))
        {
            TaskInstance->TickTask(DeltaTime, CurrentTime);
        }
    }

    if (bEndRequested && InstanceState == EKataInstanceState::Running)
    {
        EndInstance(PendingEndReason);
    }
}

void UKataInstance::BeginNextLoop()
{
    // 반복 경계에서는 남은 태스크를 끝내고 상태를 초기화한 뒤 시각 0으로 재진입한다.
    EndActiveTasks(EKataTaskEndReason::Interrupted);
    FlushDeferredTasks();
    CompletedTaskIndices.Reset();

    for (UKataTaskInstance* TaskInstance : TaskInstances)
    {
        if (IsValid(TaskInstance))
        {
            TaskInstance->ResetForLoop();
        }
    }
    TaskEndTimes.Init(0.0f, TaskInstances.Num());

    ++LoopIteration;
    CurrentTime = 0.0f;

    AdvanceTo(0.0f, 0.0f, true);
}

bool UKataInstance::ShouldLoopAgain() const
{
    if (ResolvedDefinition == nullptr || !ResolvedDefinition->LoopPolicy.bLoop)
    {
        return false;
    }

    const int32 MaxLoopCount = ResolvedDefinition->LoopPolicy.MaxLoopCount;
    if (MaxLoopCount <= 0)
    {
        // 0은 외부에서 멈출 때까지 반복한다.
        return true;
    }
    return (LoopIteration + 1) < MaxLoopCount;
}

void UKataInstance::RequestEnd(EKataEndReason Reason)
{
    if (InstanceState == EKataInstanceState::Ended)
    {
        return;
    }

    if (InstanceState == EKataInstanceState::Created)
    {
        EndInstance(Reason);
        return;
    }

    bEndRequested = true;
    PendingEndReason = Reason;

    // 경계 처리 중이 아니면 즉시 끝낸다. 처리 중이면 해당 루프가 안전한 지점에서 처리한다.
    if (!bResolvingDeferredTasks)
    {
        EndInstance(Reason);
    }
}

void UKataInstance::EndInstance(EKataEndReason Reason)
{
    if (InstanceState == EKataInstanceState::Ended)
    {
        return;
    }

    InstanceState = EKataInstanceState::Ended;
    bEndRequested = false;

    EndActiveTasks(EKataTaskEndReason::KataEnded);
    FlushDeferredTasks();
    RemoveGasActivationState();

    OnKataEnded.Broadcast(this, Reason);
}

void UKataInstance::EndActiveTasks(EKataTaskEndReason Reason)
{
    const TArray<int32> Snapshot = ActiveTaskIndices;
    ActiveTaskIndices.Reset();

    for (int32 TaskIndex : Snapshot)
    {
        if (!TaskInstances.IsValidIndex(TaskIndex))
        {
            continue;
        }
        if (UKataTaskInstance* TaskInstance = TaskInstances[TaskIndex]; IsValid(TaskInstance))
        {
            TaskInstance->EndTask(Reason);
        }
    }
}

void UKataInstance::FlushDeferredTasks()
{
    for (int32 TaskIndex : DeferredTaskIndices)
    {
        if (!TaskInstances.IsValidIndex(TaskIndex))
        {
            continue;
        }
        UKataTaskInstance* TaskInstance = TaskInstances[TaskIndex];
        if (!IsValid(TaskInstance))
        {
            continue;
        }

        // 지원하지 않는 상황을 묵시적 성공으로 감추지 않고 진단으로 남긴다.
        UE_LOG(LogKata, Warning, TEXT("Kata task '%s' never started: completion dependency was not satisfied before the timeline ended"),
            *TaskInstance->GetDisplayName());
        TaskInstance->MarkSkipped();
    }
    DeferredTaskIndices.Reset();
}

void UKataInstance::ApplyGasActivationState()
{
    if (bGasActivationApplied || ResolvedDefinition == nullptr)
    {
        return;
    }

    UAbilitySystemComponent* AbilitySystem = Context.ResolveAbilitySystem();
    if (AbilitySystem == nullptr)
    {
        UE_LOG(LogKata, Warning, TEXT("Kata '%s' started without an Ability System Component; active tags and cooldown are skipped"),
            ResolvedDefinition->SourceClass != nullptr ? *ResolvedDefinition->SourceClass->GetName() : TEXT("None"));
        return;
    }

    bGasActivationApplied = true;
    KataGas::AddActiveGrantedTags(*ResolvedDefinition, *AbilitySystem);
    KataGas::BlockAbilities(*ResolvedDefinition, *AbilitySystem);

    if (ResolvedDefinition->CooldownPolicy.bEnabled
        && ResolvedDefinition->CooldownPolicy.ApplyTime == EKataCooldownApplyTime::OnActivation)
    {
        CooldownHandle = KataGas::ApplyCooldown(*ResolvedDefinition, *AbilitySystem);
    }
}

void UKataInstance::RemoveGasActivationState()
{
    if (!bGasActivationApplied || ResolvedDefinition == nullptr)
    {
        return;
    }

    bGasActivationApplied = false;

    UAbilitySystemComponent* AbilitySystem = Context.ResolveAbilitySystem();
    if (AbilitySystem == nullptr)
    {
        return;
    }

    KataGas::RemoveActiveGrantedTags(*ResolvedDefinition, *AbilitySystem);
    KataGas::UnblockAbilities(*ResolvedDefinition, *AbilitySystem);

    // 종료 시점 쿨다운은 중단·취소를 포함한 모든 종료에서 적용한다.
    if (ResolvedDefinition->CooldownPolicy.bEnabled
        && ResolvedDefinition->CooldownPolicy.ApplyTime == EKataCooldownApplyTime::OnEnd)
    {
        CooldownHandle = KataGas::ApplyCooldown(*ResolvedDefinition, *AbilitySystem);
    }
}

TArray<UKataTaskInstance*> UKataInstance::GetActiveTaskInstances() const
{
    TArray<UKataTaskInstance*> Result;
    Result.Reserve(ActiveTaskIndices.Num());
    for (int32 TaskIndex : ActiveTaskIndices)
    {
        if (TaskInstances.IsValidIndex(TaskIndex) && IsValid(TaskInstances[TaskIndex]))
        {
            Result.Add(TaskInstances[TaskIndex]);
        }
    }
    return Result;
}
