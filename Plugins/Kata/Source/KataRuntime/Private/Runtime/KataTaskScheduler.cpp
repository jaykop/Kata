#include "Runtime/KataTaskScheduler.h"

#include "Action/KataResolvedAction.h"
#include "Action/KataTask.h"

void FKataTaskScheduler::Reset()
{
    Tasks.Reset();
    Boundaries.Reset();
    TimelineDuration = 0.0f;
}

void FKataTaskScheduler::Build(const UKataResolvedAction* ResolvedDefinition)
{
    Reset();

    if (ResolvedDefinition == nullptr)
    {
        return;
    }

    // 해석 단계에서 이미 선후 관계를 만족하도록 정렬해 두었다.
    TMap<FKataTaskId, int32> IndexById;
    Tasks.Reserve(ResolvedDefinition->Tasks.Num());
    for (int32 Index = 0; Index < ResolvedDefinition->Tasks.Num(); ++Index)
    {
        UKataTask* Task = ResolvedDefinition->Tasks[Index];
        if (Task == nullptr)
        {
            continue;
        }

        FKataScheduledTask Scheduled;
        Scheduled.TaskId = Task->TaskId;
        Scheduled.Task = Task;
        Scheduled.ExecutionIndex = Tasks.Num();
        Scheduled.StartTime = Task->StartTime;
        Scheduled.EndTime = Task->GetEndTime();
        Scheduled.bInstant = Task->IsInstant();
        Scheduled.bSingleFrame = Task->bSingleFrame;

        IndexById.Add(Scheduled.TaskId, Tasks.Num());
        Tasks.Add(MoveTemp(Scheduled));
    }

    // 완료 대기만 런타임 지연이 필요하다. AfterStart는 정렬로 이미 보장된다.
    for (FKataScheduledTask& Scheduled : Tasks)
    {
        const UKataTask* Task = Scheduled.Task.Get();
        if (Task == nullptr)
        {
            continue;
        }
        for (const FKataTaskDependency& Dependency : Task->Dependencies)
        {
            if (Dependency.Requirement != EKataTaskDependencyRequirement::AfterCompletion)
            {
                continue;
            }
            if (const int32* PrerequisiteIndex = IndexById.Find(Dependency.TaskId))
            {
                Scheduled.CompletionPrerequisites.AddUnique(*PrerequisiteIndex);
            }
        }
    }

    for (const FKataScheduledTask& Scheduled : Tasks)
    {
        FKataTimelineBoundary StartBoundary;
        StartBoundary.Time = Scheduled.StartTime;
        StartBoundary.TaskIndex = Scheduled.ExecutionIndex;
        StartBoundary.Type = EKataBoundaryType::Start;
        Boundaries.Add(StartBoundary);

        // 한 프레임 태스크의 종료는 시간 경계가 아니라 Tick 횟수로 정한다.
        if (!Scheduled.bInstant && !Scheduled.bSingleFrame)
        {
            FKataTimelineBoundary EndBoundary;
            EndBoundary.Time = Scheduled.EndTime;
            EndBoundary.TaskIndex = Scheduled.ExecutionIndex;
            EndBoundary.Type = EKataBoundaryType::End;
            Boundaries.Add(EndBoundary);
        }

        TimelineDuration = FMath::Max(TimelineDuration, Scheduled.EndTime);
    }

    // 같은 시각에서는 기존 구간을 먼저 끝내고 새 구간을 시작한다.
    Boundaries.Sort([](const FKataTimelineBoundary& Lhs, const FKataTimelineBoundary& Rhs)
    {
        if (!FMath::IsNearlyEqual(Lhs.Time, Rhs.Time))
        {
            return Lhs.Time < Rhs.Time;
        }
        if (Lhs.Type != Rhs.Type)
        {
            return static_cast<uint8>(Lhs.Type) < static_cast<uint8>(Rhs.Type);
        }
        return Lhs.TaskIndex < Rhs.TaskIndex;
    });
}

void FKataTaskScheduler::CollectBoundaries(float FromTime, float ToTime, bool bIncludeFromTime, TArray<FKataTimelineBoundary>& OutBoundaries) const
{
    OutBoundaries.Reset();

    if (ToTime < FromTime)
    {
        return;
    }

    for (const FKataTimelineBoundary& Boundary : Boundaries)
    {
        const bool bAfterStart = bIncludeFromTime
            ? Boundary.Time >= FromTime - UE_KINDA_SMALL_NUMBER
            : Boundary.Time > FromTime + UE_KINDA_SMALL_NUMBER;

        if (bAfterStart && Boundary.Time <= ToTime + UE_KINDA_SMALL_NUMBER)
        {
            OutBoundaries.Add(Boundary);
        }
    }
}
