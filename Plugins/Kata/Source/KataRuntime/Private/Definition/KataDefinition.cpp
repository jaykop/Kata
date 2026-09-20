#include "Definition/KataDefinition.h"

#include "Algo/Reverse.h"
#include "Definition/KataPropertyOverride.h"
#include "Definition/KataResolvedDefinition.h"
#include "Definition/KataTask.h"
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

    /** 두 객체의 리플렉션 프로퍼티 값이 모두 같은지 비교한다. */
    bool ArePropertyValuesIdentical(const UObject* Lhs, const UObject* Rhs)
    {
        if (Lhs == nullptr || Rhs == nullptr || Lhs->GetClass() != Rhs->GetClass())
        {
            return false;
        }
        for (TFieldIterator<FProperty> PropertyIt(Lhs->GetClass()); PropertyIt; ++PropertyIt)
        {
            // Instanced 서브오브젝트는 포인터가 다르므로 내용으로 비교한다.
            if (!PropertyIt->Identical_InContainer(Lhs, Rhs, 0, PPF_DeepComparison))
            {
                return false;
            }
        }
        return true;
    }

    /** 부모 클래스가 선언한 원본 항목을 찾는다. */
    const UKataTask* FindDeclaredTask(const UClass* DeclaringClass, const FKataTaskId& TaskId)
    {
        if (DeclaringClass == nullptr)
        {
            return nullptr;
        }
        const UKataDefinition* Defaults = DeclaringClass->GetDefaultObject<UKataDefinition>();
        if (Defaults == nullptr)
        {
            return nullptr;
        }
        for (const FKataTimelineEntry& Entry : Defaults->TimelineTasks)
        {
            if (Entry.Task != nullptr && Entry.Task->TaskId == TaskId)
            {
                return Entry.Task;
            }
        }
        return nullptr;
    }

    /** 정의 클래스 체인을 루트부터 잎 순서로 모은다. */
    void CollectDefinitionChain(UClass* LeafClass, TArray<const UKataDefinition*>& OutChain)
    {
        for (UClass* Current = LeafClass; Current != nullptr; Current = Current->GetSuperClass())
        {
            if (!Current->IsChildOf(UKataDefinition::StaticClass()))
            {
                break;
            }
            if (const UKataDefinition* CDO = Current->GetDefaultObject<UKataDefinition>())
            {
                OutChain.Add(CDO);
            }
        }
        Algo::Reverse(OutChain);
    }
}

UKataResolvedDefinition* UKataDefinition::ResolveDefinition(TSubclassOf<UKataDefinition> DefinitionClass, UObject* Outer, bool bForEditing)
{
    if (DefinitionClass == nullptr)
    {
        return nullptr;
    }

    UKataResolvedDefinition* Resolved = NewObject<UKataResolvedDefinition>(Outer != nullptr ? Outer : GetTransientPackage());
    Resolved->SourceClass = DefinitionClass;

    const UKataDefinition* LeafDefaults = DefinitionClass->GetDefaultObject<UKataDefinition>();
    if (LeafDefaults == nullptr)
    {
        Resolved->AddDiagnostic(EKataDiagnosticSeverity::Error, TEXT("MissingClassDefaults"), FKataTaskId(),
            FString::Printf(TEXT("Definition class '%s' has no class default object"), *DefinitionClass->GetName()));
        return Resolved;
    }

    if (DefinitionClass->HasAnyClassFlags(CLASS_Abstract))
    {
        Resolved->AddDiagnostic(EKataDiagnosticSeverity::Error, TEXT("AbstractDefinition"), FKataTaskId(),
            FString::Printf(TEXT("Definition class '%s' is abstract and cannot be played"), *DefinitionClass->GetName()));
    }

    TArray<const UKataDefinition*> Chain;
    CollectDefinitionChain(DefinitionClass, Chain);
    UKataResolvedDefinition* Result = ResolveChain(Chain, LeafDefaults, Outer, false, bForEditing);
    Result->SourceClass = DefinitionClass;
    Result->Diagnostics.Append(Resolved->Diagnostics);
    return Result;
}

UKataResolvedDefinition* UKataDefinition::ResolveChain(const TArray<const UKataDefinition*>& Chain,
    const UKataDefinition* EffectiveSettings, UObject* Outer, bool bAssetChain, bool bForEditing)
{
    UKataResolvedDefinition* Resolved = NewObject<UKataResolvedDefinition>(Outer ? Outer : GetTransientPackage());
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

    for (const UKataDefinition* Defaults : Chain)
    {
        UClass* OwningClass = Defaults->GetClass();

        for (const FKataTimelineEntry& Entry : Defaults->TimelineTasks)
        {
            const bool bStamped = bAssetChain || Entry.DeclaringClass != nullptr;
            if (!bAssetChain && bStamped && Entry.DeclaringClass != OwningClass)
            {
                // 부모가 선언한 항목이 자식 기본값에 복사되어 보이는 경우다. 한 번만 처리한다.
                if (Entry.Task != nullptr)
                {
                    const UKataTask* Declared = FindDeclaredTask(Entry.DeclaringClass, Entry.Task->TaskId);
                    if (Declared != nullptr && !ArePropertyValuesIdentical(Declared, Entry.Task))
                    {
                        // 상속 행을 직접 고치면 해석에 반영되지 않는다. 변경은 TaskOverrides로 표현해야 한다.
                        Resolved->AddDiagnostic(EKataDiagnosticSeverity::Warning, TEXT("EditedInheritedEntry"), Entry.Task->TaskId,
                            FString::Printf(TEXT("Class '%s' edited an inherited timeline entry declared by '%s' in place; use a task override instead because the direct edit is ignored"),
                                *Defaults->GetName(), *Entry.DeclaringClass->GetName()));
                    }
                }
                continue;
            }

            if (Entry.Task == nullptr)
            {
                Resolved->AddDiagnostic(EKataDiagnosticSeverity::Warning, TEXT("EmptyTimelineEntry"), FKataTaskId(),
                    FString::Printf(TEXT("Timeline entry declared by '%s' has no task"), *Defaults->GetName()));
                continue;
            }

            const FKataTaskId TaskId = Entry.Task->TaskId;
            if (!TaskId.IsValid())
            {
                Resolved->AddDiagnostic(EKataDiagnosticSeverity::Error, TEXT("MissingTaskId"), FKataTaskId(),
                    FString::Printf(TEXT("Task '%s' declared by '%s' has no stable id"), *Entry.Task->GetDisplayName(), *Defaults->GetName()));
                continue;
            }

            if (WorkingTasks.Contains(TaskId) || RemovedTaskIds.Contains(TaskId))
            {
                if (bStamped)
                {
                    Resolved->AddDiagnostic(EKataDiagnosticSeverity::Error, TEXT("DuplicateTaskId"), TaskId,
                        FString::Printf(TEXT("Task id declared more than once (definition '%s')"), *Defaults->GetName()));
                }
                else
                {
                    Resolved->AddDiagnostic(EKataDiagnosticSeverity::Warning, TEXT("UnstampedTimelineEntry"), TaskId,
                        FString::Printf(TEXT("Timeline entry in '%s' has no declaring class and duplicates an inherited entry; edit it once to stamp ownership"), *Defaults->GetName()));
                }
                continue;
            }

            FKataWorkingTask Working;
            Working.Task = DuplicateObject<UKataTask>(Entry.Task, Resolved, MakeUniqueObjectName(Resolved, Entry.Task->GetClass()));
            Working.DeclarationIndex = NextDeclarationIndex++;
            WorkingTasks.Add(TaskId, Working);
            DeclarationOrder.Add(TaskId);
        }

        for (const FKataTaskOverride& Override : Defaults->TaskOverrides)
        {
            if (!bAssetChain && Override.DeclaringClass != nullptr && Override.DeclaringClass != OwningClass)
            {
                continue;
            }
            if (!Override.TargetTaskId.IsValid())
            {
                Resolved->AddDiagnostic(EKataDiagnosticSeverity::Warning, TEXT("InvalidOverrideTarget"), FKataTaskId(),
                    FString::Printf(TEXT("Override declared by '%s' has no target task id"), *Defaults->GetName()));
                continue;
            }

            FKataWorkingTask* Target = WorkingTasks.Find(Override.TargetTaskId);
            if (Target == nullptr)
            {
                // 부모에서 삭제된 태스크의 오버라이드다. 다른 태스크에 재적용하지 않는다.
                Resolved->AddDiagnostic(EKataDiagnosticSeverity::Warning, TEXT("OrphanedOverride"), Override.TargetTaskId,
                    FString::Printf(TEXT("Override declared by '%s' targets a task that no longer exists"), *Defaults->GetName()));
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
                        FString::Printf(TEXT("Modify override declared by '%s' has no value template"), *Defaults->GetName()));
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

const UKataTask* UKataDefinition::FindInheritedTaskTemplate(const FKataTaskId& TaskId) const
{
    for (UClass* Current = GetClass()->GetSuperClass(); Current != nullptr; Current = Current->GetSuperClass())
    {
        if (!Current->IsChildOf(UKataDefinition::StaticClass()))
        {
            break;
        }
        const UKataDefinition* Defaults = Current->GetDefaultObject<UKataDefinition>();
        if (Defaults == nullptr)
        {
            continue;
        }
        for (const FKataTimelineEntry& Entry : Defaults->TimelineTasks)
        {
            if (Entry.Task != nullptr && Entry.Task->TaskId == TaskId)
            {
                return Entry.Task;
            }
        }
    }
    return nullptr;
}

#if WITH_EDITOR
void UKataDefinition::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
    StampDeclaringClasses();
    TrackOverriddenProperty(PropertyChangedEvent);

    Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void UKataDefinition::StampDeclaringClasses()
{
    UClass* OwningClass = GetClass();

    for (FKataTimelineEntry& Entry : TimelineTasks)
    {
        if (Entry.Task != nullptr)
        {
            Entry.Task->EnsureTaskId();
        }
        if (Entry.DeclaringClass == nullptr)
        {
            Entry.DeclaringClass = OwningClass;
        }
    }

    for (FKataTaskOverride& Override : TaskOverrides)
    {
        if (Override.DeclaringClass == nullptr)
        {
            Override.DeclaringClass = OwningClass;
        }
        SyncOverrideTemplate(Override);
    }
}

void UKataDefinition::SyncOverrideTemplate(FKataTaskOverride& Override)
{
    if (Override.Mode != EKataTimelineChangeMode::Modify || !Override.TargetTaskId.IsValid())
    {
        return;
    }

    const UKataTask* Inherited = FindInheritedTaskTemplate(Override.TargetTaskId);
    if (Inherited == nullptr)
    {
        return;
    }

    if (Override.OverrideValues == nullptr || Override.OverrideValues->GetClass() != Inherited->GetClass())
    {
        // 상속 원본을 그대로 복제해 편집 시작점으로 삼는다. 어떤 프로퍼티가 오버라이드인지는 따로 기록한다.
        Override.OverrideValues = DuplicateObject<UKataTask>(Inherited, this, MakeUniqueObjectName(this, Inherited->GetClass()));
        Override.OverrideValues->SetFlags(RF_Transactional);
        Override.OverriddenProperties.Reset();
    }
}

void UKataDefinition::TrackOverriddenProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
    static const FName OverridesPropertyName = GET_MEMBER_NAME_CHECKED(UKataDefinition, TaskOverrides);
    static const FName OverrideValuesPropertyName = GET_MEMBER_NAME_CHECKED(FKataTaskOverride, OverrideValues);

    const int32 OverrideIndex = PropertyChangedEvent.GetArrayIndex(OverridesPropertyName.ToString());
    if (!TaskOverrides.IsValidIndex(OverrideIndex))
    {
        return;
    }

    bool bReachedOverrideValues = false;
    FName ChangedPropertyName = NAME_None;
    for (auto* Node = PropertyChangedEvent.PropertyChain.GetHead(); Node != nullptr; Node = Node->GetNextNode())
    {
        const FProperty* Property = Node->GetValue();
        if (Property == nullptr)
        {
            continue;
        }
        if (bReachedOverrideValues)
        {
            ChangedPropertyName = Property->GetFName();
            break;
        }
        if (Property->GetFName() == OverrideValuesPropertyName)
        {
            bReachedOverrideValues = true;
        }
    }

    if (!bReachedOverrideValues)
    {
        return;
    }

    if (ChangedPropertyName.IsNone())
    {
        // 오버라이드 사본 자체가 교체된 경우다. 이전 프로퍼티 목록은 더 이상 유효하지 않다.
        TaskOverrides[OverrideIndex].OverriddenProperties.Reset();
        return;
    }

    TaskOverrides[OverrideIndex].OverriddenProperties.AddUnique(ChangedPropertyName);
}

EDataValidationResult UKataDefinition::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    UKataResolvedDefinition* Resolved = ResolveDefinition(GetClass(), GetTransientPackage());
    if (Resolved == nullptr)
    {
        return Result;
    }

    for (const FKataDiagnostic& Diagnostic : Resolved->Diagnostics)
    {
        if (Diagnostic.Severity == EKataDiagnosticSeverity::Info)
        {
            continue;
        }
        // 에셋 경로와 같게 저작이 끝나지 않은 태스크는 경고로만 남긴다.
        const FText Message = FText::FromString(Diagnostic.ToDetailString());
        if (Diagnostic.Severity == EKataDiagnosticSeverity::Error && !Diagnostic.bIncompleteAuthoring)
        {
            Context.AddError(Message);
            Result = EDataValidationResult::Invalid;
        }
        else
        {
            Context.AddWarning(Message);
        }
    }

    return Result;
}
#endif
