#include "Action/KataAction.h"

#include "Algo/Reverse.h"
#include "Action/KataPropertyOverride.h"
#include "Action/KataResolvedAction.h"
#include "Action/KataTask.h"
#include "KataCondition.h"
#include "KataRuntimeLog.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
    /** 해석 중 유지하는 태스크 작업본. 상속 순서대로 오버라이드를 누적한다. */
    struct FKataWorkingTask
    {
        TObjectPtr<UKataTask> Task;
        int32 DeclarationIndex = 0;
        bool bDisabledByOverride = false;
    };

    /** 정렬 비교에 쓰는 안정적인 키. 시간이 같을 때만 단계와 보조 순서를 본다. */
    struct FKataOrderKey
    {
        float StartTime = 0.0f;
        uint8 Phase = 0;
        int32 OrderHint = 0;
        int32 DeclarationIndex = 0;

        bool IsLessThan(const FKataOrderKey& Other) const
        {
            if (!FMath::IsNearlyEqual(StartTime, Other.StartTime))
            {
                return StartTime < Other.StartTime;
            }
            if (Phase != Other.Phase)
            {
                return Phase < Other.Phase;
            }
            if (OrderHint != Other.OrderHint)
            {
                return OrderHint < Other.OrderHint;
            }
            return DeclarationIndex < Other.DeclarationIndex;
        }
    };
}

bool UKataAction::CollectActionChain(TArray<const UKataAction*>& OutChain) const
{
    OutChain.Reset();
    TSet<const UKataAction*> Visited;
    for (const UKataAction* Current = this; Current; Current = Current->ParentAction)
    {
        if (Visited.Contains(Current))
        {
            OutChain.Reset();
            return false;
        }
        Visited.Add(Current);
        OutChain.Add(Current);
    }
    Algo::Reverse(OutChain);
    return true;
}

UKataAction* UKataAction::MakeEffectiveSettings(UObject* Outer) const
{
    UKataAction* Effective = NewObject<UKataAction>(Outer ? Outer : GetTransientPackage(), NAME_None, RF_Transient | RF_Transactional);
    TArray<const UKataAction*> Chain;
    if (!CollectActionChain(Chain))
    {
        return Effective;
    }

    // 고유 설정만 합친다. 태스크와 부모 참조는 별도의 병합 규칙을 따른다.
    const TArray<FName> Settings = {
        TEXT("KataTags"), TEXT("ActivationRequiredTags"), TEXT("ActivationBlockedTags"),
        TEXT("ActiveGrantedTags"), TEXT("StartCondition"), TEXT("BlockingPolicy"),
        TEXT("CooldownPolicy"), TEXT("LoopPolicy")
    };
    for (int32 Index = 0; Index < Chain.Num(); ++Index)
    {
        const UKataAction* Source = Chain[Index];
        const TArray<FName>& Paths = Index == 0 ? Settings : Source->OverriddenSettings;
        for (FName Path : Paths)
        {
            FString RootName;
            FString Tail;
            if (!Path.ToString().Split(TEXT("."), &RootName, &Tail))
            {
                RootName = Path.ToString();
            }
            if (Settings.Contains(FName(*RootName)))
            {
                FString Error;
                KataPropertyOverride::CopyOverriddenProperty(Effective, Source, Path, Error);
            }
        }
    }
    Effective->ParentAction = ParentAction;
    Effective->OverriddenSettings = OverriddenSettings;
#if WITH_EDITORONLY_DATA
    Effective->PreviewActorClass = PreviewActorClass;
    Effective->PreviewTargetClass = PreviewTargetClass;
    Effective->PreviewActorTransform = PreviewActorTransform;
    Effective->PreviewTargetTransform = PreviewTargetTransform;
    Effective->PreviewLightRotation = PreviewLightRotation;
    Effective->PreviewLightBrightness = PreviewLightBrightness;
    Effective->PreviewLightColor = PreviewLightColor;
    Effective->PreviewBackgroundColor = PreviewBackgroundColor;
    Effective->PreviewEnvironmentSize = PreviewEnvironmentSize;
    Effective->bPreviewShowDebugShape = bPreviewShowDebugShape;
    Effective->PreviewDebugShape = PreviewDebugShape;
    Effective->PreviewDebugColor = PreviewDebugColor;
    Effective->PreviewDebugThickness = PreviewDebugThickness;
    Effective->PreviewGridCellSize = PreviewGridCellSize;
#endif
    return Effective;
}

UKataResolvedAction* UKataAction::Resolve(UObject* Outer, bool bForEditing) const
{
    UObject* ResultOuter = Outer ? Outer : GetTransientPackage();
    TArray<const UKataAction*> Chain;
    if (!CollectActionChain(Chain))
    {
        UKataResolvedAction* Result = NewObject<UKataResolvedAction>(ResultOuter);
        Result->SourceAction = const_cast<UKataAction*>(this);
        Result->AddDiagnostic(EKataDiagnosticSeverity::Error, TEXT("ParentActionCycle"), {},
            TEXT("Parent Kata actions form a cycle"));
        return Result;
    }

    UKataAction* Effective = MakeEffectiveSettings(GetTransientPackage());
    UKataResolvedAction* Result = ResolveChain(Chain, Effective, ResultOuter, bForEditing);
    Result->SourceAction = const_cast<UKataAction*>(this);
    return Result;
}

UKataResolvedAction* UKataAction::ResolveChain(const TArray<const UKataAction*>& Chain,
    const UKataAction* EffectiveSettings, UObject* Outer, bool bForEditing)
{
    UKataResolvedAction* Resolved = NewObject<UKataResolvedAction>(Outer ? Outer : GetTransientPackage());
    // 상속 처리를 마친 고유 설정을 실행용 사본으로 옮긴다.
    Resolved->KataTags = EffectiveSettings->KataTags;
    Resolved->ActivationRequiredTags = EffectiveSettings->ActivationRequiredTags;
    Resolved->ActivationBlockedTags = EffectiveSettings->ActivationBlockedTags;
    Resolved->ActiveGrantedTags = EffectiveSettings->ActiveGrantedTags;
    Resolved->BlockingPolicy = EffectiveSettings->BlockingPolicy;
    Resolved->CooldownPolicy = EffectiveSettings->CooldownPolicy;
    Resolved->LoopPolicy = EffectiveSettings->LoopPolicy;
    if (EffectiveSettings->StartCondition != nullptr)
    {
        // 조건 객체는 공유하지 않고 실행용 사본을 해석 결과가 소유한다.
        Resolved->StartCondition = DuplicateObject<UKataCondition>(EffectiveSettings->StartCondition, Resolved);
    }

    TMap<FKataTaskId, FKataWorkingTask> WorkingTasks;
    TArray<FKataTaskId> DeclarationOrder;
    TSet<FKataTaskId> RemovedTaskIds;
    int32 NextDeclarationIndex = 0;

    for (const UKataAction* Action : Chain)
    {
        for (const FKataTimelineEntry& Entry : Action->TimelineTasks)
        {
            if (Entry.Task == nullptr)
            {
                Resolved->AddDiagnostic(EKataDiagnosticSeverity::Warning, TEXT("EmptyTimelineEntry"), FKataTaskId(),
                    FString::Printf(TEXT("Timeline entry declared by '%s' has no task"), *Action->GetName()));
                continue;
            }

            const FKataTaskId TaskId = Entry.Task->TaskId;
            if (!TaskId.IsValid())
            {
                Resolved->AddDiagnostic(EKataDiagnosticSeverity::Error, TEXT("MissingTaskId"), FKataTaskId(),
                    FString::Printf(TEXT("Task '%s' declared by '%s' has no stable id"), *Entry.Task->GetDisplayName(), *Action->GetName()));
                continue;
            }

            if (WorkingTasks.Contains(TaskId) || RemovedTaskIds.Contains(TaskId))
            {
                Resolved->AddDiagnostic(EKataDiagnosticSeverity::Error, TEXT("DuplicateTaskId"), TaskId,
                    FString::Printf(TEXT("Task id declared more than once (action '%s')"), *Action->GetName()));
                continue;
            }

            FKataWorkingTask Working;
            Working.Task = DuplicateObject<UKataTask>(Entry.Task, Resolved, MakeUniqueObjectName(Resolved, Entry.Task->GetClass()));
            Working.DeclarationIndex = NextDeclarationIndex++;
            WorkingTasks.Add(TaskId, Working);
            DeclarationOrder.Add(TaskId);
        }

        for (const FKataTaskOverride& Override : Action->TaskOverrides)
        {
            if (!Override.TargetTaskId.IsValid())
            {
                Resolved->AddDiagnostic(EKataDiagnosticSeverity::Warning, TEXT("InvalidOverrideTarget"), FKataTaskId(),
                    FString::Printf(TEXT("Override declared by '%s' has no target task id"), *Action->GetName()));
                continue;
            }

            FKataWorkingTask* Target = WorkingTasks.Find(Override.TargetTaskId);
            if (Target == nullptr)
            {
                // 부모에서 삭제된 태스크의 오버라이드다. 다른 태스크에 재적용하지 않는다.
                Resolved->AddDiagnostic(EKataDiagnosticSeverity::Warning, TEXT("OrphanedOverride"), Override.TargetTaskId,
                    FString::Printf(TEXT("Override declared by '%s' targets a task that no longer exists"), *Action->GetName()));
                continue;
            }

            switch (Override.Mode)
            {
            case EKataTimelineChangeMode::Remove:
                WorkingTasks.Remove(Override.TargetTaskId);
                DeclarationOrder.Remove(Override.TargetTaskId);
                RemovedTaskIds.Add(Override.TargetTaskId);
                break;

            case EKataTimelineChangeMode::Disable:
                Target->bDisabledByOverride = true;
                break;

            case EKataTimelineChangeMode::Modify:
            default:
            {
                if (Override.OverrideValues == nullptr)
                {
                    Resolved->AddDiagnostic(EKataDiagnosticSeverity::Warning, TEXT("MissingOverrideValues"), Override.TargetTaskId,
                        FString::Printf(TEXT("Modify override declared by '%s' has no value template"), *Action->GetName()));
                    break;
                }

                UClass* TargetClass = Target->Task->GetClass();
                UClass* OverrideClass = Override.OverrideValues->GetClass();
                if (!OverrideClass->IsChildOf(TargetClass) && !TargetClass->IsChildOf(OverrideClass))
                {
                    // 태스크 타입이 바뀐 경우다. 무관한 태스크에 오래된 필드를 적용하지 않는다.
                    Resolved->AddDiagnostic(EKataDiagnosticSeverity::Error, TEXT("OverrideClassMismatch"), Override.TargetTaskId,
                        FString::Printf(TEXT("Override value class '%s' is unrelated to task class '%s'"), *OverrideClass->GetName(), *TargetClass->GetName()));
                    break;
                }

                static const FName TaskIdPropertyName = GET_MEMBER_NAME_CHECKED(UKataTask, TaskId);
                for (const FName& PropertyName : Override.OverriddenProperties)
                {
#if !WITH_EDITORONLY_DATA
                    // 편집기 전용 태스크 표시는 cooked 런타임에 프로퍼티가 없으므로 조용히 건너뛴다.
                    if (PropertyName == TEXT("bUseAutomaticTimelineColor")
                        || PropertyName == TEXT("TimelineDisplayColor") || PropertyName == TEXT("EditorComment"))
                    {
                        continue;
                    }
#endif
                    if (PropertyName == TaskIdPropertyName)
                    {
                        Resolved->AddDiagnostic(EKataDiagnosticSeverity::Warning, TEXT("TaskIdOverrideIgnored"), Override.TargetTaskId,
                            TEXT("Task id cannot be overridden"));
                        continue;
                    }

                    if (PropertyName == GET_MEMBER_NAME_CHECKED(UKataTask, bEnabled))
                    {
                        Target->bDisabledByOverride = false;
                    }
                    FString CopyError;
                    if (!KataPropertyOverride::CopyOverriddenProperty(Target->Task, Override.OverrideValues, PropertyName, CopyError))
                    {
                        Resolved->AddDiagnostic(EKataDiagnosticSeverity::Warning, TEXT("OverridePropertyFailed"), Override.TargetTaskId, CopyError);
                    }
                }
                break;
            }
            }
        }
    }

    if (bForEditing)
    {
        // 미완성 설정과 비활성 항목도 에디터에서 선택하고 고칠 수 있어야 한다.
        for (const FKataTaskId& Id : DeclarationOrder)
        {
            if (FKataWorkingTask* Working = WorkingTasks.Find(Id))
            {
                Working->Task->bEnabled &= !Working->bDisabledByOverride;
                Resolved->Tasks.Add(Working->Task);
            }
        }
        return Resolved;
    }

    // 실행 후보를 고른다. Disable과 bEnabled는 같은 결과를 내지만 기록은 구분해 남긴다.
    TArray<FKataWorkingTask> Candidates;
    Candidates.Reserve(DeclarationOrder.Num());
    for (const FKataTaskId& TaskId : DeclarationOrder)
    {
        const FKataWorkingTask* Working = WorkingTasks.Find(TaskId);
        if (Working == nullptr || Working->Task == nullptr)
        {
            continue;
        }
        if (Working->bDisabledByOverride || !Working->Task->bEnabled)
        {
            Resolved->AddDiagnostic(EKataDiagnosticSeverity::Info, TEXT("TaskDisabled"), TaskId,
                TEXT("Disabled and will not run"), Working->Task->GetDisplayName());
            continue;
        }

        const FName ConfigurationError = Working->Task->GetConfigurationError();
        if (!ConfigurationError.IsNone())
        {
            // 저작이 끝나지 않은 설정이므로 실행에서는 제외하되 저장 검사에서는 경고로 다룬다.
            Resolved->AddDiagnostic(EKataDiagnosticSeverity::Error, ConfigurationError, TaskId,
                FString::Printf(TEXT("Starts at %.3fs and will not run: %s"),
                    Working->Task->StartTime, *Working->Task->DescribeConfigurationError(ConfigurationError)),
                Working->Task->GetDisplayName(), true);
            continue;
        }

        Candidates.Add(*Working);
    }

    // 의존성 검사. 시간 순서와 모순되거나 대상이 없는 관계는 거절한다.
    TMap<FKataTaskId, int32> CandidateIndexById;
    for (int32 Index = 0; Index < Candidates.Num(); ++Index)
    {
        CandidateIndexById.Add(Candidates[Index].Task->TaskId, Index);
    }

    TArray<int32> InDegrees;
    InDegrees.SetNumZeroed(Candidates.Num());
    TArray<TArray<int32>> Dependents;
    Dependents.SetNum(Candidates.Num());

    for (int32 Index = 0; Index < Candidates.Num(); ++Index)
    {
        const UKataTask* Task = Candidates[Index].Task;
        for (const FKataTaskDependency& Dependency : Task->Dependencies)
        {
            const int32* PrerequisiteIndex = CandidateIndexById.Find(Dependency.TaskId);
            if (PrerequisiteIndex == nullptr)
            {
                const bool bExistsButInactive = WorkingTasks.Contains(Dependency.TaskId) || RemovedTaskIds.Contains(Dependency.TaskId);
                Resolved->AddDiagnostic(EKataDiagnosticSeverity::Error,
                    bExistsButInactive ? FName(TEXT("DependencyOnInactiveTask")) : FName(TEXT("MissingDependency")),
                    Task->TaskId,
                    FString::Printf(TEXT("Depends on task %s which is not part of the resolved timeline"),
                        *Dependency.TaskId.ToString()),
                    Task->GetDisplayName());
                continue;
            }

            const UKataTask* Prerequisite = Candidates[*PrerequisiteIndex].Task;
            if (Prerequisite->StartTime > Task->StartTime + UE_KINDA_SMALL_NUMBER)
            {
                Resolved->AddDiagnostic(EKataDiagnosticSeverity::Error, TEXT("DependencyTimeConflict"), Task->TaskId,
                    FString::Printf(TEXT("Starts at %.3fs but depends on '%s' starting later at %.3fs"),
                        Task->StartTime, *Prerequisite->GetDisplayName(), Prerequisite->StartTime),
                    Task->GetDisplayName());
                continue;
            }

            InDegrees[Index]++;
            Dependents[*PrerequisiteIndex].Add(Index);
        }
    }

    // 강제된 선후 관계를 먼저 만족시키고, 독립된 항목 사이에서만 안정적인 키로 순서를 정한다.
    TArray<bool> bPlaced;
    bPlaced.Init(false, Candidates.Num());
    TArray<FKataOrderKey> OrderKeys;
    OrderKeys.SetNum(Candidates.Num());
    for (int32 Index = 0; Index < Candidates.Num(); ++Index)
    {
        const UKataTask* Task = Candidates[Index].Task;
        OrderKeys[Index].StartTime = Task->StartTime;
        OrderKeys[Index].Phase = static_cast<uint8>(Task->Phase);
        OrderKeys[Index].OrderHint = Task->OrderHint;
        OrderKeys[Index].DeclarationIndex = Candidates[Index].DeclarationIndex;
    }

    TArray<int32> SortedIndices;
    SortedIndices.Reserve(Candidates.Num());
    for (int32 Step = 0; Step < Candidates.Num(); ++Step)
    {
        int32 BestIndex = INDEX_NONE;
        for (int32 Index = 0; Index < Candidates.Num(); ++Index)
        {
            if (bPlaced[Index] || InDegrees[Index] > 0)
            {
                continue;
            }
            if (BestIndex == INDEX_NONE || OrderKeys[Index].IsLessThan(OrderKeys[BestIndex]))
            {
                BestIndex = Index;
            }
        }

        if (BestIndex == INDEX_NONE)
        {
            for (int32 Index = 0; Index < Candidates.Num(); ++Index)
            {
                if (!bPlaced[Index])
                {
                    Resolved->AddDiagnostic(EKataDiagnosticSeverity::Error, TEXT("DependencyCycle"), Candidates[Index].Task->TaskId,
                        TEXT("Part of a dependency cycle"), Candidates[Index].Task->GetDisplayName());
                }
            }
            break;
        }

        bPlaced[BestIndex] = true;
        SortedIndices.Add(BestIndex);
        for (int32 DependentIndex : Dependents[BestIndex])
        {
            InDegrees[DependentIndex]--;
        }
    }

    Resolved->Tasks.Reserve(SortedIndices.Num());
    for (int32 Index : SortedIndices)
    {
        Resolved->Tasks.Add(Candidates[Index].Task);
    }

    if (Resolved->CooldownPolicy.bEnabled
        && (!(Resolved->CooldownPolicy.Duration > 0.0f) || !FMath::IsFinite(Resolved->CooldownPolicy.Duration)))
    {
        Resolved->AddDiagnostic(EKataDiagnosticSeverity::Error, TEXT("InvalidCooldownDuration"), FKataTaskId(),
            TEXT("Cooldown is enabled but duration is not a finite value greater than zero"));
    }

    if (Resolved->LoopPolicy.bLoop && Resolved->GetTimelineDuration() <= UE_KINDA_SMALL_NUMBER)
    {
        // 길이 0 타임라인을 무한 반복하면 한 프레임에서 진행하지 못한다.
        Resolved->AddDiagnostic(EKataDiagnosticSeverity::Error, TEXT("ZeroLengthLoop"), FKataTaskId(),
            TEXT("Loop is enabled but the resolved timeline has zero length"));
    }

    return Resolved;
}

#if WITH_EDITOR
void UKataAction::PostEditChangeChainProperty(FPropertyChangedChainEvent& Event)
{
    for (FKataTimelineEntry& Entry : TimelineTasks)
    {
        if (Entry.Task)
        {
            Entry.Task->EnsureTaskId();
        }
    }
    if (ParentAction && Event.MemberProperty)
    {
        const FName Name = Event.MemberProperty->GetFName();
        if (Name != GET_MEMBER_NAME_CHECKED(UKataAction, ParentAction)
            && Name != GET_MEMBER_NAME_CHECKED(UKataAction, OverriddenSettings)
            && Name != GET_MEMBER_NAME_CHECKED(UKataAction, TimelineTasks)
            && Name != GET_MEMBER_NAME_CHECKED(UKataAction, TaskOverrides)
            && !Name.ToString().StartsWith(TEXT("Preview"))
            && !Name.ToString().StartsWith(TEXT("bPreview")))
        {
            OverriddenSettings.AddUnique(KataPropertyOverride::GetPropertyPath(Event.MemberProperty, Event.Property));
        }
    }
    UObject::PostEditChangeChainProperty(Event);
}

EDataValidationResult UKataAction::IsDataValid(FDataValidationContext& Context) const
{
    UKataResolvedAction* Result = Resolve(GetTransientPackage());
    bool bInvalid = false;
    for (const FKataDiagnostic& Diagnostic : Result->Diagnostics)
    {
        if (Diagnostic.Severity == EKataDiagnosticSeverity::Info)
        {
            continue;
        }
        // 저작이 끝나지 않은 태스크는 저장을 막지 않는다. 실행에서는 그대로 제외한다.
        const bool bError = Diagnostic.Severity == EKataDiagnosticSeverity::Error && !Diagnostic.bIncompleteAuthoring;
        const FText Message = FText::FromString(Diagnostic.ToDetailString());
        if (bError)
        {
            Context.AddError(Message);
            bInvalid = true;
        }
        else
        {
            Context.AddWarning(Message);
        }
    }
    return bInvalid ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
