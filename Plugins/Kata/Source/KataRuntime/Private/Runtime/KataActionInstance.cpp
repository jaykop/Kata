#include "Runtime/KataActionInstance.h"

#include "AbilitySystemComponent.h"
#include "Action/KataCommand.h"
#include "Action/KataResolvedAction.h"
#include "Action/KataAction.h"
#include "Action/KataTask.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "GAS/KataGasBridge.h"
#include "GameFramework/Actor.h"
#include "KataRuntimeLog.h"
#include "Runtime/KataTaskInstance.h"

UKataAction* UKataActionInstance::GetKataAction() const
{
    return ResolvedDefinition ? ResolvedDefinition->SourceAction.Get() : nullptr;
}

UWorld* UKataActionInstance::GetWorld() const
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

EKataStartResult UKataActionInstance::InitializeInstance(UKataResolvedAction* InResolvedDefinition, const FKataContext& InContext)
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
    TaskLastTickTimes.Init(0.0f, TaskInstances.Num());

    InstanceState = EKataInstanceState::Created;
    CurrentTime = 0.0f;
    LoopIteration = 0;
    return EKataStartResult::Started;
}

void UKataActionInstance::StartInstance()
{
    if (InstanceState != EKataInstanceState::Created)
    {
        return;
    }

    InstanceState = EKataInstanceState::Running;
    CurrentTime = 0.0f;
    LoopIteration = 0;

    // 시작 처리를 이번 프레임의 갱신으로 기록한다. 콜백 재진입과 이후 월드 Tick의 중복 진행을 막는다.
    LastTickFrame = GFrameCounter;
    ++TickSerial;
    TGuardValue<bool> TickGuard(bTickingInstance, true);
    TickedTaskIndices.Reset();
    bIncludeInitialBoundary = false;

    ApplyGasActivationState();
    if (InstanceState == EKataInstanceState::Running && !bEndRequested)
    {
        // 타임라인보다 먼저 실행해 시각 0 태스크도 명령이 바꾼 대상을 읽게 한다.
        RunPreCommands();
    }
    if (InstanceState == EKataInstanceState::Running && !bEndRequested)
    {
        // 입력 반응을 다음 프레임으로 미루지 않는다. 경과 시간은 없으므로 첫 Tick은 0을 전달한다.
        AdvanceTo(0.0f, 0.0f, true);
    }

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
        // 순간 태스크와 Single Frame 태스크만 있는 길이 0 액션도 재생 요청 안에서 완료한다.
        EndInstance(EKataEndReason::Completed);
    }
}

void UKataActionInstance::TickInstance(float DeltaTime)
{
    if (InstanceState != EKataInstanceState::Running || bTickingInstance)
    {
        return;
    }
    if (!Context.HasValidOwner())
    {
        EndInstance(EKataEndReason::OwnerInvalid);
        return;
    }
    if (!(DeltaTime > 0.0f) || !FMath::IsFinite(DeltaTime))
    {
        return;
    }

    const UWorld* World = GetWorld();
    const bool bPreviewSimulation = World != nullptr && World->WorldType == EWorldType::EditorPreview;
    if (!bPreviewSimulation && LastTickFrame == GFrameCounter)
    {
        return;
    }
    LastTickFrame = GFrameCounter;
    ++TickSerial;
    TGuardValue<bool> TickGuard(bTickingInstance, true);
    TickedTaskIndices.Reset();

    if (bLoopPending)
    {
        BeginNextLoop();
    }

    // 애니메이션 등의 외부 콜백으로 충족된 완료 의존성도 이번 갱신에서 처리한다.
    TryStartDeferredTasks();

    const float Duration = Scheduler.GetTimelineDuration();
    const float FrameEndTime = FMath::Min(CurrentTime + DeltaTime, Duration);
    const bool bIncludeFromTime = bIncludeInitialBoundary;
    bIncludeInitialBoundary = false;
    if (InstanceState == EKataInstanceState::Running && !bEndRequested)
    {
        AdvanceTo(CurrentTime, FrameEndTime, bIncludeFromTime);
    }

    if (InstanceState != EKataInstanceState::Running)
    {
        return;
    }
    if (bEndRequested)
    {
        EndInstance(PendingEndReason);
        return;
    }
    if (CurrentTime < Duration - UE_KINDA_SMALL_NUMBER)
    {
        return;
    }
    if (!ShouldLoopAgain())
    {
        EndInstance(EKataEndReason::Completed);
        return;
    }

    // 이번 회차를 여기서 정리한다. 남은 DeltaTime은 넘기지 않고 다음 프레임에 다시 시작한다.
    bLoopPending = true;
    EndActiveTasks(EKataTaskEndReason::Interrupted);
    FlushDeferredTasks();
    OpenTransitionWindows.Reset();
    if (bEndRequested && InstanceState == EKataInstanceState::Running)
    {
        EndInstance(PendingEndReason);
    }
}

void UKataActionInstance::AdvanceTo(float FromTime, float ToTime, bool bIncludeFromTime)
{
    TArray<FKataTimelineBoundary> Boundaries;
    Scheduler.CollectBoundaries(FromTime, ToTime, bIncludeFromTime, Boundaries);

    int32 BoundaryIndex = 0;
    while (InstanceState == EKataInstanceState::Running && !bEndRequested)
    {
        float NextTime = ToTime;
        if (Boundaries.IsValidIndex(BoundaryIndex))
        {
            NextTime = FMath::Min(NextTime, Boundaries[BoundaryIndex].Time);
        }

        // 완료 의존성으로 늦게 시작한 태스크는 정적 타임라인 경계와 다른 시각에 끝날 수 있다.
        // 실제 종료 시각에서 마지막 Tick과 End를 처리해 지속 시간을 넘기지 않는다.
        for (int32 TaskIndex : ActiveTaskIndices)
        {
            if (!TaskEndTimes.IsValidIndex(TaskIndex))
            {
                continue;
            }
            const float EndTime = TaskEndTimes[TaskIndex];
            if (EndTime > CurrentTime + UE_KINDA_SMALL_NUMBER && EndTime < NextTime - UE_KINDA_SMALL_NUMBER)
            {
                NextTime = EndTime;
            }
        }

        CurrentTime = FMath::Max(CurrentTime, NextTime);

        // 같은 시각에서는 실행 중이던 태스크를 먼저 끝낸 뒤 새 태스크를 시작한다.
        FinishElapsedTasks();
        if (InstanceState != EKataInstanceState::Running || bEndRequested)
        {
            break;
        }

        // CollectBoundaries와 같은 허용 오차로 도달을 판정한다. 수집 범위는 ToTime보다 조금 뒤까지 포함하므로
        // 더 엄격한 기준을 쓰면 수집한 경계를 소비하지 못해 이 루프가 끝나지 않는다.
        // 그 경계는 다음 프레임 수집에서도 빠지므로 이번 프레임에 처리해야 누락되지 않는다.
        while (Boundaries.IsValidIndex(BoundaryIndex)
            && Boundaries[BoundaryIndex].Time <= CurrentTime + UE_KINDA_SMALL_NUMBER)
        {
            const FKataTimelineBoundary& Boundary = Boundaries[BoundaryIndex++];
            if (Boundary.Type == EKataBoundaryType::Start)
            {
                TryStartTask(Boundary.TaskIndex);
            }
            if (InstanceState != EKataInstanceState::Running || bEndRequested)
            {
                break;
            }
        }

        if (CurrentTime >= ToTime - UE_KINDA_SMALL_NUMBER
            && !Boundaries.IsValidIndex(BoundaryIndex))
        {
            break;
        }
    }

    if (InstanceState == EKataInstanceState::Running && !bEndRequested)
    {
        // 다른 태스크의 시작·종료 경계는 이 태스크의 Tick 횟수에 영향을 주지 않는다.
        TickActiveTasks();
    }
}

void UKataActionInstance::FinishElapsedTasks()
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
            TickTaskOnce(TaskIndex);
            if (InstanceState == EKataInstanceState::Running && !bEndRequested
                && ActiveTaskIndices.Contains(TaskIndex) && TaskInstances[TaskIndex]->IsRunning())
            {
                HandleTaskFinished(TaskInstances[TaskIndex], EKataTaskEndReason::Completed);
            }
        }
    }
}

void UKataActionInstance::TryStartTask(int32 TaskIndex)
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

    const TArray<FKataScheduledTask>& ScheduledTasks = Scheduler.GetTasks();
    if (LoopIteration > 0 && ScheduledTasks.IsValidIndex(TaskIndex) && !ScheduledTasks[TaskIndex].bRestartOnLoop)
    {
        // 첫 회차에만 실행하는 항목이다. 시작하지 않았음을 상태로 남겨 진단에서 구분할 수 있게 한다.
        TaskInstance->MarkSkipped();
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

void UKataActionInstance::StartTaskNow(int32 TaskIndex)
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
        // 한 프레임 태스크는 시간이 아니라 Tick 한 번으로 끝난다. 시간 경계가 먼저 끝내지 않게 막는다.
        TaskEndTimes[TaskIndex] = Scheduled.bSingleFrame ? TNumericLimits<float>::Max() : ActualEndTime;
    }

    if (!Scheduled.bInstant && !Scheduled.bSingleFrame && ActualEndTime > Scheduler.GetTimelineDuration() + UE_KINDA_SMALL_NUMBER)
    {
        // 늦은 시작으로 타임라인 길이를 넘기면 Kata 종료 시점에 잘린다. 조용히 넘기지 않는다.
        UE_LOG(LogKata, Warning,
            TEXT("Kata task '%s' started late at %.3fs; its end %.3fs exceeds the timeline duration %.3fs and it will be cut short"),
            *TaskInstance->GetDisplayName(), CurrentTime, ActualEndTime, Scheduler.GetTimelineDuration());
    }

    DeferredTaskIndices.Remove(TaskIndex);
    ActiveTaskIndices.AddUnique(TaskIndex);
    TaskLastTickTimes[TaskIndex] = CurrentTime;
    TaskInstance->BeginTask(CurrentTime);

    if (InstanceState != EKataInstanceState::Running || bEndRequested)
    {
        return;
    }

    if (Scheduled.bInstant && TaskInstance->IsRunning())
    {
        HandleTaskFinished(TaskInstance, EKataTaskEndReason::Completed);
    }
    else if (Scheduled.bSingleFrame && TaskInstance->IsRunning())
    {
        // 한 프레임 태스크는 타임라인 끝이나 루프 경계에서도 누락되지 않도록 시작 즉시 한 번 실행한다.
        TickTaskOnce(TaskIndex);
        if (InstanceState == EKataInstanceState::Running && !bEndRequested
            && ActiveTaskIndices.Contains(TaskIndex) && TaskInstance->IsRunning())
        {
            HandleTaskFinished(TaskInstance, EKataTaskEndReason::Completed);
        }
    }
}

bool UKataActionInstance::AreCompletionPrerequisitesMet(int32 TaskIndex) const
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

void UKataActionInstance::TryStartDeferredTasks()
{
    if (!bTickingInstance || bLoopPending || InstanceState != EKataInstanceState::Running || bEndRequested)
    {
        return;
    }
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
    while (bDeferredTasksDirty && InstanceState == EKataInstanceState::Running && !bEndRequested);

    bResolvingDeferredTasks = false;
}

void UKataActionInstance::HandleTaskFinished(UKataTaskInstance* TaskInstance, EKataTaskEndReason Reason)
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

    if (Reason == EKataTaskEndReason::Completed && InstanceState == EKataInstanceState::Running && !bLoopPending)
    {
        CompletedTaskIndices.Add(TaskIndex);
        TryStartDeferredTasks();
    }
}

void UKataActionInstance::TickTaskOnce(int32 TaskIndex)
{
    if (TickedTaskIndices.Contains(TaskIndex))
    {
        return;
    }
    TickedTaskIndices.Add(TaskIndex);

    if (InstanceState != EKataInstanceState::Running || bEndRequested
        || !ActiveTaskIndices.Contains(TaskIndex) || !TaskInstances.IsValidIndex(TaskIndex))
    {
        return;
    }
    UKataTaskInstance* TaskInstance = TaskInstances[TaskIndex];
    if (!IsValid(TaskInstance) || !TaskInstance->IsRunning())
    {
        return;
    }

    const float TaskDeltaTime = FMath::Max(0.0f, CurrentTime - TaskLastTickTimes[TaskIndex]);
    TaskLastTickTimes[TaskIndex] = CurrentTime;
    TaskInstance->TickTask(TaskDeltaTime, CurrentTime);
}

void UKataActionInstance::TickActiveTasks()
{
    // 콜백에서 대기 태스크가 시작될 수 있다. 아직 처리하지 않은 항목만 골라 각각 한 번 호출한다.
    while (InstanceState == EKataInstanceState::Running && !bEndRequested)
    {
        const int32* NextTaskIndex = ActiveTaskIndices.FindByPredicate([this](int32 TaskIndex)
        {
            return !TickedTaskIndices.Contains(TaskIndex);
        });
        if (NextTaskIndex == nullptr)
        {
            break;
        }
        // Tick 콜백이 배열을 바꾸기 전에 인덱스를 값으로 전달한다.
        TickTaskOnce(*NextTaskIndex);
    }
}

void UKataActionInstance::BeginNextLoop()
{
    // 이전 프레임에서 정리를 마쳤다. Task는 반복 여부를 판단하지 않고 새 실행을 준비한다.
    bLoopPending = false;
    CompletedTaskIndices.Reset();

    for (UKataTaskInstance* TaskInstance : TaskInstances)
    {
        if (IsValid(TaskInstance))
        {
            TaskInstance->ResetForExecution();
        }
    }
    TaskEndTimes.Init(0.0f, TaskInstances.Num());
    TaskLastTickTimes.Init(0.0f, TaskInstances.Num());

    ++LoopIteration;
    CurrentTime = 0.0f;

    bIncludeInitialBoundary = true;
}

bool UKataActionInstance::ShouldLoopAgain() const
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

void UKataActionInstance::RequestEnd(EKataEndReason Reason)
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

    // 종료가 미뤄진 사이에 들어온 요청은 사유를 덮지 않는다. 그래프 전이가 Branched로 끝낸 뒤
    // 다음 액션 시작이 KataComponent에서 다시 Interrupted를 요청하는 경우가 이에 해당한다.
    if (!bEndRequested)
    {
        bEndRequested = true;
        PendingEndReason = Reason;
    }

    // 경계 처리 중이 아니면 즉시 끝낸다. 처리 중이면 해당 루프가 안전한 지점에서 처리한다.
    if (!bResolvingDeferredTasks)
    {
        EndInstance(PendingEndReason);
    }
}

void UKataActionInstance::EndInstance(EKataEndReason Reason)
{
    if (InstanceState == EKataInstanceState::Ended)
    {
        return;
    }

    // 시작하지 못하고 끝난 실행에서는 PreCommands가 돌지 않았으므로 PostCommands도 실행하지 않는다.
    const bool bWasStarted = InstanceState == EKataInstanceState::Running;
    InstanceState = EKataInstanceState::Ended;
    bEndRequested = false;
    EndReason = Reason;

    EndActiveTasks(EKataTaskEndReason::KataEnded);
    FlushDeferredTasks();
    if (bWasStarted)
    {
        // 타임라인 태스크가 자원을 회수한 뒤, GAS 활성 태그를 거두기 전에 실행한다.
        RunPostCommands(Reason);
    }
    RemoveGasActivationState();

    // 창 태스크가 정리되며 스스로 닫지만, 어떤 사유로 끝나도 남지 않도록 비운다.
    OpenTransitionWindows.Reset();

    OnKataEnded.Broadcast(this, Reason);
}

void UKataActionInstance::EndActiveTasks(EKataTaskEndReason Reason)
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

void UKataActionInstance::FlushDeferredTasks()
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

bool UKataActionInstance::SetTargetActor(AActor* NewTarget)
{
    if (!bRunningPreCommands)
    {
        UE_LOG(LogKata, Warning, TEXT("Kata action '%s' rejected a target change outside its pre commands"),
            *GetNameSafe(GetKataAction()));
        return false;
    }

    Context.TargetActor = NewTarget;
    return true;
}

void UKataActionInstance::RunPreCommands()
{
    if (ResolvedDefinition == nullptr)
    {
        return;
    }

    TGuardValue<bool> PreCommandGuard(bRunningPreCommands, true);
    // 명령 안에서 액션이 끝나 해석 결과가 바뀌어도 순회가 깨지지 않도록 사본을 순회한다.
    const TArray<TObjectPtr<UKataCommand>> Commands = ResolvedDefinition->PreCommands;
    for (UKataCommand* Command : Commands)
    {
        if (InstanceState != EKataInstanceState::Running || bEndRequested)
        {
            break;
        }
        if (IsValid(Command))
        {
            Command->Run(this);
        }
    }
}

void UKataActionInstance::RunPostCommands(EKataEndReason Reason)
{
    if (ResolvedDefinition == nullptr)
    {
        return;
    }

    const TArray<FKataPostCommandEntry> Entries = ResolvedDefinition->PostCommands;
    for (const FKataPostCommandEntry& Entry : Entries)
    {
        if (IsValid(Entry.Command) && Entry.ShouldRunFor(Reason))
        {
            Entry.Command->Run(this);
        }
    }
}

void UKataActionInstance::ApplyGasActivationState()
{
    if (bGasActivationApplied || ResolvedDefinition == nullptr)
    {
        return;
    }

    UAbilitySystemComponent* AbilitySystem = Context.ResolveAbilitySystem();
    if (AbilitySystem == nullptr)
    {
        UE_LOG(LogKata, Warning, TEXT("Kata '%s' started without an Ability System Component; active tags and cooldown are skipped"),
            ResolvedDefinition->SourceAction != nullptr ? *ResolvedDefinition->SourceAction->GetName() : TEXT("None"));
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

void UKataActionInstance::RemoveGasActivationState()
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

TArray<UKataTaskInstance*> UKataActionInstance::GetActiveTaskInstances() const
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

void UKataActionInstance::OpenTransitionWindow(const FGameplayTag& WindowTag, float PreAcceptSeconds)
{
    if (!WindowTag.IsValid())
    {
        return;
    }

    const UWorld* World = GetWorld();
    const float NowSeconds = World != nullptr ? World->GetTimeSeconds() : 0.0f;

    FKataOpenTransitionWindow& Window = OpenTransitionWindows.FindOrAdd(WindowTag);
    if (Window.OpenCount == 0)
    {
        // 처음 열릴 때만 시각을 기록한다. 겹쳐 열려도 선행 수용 폭의 기준은 첫 개방이다.
        Window.OpenedAtWorldSeconds = NowSeconds;
        Window.PreAcceptSeconds = PreAcceptSeconds;
    }
    else
    {
        // 겹쳐 열리면 더 관대한 값을 따른다.
        Window.PreAcceptSeconds = FMath::Max(Window.PreAcceptSeconds, PreAcceptSeconds);
    }
    ++Window.OpenCount;
}

void UKataActionInstance::CloseTransitionWindow(const FGameplayTag& WindowTag)
{
    FKataOpenTransitionWindow* Window = OpenTransitionWindows.Find(WindowTag);
    if (Window == nullptr)
    {
        return;
    }

    --Window->OpenCount;
    if (Window->OpenCount <= 0)
    {
        OpenTransitionWindows.Remove(WindowTag);
    }
}

bool UKataActionInstance::IsTransitionWindowOpen(const FGameplayTag& WindowTag) const
{
    return OpenTransitionWindows.Contains(WindowTag);
}

bool UKataActionInstance::AcceptsTriggerAt(const FGameplayTag& WindowTag, float TriggerWorldSeconds) const
{
    if (!IsRunning())
    {
        return false;
    }

    // 창을 요구하지 않는 전이는 액션 실행 중에는 항상 받는다.
    if (!WindowTag.IsValid())
    {
        return true;
    }

    const FKataOpenTransitionWindow* Window = OpenTransitionWindows.Find(WindowTag);
    if (Window == nullptr)
    {
        return false;
    }

    return TriggerWorldSeconds >= Window->OpenedAtWorldSeconds - Window->PreAcceptSeconds;
}
