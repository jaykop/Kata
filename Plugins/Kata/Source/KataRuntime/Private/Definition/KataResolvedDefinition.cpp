#include "Definition/KataResolvedDefinition.h"

#include "Definition/KataTask.h"
#include "Definition/KataAsset.h"
#include "KataRuntimeLog.h"

bool UKataResolvedDefinition::HasErrors() const
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

float UKataResolvedDefinition::GetTimelineDuration() const
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

const UKataTask* UKataResolvedDefinition::FindTask(const FKataTaskId& TaskId) const
{
    const int32 Index = FindTaskIndex(TaskId);
    return Tasks.IsValidIndex(Index) ? Tasks[Index].Get() : nullptr;
}

int32 UKataResolvedDefinition::FindTaskIndex(const FKataTaskId& TaskId) const
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

void UKataResolvedDefinition::AddDiagnostic(EKataDiagnosticSeverity Severity, FName Code, const FKataTaskId& TaskId, FString Detail,
    FString TaskLabel, bool bIncompleteAuthoring)
{
    Diagnostics.Emplace(Severity, Code, TaskId, MoveTemp(Detail), MoveTemp(TaskLabel), bIncompleteAuthoring);
}

void UKataResolvedDefinition::LogDiagnostics() const
{
    const FString SourceName = SourceAsset ? SourceAsset->GetName() : (SourceClass != nullptr ? SourceClass->GetName() : TEXT("None"));
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
