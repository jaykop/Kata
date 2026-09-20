#include "Action/KataResolvedAction.h"

#include "Action/KataTask.h"
#include "Action/KataAction.h"
#include "KataRuntimeLog.h"

bool UKataResolvedAction::HasErrors() const
{
    for (const FKataDiagnostic& Diagnostic : Diagnostics)
    {
        if (Diagnostic.Severity == EKataDiagnosticSeverity::Error)
        {
            return true;
        }
    }
    return false;
}

float UKataResolvedAction::GetTimelineDuration() const
{
    float Duration = 0.0f;
    for (const TObjectPtr<UKataTask>& Task : Tasks)
    {
        if (Task != nullptr)
        {
            Duration = FMath::Max(Duration, Task->GetEndTime());
        }
    }
    return Duration;
}

const UKataTask* UKataResolvedAction::FindTask(const FKataTaskId& TaskId) const
{
    const int32 Index = FindTaskIndex(TaskId);
    return Tasks.IsValidIndex(Index) ? Tasks[Index].Get() : nullptr;
}

int32 UKataResolvedAction::FindTaskIndex(const FKataTaskId& TaskId) const
{
    for (int32 Index = 0; Index < Tasks.Num(); ++Index)
    {
        if (Tasks[Index] != nullptr && Tasks[Index]->TaskId == TaskId)
        {
            return Index;
        }
    }
    return INDEX_NONE;
}

void UKataResolvedAction::AddDiagnostic(EKataDiagnosticSeverity Severity, FName Code, const FKataTaskId& TaskId, FString Detail,
    FString TaskLabel, bool bIncompleteAuthoring)
{
    Diagnostics.Emplace(Severity, Code, TaskId, MoveTemp(Detail), MoveTemp(TaskLabel), bIncompleteAuthoring);
}

void UKataResolvedAction::LogDiagnostics() const
{
    const FString SourceName = SourceAction ? SourceAction->GetName() : TEXT("None");
    for (const FKataDiagnostic& Diagnostic : Diagnostics)
    {
        switch (Diagnostic.Severity)
        {
        case EKataDiagnosticSeverity::Error:
            UE_LOG(LogKata, Error, TEXT("[%s] %s"), *SourceName, *Diagnostic.ToDisplayString());
            break;
        case EKataDiagnosticSeverity::Warning:
            UE_LOG(LogKata, Warning, TEXT("[%s] %s"), *SourceName, *Diagnostic.ToDisplayString());
            break;
        default:
            UE_LOG(LogKata, Log, TEXT("[%s] %s"), *SourceName, *Diagnostic.ToDisplayString());
            break;
        }
    }
}
