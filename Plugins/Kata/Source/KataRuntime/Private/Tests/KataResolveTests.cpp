#include "Action/KataAction.h"
#include "Action/KataActionTemplate.h"
#include "Action/KataPropertyOverride.h"
#include "Action/KataResolvedAction.h"
#include "Tasks/KataTask_ApplyGameplayEffect.h"
#include "Tasks/KataTask_ApplyLooseTag.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Conditions/KataCondition_Group.h"
#include "Conditions/KataCondition_Tag.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

namespace KataResolveTests
{
    constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

    /**
     * 설정 오류 없이 실행 후보가 되는 태스크를 Owner의 타임라인에 추가한다.
     * 태그를 요구하지 않는 태스크가 필요해 효과 클래스만 채운 Apply Gameplay Effect를 쓴다.
     * 해석 규칙만 확인하므로 프로젝트의 GameplayTag 등록 여부에 의존하지 않는다.
     */
    UKataTask_ApplyGameplayEffect* AddTask(UKataActionAssetBase* Owner, float StartTime, float Duration = 0.0f)
    {
        UKataTask_ApplyGameplayEffect* Task = NewObject<UKataTask_ApplyGameplayEffect>(Owner);
        // 편집기 빌드는 생성할 때 TaskId를 발급하지만, 빌드 구성과 무관하게 유효한 TaskId를 갖도록 직접 넣는다.
        Task->TaskId = FKataTaskId::NewId();
        Task->EffectClass = UGameplayEffect::StaticClass();
        Task->StartTime = StartTime;
        Task->Duration = Duration;
        FKataTimelineEntry Entry;
        Entry.Task = Task;
        Owner->TimelineTasks.Add(Entry);
        return Task;
    }

    UKataActionTemplate* MakeTemplate()
    {
        return NewObject<UKataActionTemplate>(GetTransientPackage());
    }

    UKataAction* MakeChild(UKataActionTemplate* Parent)
    {
        UKataAction* Action = NewObject<UKataAction>(GetTransientPackage());
        Action->ParentAction = Parent;
        return Action;
    }

    void AddOverride(UKataAction* Action, const FKataTaskId& TargetId, EKataTimelineChangeMode Mode)
    {
        FKataTaskOverride& Override = Action->TaskOverrides.AddDefaulted_GetRef();
        Override.TargetTaskId = TargetId;
        Override.Mode = Mode;
    }

    /** Properties만 덮어쓰는 Modify 오버라이드를 추가하고, 테스트가 값을 채울 값 템플릿을 반환한다. */
    template <typename TaskType = UKataTask_ApplyGameplayEffect>
    TaskType* AddModify(UKataAction* Action, const FKataTaskId& TargetId, std::initializer_list<FName> Properties)
    {
        TaskType* Values = NewObject<TaskType>(Action);
        FKataTaskOverride& Override = Action->TaskOverrides.AddDefaulted_GetRef();
        Override.TargetTaskId = TargetId;
        Override.Mode = EKataTimelineChangeMode::Modify;
        Override.OverriddenProperties = Properties;
        Override.OverrideValues = Values;
        return Values;
    }

    void AddDependency(UKataTask* Task, const FKataTaskId& PrerequisiteId)
    {
        FKataTaskDependency& Dependency = Task->Dependencies.AddDefaulted_GetRef();
        Dependency.TaskId = PrerequisiteId;
    }

    /** TaskId가 비어 있으면 대상 태스크와 무관하게 코드와 심각도만 본다. */
    bool HasDiagnostic(const UKataResolvedAction* Resolved, FName Code, EKataDiagnosticSeverity Severity,
        const FKataTaskId& TaskId = FKataTaskId())
    {
        return Resolved->Diagnostics.ContainsByPredicate([&](const FKataDiagnostic& Diagnostic)
        {
            return Diagnostic.Code == Code && Diagnostic.Severity == Severity
                && (!TaskId.IsValid() || Diagnostic.TaskId == TaskId);
        });
    }

    /**
     * Error 진단을 "코드:태스크ID"로 정렬해 이어 붙인다. 두 해석 결과의 오류 판정을 비교할 때 쓴다.
     * Warning·Info는 판정에 영향을 주지 않고, 판정용 해석은 명령 경고를 만들지 않으므로 비교에서 뺀다.
     */
    FString JoinErrors(const UKataResolvedAction* Resolved)
    {
        TArray<FString> Errors;
        for (const FKataDiagnostic& Diagnostic : Resolved->Diagnostics)
        {
            if (Diagnostic.Severity == EKataDiagnosticSeverity::Error)
            {
                Errors.Add(Diagnostic.Code.ToString() + TEXT(":") + Diagnostic.TaskId.ToString());
            }
        }
        Errors.Sort();
        return FString::Join(Errors, TEXT(","));
    }

    bool HasTaskOrder(const UKataResolvedAction* Resolved, std::initializer_list<const UKataTask*> Expected)
    {
        if (Resolved->Tasks.Num() != static_cast<int32>(Expected.size()))
        {
            return false;
        }
        int32 Index = 0;
        for (const UKataTask* Task : Expected)
        {
            if (Resolved->Tasks[Index++]->TaskId != Task->TaskId)
            {
                return false;
            }
        }
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataResolveNoParentTest, "Kata.Runtime.Resolve.NoParent", KataResolveTests::Flags)
bool FKataResolveNoParentTest::RunTest(const FString& Parameters)
{
    using namespace KataResolveTests;
    UKataAction* Action = NewObject<UKataAction>(GetTransientPackage());
    Action->CooldownPolicy.bEnabled = true;
    Action->CooldownPolicy.Duration = 2.0f;
    UKataTask* Late = AddTask(Action, 0.5f);
    UKataTask* Early = AddTask(Action, 0.0f);

    UKataResolvedAction* Resolved = Action->Resolve(GetTransientPackage());
    TestFalse(TEXT("Standalone action resolves without errors"), Resolved->HasErrors());
    TestTrue(TEXT("Action is its own source"), Resolved->SourceAction == Action);
    TestEqual(TEXT("Own settings are copied"), Resolved->CooldownPolicy.Duration, 2.0f);
    TestTrue(TEXT("Tasks are sorted by start time"), HasTaskOrder(Resolved, { Early, Late }));
    if (Resolved->Tasks.Num() == 2)
    {
        TestTrue(TEXT("Resolved task is a copy owned by the result"),
            Resolved->Tasks[0] != Early && Resolved->Tasks[0]->GetOuter() == Resolved);
    }

    UKataActionTemplate* Template = MakeTemplate();
    AddTask(Template, 0.0f);
    UKataResolvedAction* TemplateResolved = Template->Resolve(GetTransientPackage());
    TestNull(TEXT("Template result has no playable source"), TemplateResolved->SourceAction.Get());
    TestEqual(TEXT("Template resolves its own tasks"), TemplateResolved->Tasks.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataResolveSettingOverrideTest, "Kata.Runtime.Resolve.SettingOverride", KataResolveTests::Flags)
bool FKataResolveSettingOverrideTest::RunTest(const FString& Parameters)
{
    using namespace KataResolveTests;
    UKataActionTemplate* Template = MakeTemplate();
    Template->CooldownPolicy.bEnabled = true;
    Template->CooldownPolicy.Duration = 2.0f;
    UKataCondition_Tag* ParentCondition = NewObject<UKataCondition_Tag>(Template);
    Template->StartCondition = ParentCondition;

    UKataAction* Action = MakeChild(Template);
    // 자식 에셋에 남은 값이라도 OverriddenSettings에 없으면 부모 값을 따라야 한다.
    Action->CooldownPolicy.bEnabled = false;
    Action->CooldownPolicy.Duration = 5.0f;
    Action->OverriddenSettings.Add(TEXT("CooldownPolicy.Duration"));

    UKataResolvedAction* Resolved = Action->Resolve(GetTransientPackage());
    TestTrue(TEXT("Unlisted struct field follows parent"), Resolved->CooldownPolicy.bEnabled);
    TestEqual(TEXT("Listed struct field uses child value"), Resolved->CooldownPolicy.Duration, 5.0f);
    TestEqual(TEXT("Parent asset is not modified"), Template->CooldownPolicy.Duration, 2.0f);
    TestTrue(TEXT("Inherited condition is a copy owned by the result"),
        Resolved->StartCondition != nullptr && Resolved->StartCondition != ParentCondition && Resolved->StartCondition->GetOuter() == Resolved);

    // 조건 객체는 필드 단위가 아니라 객체 전체를 하나의 값으로 덮어쓴다.
    UKataCondition_Tag* ChildCondition = NewObject<UKataCondition_Tag>(Action);
    ChildCondition->bInvert = true;
    Action->StartCondition = ChildCondition;
    Action->OverriddenSettings.Add(TEXT("StartCondition"));
    Resolved = Action->Resolve(GetTransientPackage());
    TestTrue(TEXT("Overridden condition replaces the parent object"), Resolved->StartCondition != nullptr && Resolved->StartCondition->bInvert);
    TestTrue(TEXT("Overridden condition is copied, not shared"), Resolved->StartCondition != ChildCondition);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataResolveEqualOverrideTest, "Kata.Runtime.Resolve.EqualOverrideKept", KataResolveTests::Flags)
bool FKataResolveEqualOverrideTest::RunTest(const FString& Parameters)
{
    using namespace KataResolveTests;
    UKataActionTemplate* Template = MakeTemplate();
    Template->CooldownPolicy.bEnabled = true;
    Template->CooldownPolicy.Duration = 2.0f;
    UKataTask* ParentTask = AddTask(Template, 0.2f);

    // 부모와 같은 값으로 오버라이드를 기록한다.
    UKataAction* Action = MakeChild(Template);
    Action->CooldownPolicy = Template->CooldownPolicy;
    Action->OverriddenSettings.Add(TEXT("CooldownPolicy.Duration"));
    AddModify(Action, ParentTask->TaskId, { TEXT("StartTime") })->StartTime = 0.2f;
    Action->Resolve(GetTransientPackage());
    TestEqual(TEXT("Resolve does not prune setting overrides equal to the parent"), Action->OverriddenSettings.Num(), 1);
    TestEqual(TEXT("Resolve does not prune task overrides equal to the parent"), Action->TaskOverrides[0].OverriddenProperties.Num(), 1);

    // 값이 같았던 오버라이드도 남아 있으므로 부모가 바뀐 뒤에도 자식 값을 지킨다.
    Template->CooldownPolicy.Duration = 4.0f;
    ParentTask->StartTime = 0.7f;
    ParentTask->Duration = 0.3f;
    UKataResolvedAction* Resolved = Action->Resolve(GetTransientPackage());
    TestEqual(TEXT("Equal setting override keeps child value after parent change"), Resolved->CooldownPolicy.Duration, 2.0f);
    const UKataTask* Task = Resolved->FindTask(ParentTask->TaskId);
    if (!TestNotNull(TEXT("Overridden task is resolved"), Task))
    {
        return false;
    }
    TestEqual(TEXT("Equal task override keeps child start time"), Task->StartTime, 0.2f);
    TestEqual(TEXT("Unlisted task property follows parent change"), Task->Duration, 0.3f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataResolveTaskModesTest, "Kata.Runtime.Resolve.TaskModes", KataResolveTests::Flags)
bool FKataResolveTaskModesTest::RunTest(const FString& Parameters)
{
    using namespace KataResolveTests;
    UKataActionTemplate* Template = MakeTemplate();
    UKataTask* Removed = AddTask(Template, 0.0f);
    UKataTask* Disabled = AddTask(Template, 0.1f);
    UKataTask* Modified = AddTask(Template, 0.2f);
    UKataTask* Reenabled = AddTask(Template, 0.3f);

    UKataAction* Action = MakeChild(Template);
    AddOverride(Action, Removed->TaskId, EKataTimelineChangeMode::Remove);
    AddOverride(Action, Disabled->TaskId, EKataTimelineChangeMode::Disable);
    UKataTask_ApplyGameplayEffect* Values = AddModify(Action, Modified->TaskId, { TEXT("StartTime") });
    Values->StartTime = 0.4f;
    // 목록에 없는 프로퍼티라 적용되지 않아야 한다.
    Values->Duration = 9.0f;
    // 같은 에셋 안에서 Disable 뒤에 bEnabled를 Modify하면 다시 실행 대상이 된다.
    AddOverride(Action, Reenabled->TaskId, EKataTimelineChangeMode::Disable);
    AddModify(Action, Reenabled->TaskId, { GET_MEMBER_NAME_CHECKED(UKataTask, bEnabled) })->bEnabled = true;

    UKataResolvedAction* Resolved = Action->Resolve(GetTransientPackage());
    TestFalse(TEXT("Task modes resolve without errors"), Resolved->HasErrors());
    TestNull(TEXT("Removed task does not run"), Resolved->FindTask(Removed->TaskId));
    TestNull(TEXT("Disabled task does not run"), Resolved->FindTask(Disabled->TaskId));
    TestTrue(TEXT("Disabled task is reported as info"),
        HasDiagnostic(Resolved, TEXT("TaskDisabled"), EKataDiagnosticSeverity::Info, Disabled->TaskId));
    TestFalse(TEXT("Removed task leaves no disabled record"),
        HasDiagnostic(Resolved, TEXT("TaskDisabled"), EKataDiagnosticSeverity::Info, Removed->TaskId));
    TestTrue(TEXT("Re-enabled task runs before the moved task"), HasTaskOrder(Resolved, { Reenabled, Modified }));
    if (const UKataTask* ModifiedTask = Resolved->FindTask(Modified->TaskId))
    {
        TestEqual(TEXT("Listed property is overridden"), ModifiedTask->StartTime, 0.4f);
        TestEqual(TEXT("Unlisted property keeps parent value"), ModifiedTask->Duration, 0.0f);
    }
    TestEqual(TEXT("Parent task is not modified"), Modified->StartTime, 0.2f);

    // 편집용 결과는 비활성 항목도 고칠 수 있도록 남기지만 삭제한 항목은 남기지 않는다.
    UKataResolvedAction* Editing = Action->Resolve(GetTransientPackage(), true);
    TestNull(TEXT("Editing result omits removed task"), Editing->FindTask(Removed->TaskId));
    const UKataTask* EditingDisabled = Editing->FindTask(Disabled->TaskId);
    TestTrue(TEXT("Editing result keeps disabled task as disabled"), EditingDisabled != nullptr && !EditingDisabled->bEnabled);
    TestTrue(TEXT("Editing result does not disable the parent task"), Disabled->bEnabled);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataResolveParentAddedTaskTest, "Kata.Runtime.Resolve.ParentAddedTask", KataResolveTests::Flags)
bool FKataResolveParentAddedTaskTest::RunTest(const FString& Parameters)
{
    using namespace KataResolveTests;
    UKataActionTemplate* Template = MakeTemplate();
    UKataTask* ParentFirst = AddTask(Template, 0.2f);
    UKataAction* Action = MakeChild(Template);
    UKataTask* ChildTie = AddTask(Action, 0.2f);
    UKataTask* ChildFirst = AddTask(Action, 0.0f);

    // 자식을 만든 뒤 부모에 추가한 태스크도 자식 해석에 들어간다. 부모 항목이 자식 항목보다 먼저 선언된 것으로 본다.
    UKataTask* ParentAdded = AddTask(Template, 0.1f);
    UKataResolvedAction* Resolved = Action->Resolve(GetTransientPackage());
    TestFalse(TEXT("Parent addition resolves without errors"), Resolved->HasErrors());
    TestTrue(TEXT("Start time first, then parent-before-child declaration"),
        HasTaskOrder(Resolved, { ChildFirst, ParentAdded, ParentFirst, ChildTie }));

    // 시작 시각과 Phase가 같으면 선언 순서보다 OrderHint를 먼저 본다.
    ChildTie->OrderHint = -1;
    Resolved = Action->Resolve(GetTransientPackage());
    TestTrue(TEXT("OrderHint precedes declaration order"),
        HasTaskOrder(Resolved, { ChildFirst, ParentAdded, ChildTie, ParentFirst }));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataResolveDiagnosticsTest, "Kata.Runtime.Resolve.Diagnostics", KataResolveTests::Flags)
bool FKataResolveDiagnosticsTest::RunTest(const FString& Parameters)
{
    using namespace KataResolveTests;
    UKataActionTemplate* Template = MakeTemplate();
    UKataTask* Kept = AddTask(Template, 0.0f);
    UKataTask* Deleted = AddTask(Template, 0.5f);

    // 부모에서 지운 태스크를 가리키는 오버라이드는 경고만 남기고 다른 태스크에 다시 적용하지 않는다.
    UKataAction* Orphaned = MakeChild(Template);
    AddModify(Orphaned, Deleted->TaskId, { TEXT("StartTime") })->StartTime = 0.9f;
    Template->TimelineTasks.RemoveAll([Deleted](const FKataTimelineEntry& Entry) { return Entry.Task == Deleted; });
    UKataResolvedAction* Resolved = Orphaned->Resolve(GetTransientPackage());
    TestTrue(TEXT("Override of deleted task is orphaned"),
        HasDiagnostic(Resolved, TEXT("OrphanedOverride"), EKataDiagnosticSeverity::Warning, Deleted->TaskId));
    TestFalse(TEXT("Orphaned override is not an error"), Resolved->HasErrors());
    const UKataTask* KeptTask = Resolved->FindTask(Kept->TaskId);
    TestTrue(TEXT("Orphaned value is not reapplied to another task"), KeptTask != nullptr && FMath::IsNearlyZero(KeptTask->StartTime));

    // 대상과 상속 관계가 없는 태스크 타입의 오버라이드 값은 적용하지 않는다.
    UKataAction* Mismatched = MakeChild(Template);
    AddModify<UKataTask_ApplyLooseTag>(Mismatched, Kept->TaskId, { TEXT("StartTime") })->StartTime = 0.9f;
    Resolved = Mismatched->Resolve(GetTransientPackage());
    TestTrue(TEXT("Unrelated value class is an error"),
        HasDiagnostic(Resolved, TEXT("OverrideClassMismatch"), EKataDiagnosticSeverity::Error, Kept->TaskId));
    KeptTask = Resolved->FindTask(Kept->TaskId);
    TestTrue(TEXT("Mismatched value is not applied"), KeptTask != nullptr && FMath::IsNearlyZero(KeptTask->StartTime));

    // TaskId는 오버라이드할 수 없지만 같은 오버라이드의 다른 프로퍼티는 적용한다.
    UKataAction* IdOverride = MakeChild(Template);
    UKataTask_ApplyGameplayEffect* Values = AddModify(IdOverride, Kept->TaskId,
        { GET_MEMBER_NAME_CHECKED(UKataTask, TaskId), TEXT("StartTime") });
    Values->TaskId = FKataTaskId::NewId();
    Values->StartTime = 0.6f;
    Resolved = IdOverride->Resolve(GetTransientPackage());
    TestTrue(TEXT("Task id override is ignored with a warning"),
        HasDiagnostic(Resolved, TEXT("TaskIdOverrideIgnored"), EKataDiagnosticSeverity::Warning, Kept->TaskId));
    TestFalse(TEXT("Ignored task id is not an error"), Resolved->HasErrors());
    KeptTask = Resolved->FindTask(Kept->TaskId);
    TestTrue(TEXT("Task keeps its id and other overridden values apply"), KeptTask != nullptr && FMath::IsNearlyEqual(KeptTask->StartTime, 0.6f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataResolveStartCheckParityTest, "Kata.Runtime.Resolve.StartCheckParity", KataResolveTests::Flags)
bool FKataResolveStartCheckParityTest::RunTest(const FString& Parameters)
{
    using namespace KataResolveTests;
    // 판정용 해석은 명령을 다루지 않고 Modify 대상 태스크만 복제한다. 오류 판정은 실행용 해석과 같아야 한다.
    TArray<TPair<FString, UKataAction*>> Cases;

    UKataActionTemplate* CleanTemplate = MakeTemplate();
    UKataCondition_Tag* CleanCondition = NewObject<UKataCondition_Tag>(CleanTemplate);
    CleanTemplate->StartCondition = CleanCondition;
    UKataTask* CleanTask = AddTask(CleanTemplate, 0.2f);
    UKataAction* Clean = MakeChild(CleanTemplate);
    AddModify(Clean, CleanTask->TaskId, { TEXT("StartTime") })->StartTime = 0.4f;
    Cases.Emplace(TEXT("Clean"), Clean);

    UKataAction* Mismatched = MakeChild(CleanTemplate);
    AddModify<UKataTask_ApplyLooseTag>(Mismatched, CleanTask->TaskId, { TEXT("StartTime") });
    Cases.Emplace(TEXT("ClassMismatch"), Mismatched);

    // 없는 태스크, 비활성 태스크, 자신보다 늦게 시작하는 태스크에 대한 의존은 모두 오류다.
    UKataAction* Dependencies = NewObject<UKataAction>(GetTransientPackage());
    UKataTask* Inactive = AddTask(Dependencies, 0.0f);
    Inactive->bEnabled = false;
    UKataTask* LateStart = AddTask(Dependencies, 0.8f);
    UKataTask* Dependent = AddTask(Dependencies, 0.5f);
    AddDependency(Dependent, FKataTaskId::NewId());
    AddDependency(Dependent, Inactive->TaskId);
    AddDependency(Dependent, LateStart->TaskId);
    Cases.Emplace(TEXT("Dependencies"), Dependencies);

    // 설정이 덜 된 태스크는 실행에서 빠지므로 남은 타임라인 길이가 0이 되어 반복도 오류가 된다.
    UKataAction* Settings = NewObject<UKataAction>(GetTransientPackage());
    AddTask(Settings, 0.0f)->EffectClass = nullptr;
    Settings->LoopPolicy.bLoop = true;
    Settings->CooldownPolicy.bEnabled = true;
    Cases.Emplace(TEXT("Settings"), Settings);

    for (const TPair<FString, UKataAction*>& Case : Cases)
    {
        UKataResolvedAction* Full = Case.Value->Resolve(GetTransientPackage());
        UKataResolvedAction* Check = Case.Value->ResolveForStartCheck(GetTransientPackage());
        TestEqual(*FString::Printf(TEXT("%s: start check reports the same errors"), *Case.Key), JoinErrors(Check), JoinErrors(Full));
        TestTrue(*FString::Printf(TEXT("%s: only the start check result is marked"), *Case.Key), Check->bStartCheckOnly && !Full->bStartCheckOnly);
        TestEqual(*FString::Printf(TEXT("%s: start check result holds no tasks"), *Case.Key), Check->Tasks.Num(), 0);
    }

    // 비교한 구성이 실제로 오류를 만드는지 확인해 비어 있는 비교가 통과하지 않게 한다.
    UKataResolvedAction* DependencyResult = Dependencies->Resolve(GetTransientPackage());
    TestTrue(TEXT("Missing dependency is reported"), HasDiagnostic(DependencyResult, TEXT("MissingDependency"), EKataDiagnosticSeverity::Error));
    TestTrue(TEXT("Inactive dependency is reported"), HasDiagnostic(DependencyResult, TEXT("DependencyOnInactiveTask"), EKataDiagnosticSeverity::Error));
    TestTrue(TEXT("Dependency time conflict is reported"), HasDiagnostic(DependencyResult, TEXT("DependencyTimeConflict"), EKataDiagnosticSeverity::Error));
    UKataResolvedAction* SettingsResult = Settings->Resolve(GetTransientPackage());
    TestTrue(TEXT("Incomplete task is reported"), HasDiagnostic(SettingsResult, TEXT("MissingEffectClass"), EKataDiagnosticSeverity::Error));
    TestTrue(TEXT("Zero-length loop is reported"), HasDiagnostic(SettingsResult, TEXT("ZeroLengthLoop"), EKataDiagnosticSeverity::Error));
    TestTrue(TEXT("Invalid cooldown is reported"), HasDiagnostic(SettingsResult, TEXT("InvalidCooldownDuration"), EKataDiagnosticSeverity::Error));
    TestFalse(TEXT("Clean case has no errors"), Clean->Resolve(GetTransientPackage())->HasErrors());

    // 판정용 해석은 Modify 대상만 복제하고, 조건은 복제하지 않고 원본을 가리킨다는 계약을 따른다.
    UKataResolvedAction* CleanCheck = Clean->ResolveForStartCheck(GetTransientPackage());
    TestEqual(TEXT("Start check does not modify the parent task"), CleanTask->StartTime, 0.2f);
    TestTrue(TEXT("Start check reads the source condition"), CleanCheck->StartCondition == CleanCondition);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKataCopyOverriddenPropertyTest, "Kata.Runtime.Resolve.CopyOverriddenProperty", KataResolveTests::Flags)
bool FKataCopyOverriddenPropertyTest::RunTest(const FString& Parameters)
{
    using namespace KataResolveTests;
    UKataAction* Source = NewObject<UKataAction>(GetTransientPackage());
    UKataAction* Dest = NewObject<UKataAction>(GetTransientPackage());
    FString Error;
    TestFalse(TEXT("Null owner is rejected"), KataPropertyOverride::CopyOverriddenProperty(nullptr, Source, TEXT("LoopPolicy"), Error));
    TestFalse(TEXT("Rejection reports a reason"), Error.IsEmpty());
    TestFalse(TEXT("Empty path is rejected"), KataPropertyOverride::CopyOverriddenProperty(Dest, Source, NAME_None, Error));
    TestFalse(TEXT("Unknown property is rejected"), KataPropertyOverride::CopyOverriddenProperty(Dest, Source, TEXT("MissingProperty"), Error));
    TestFalse(TEXT("Property missing on destination class is rejected"),
        KataPropertyOverride::CopyOverriddenProperty(MakeTemplate(), Source, TEXT("ParentAction"), Error));
    TestFalse(TEXT("Path through an object property is rejected"),
        KataPropertyOverride::CopyOverriddenProperty(Dest, Source, TEXT("StartCondition.bInvert"), Error));

    // 구조체 필드 경로는 그 필드만 복사하고 같은 구조체의 다른 필드는 건드리지 않는다.
    Source->LoopPolicy.bLoop = true;
    Source->LoopPolicy.MaxLoopCount = 3;
    TestTrue(TEXT("Nested field copies"), KataPropertyOverride::CopyOverriddenProperty(Dest, Source, TEXT("LoopPolicy.MaxLoopCount"), Error));
    TestEqual(TEXT("Nested field takes source value"), Dest->LoopPolicy.MaxLoopCount, 3);
    TestFalse(TEXT("Sibling field is untouched"), Dest->LoopPolicy.bLoop);

    // Instanced 객체는 기본적으로 대상 소유 사본을 만들고, 공유를 허용하면 원본을 그대로 가리킨다.
    UKataCondition_Tag* SourceCondition = NewObject<UKataCondition_Tag>(Source);
    SourceCondition->bInvert = true;
    Source->StartCondition = SourceCondition;
    TestTrue(TEXT("Instanced object copies"), KataPropertyOverride::CopyOverriddenProperty(Dest, Source, TEXT("StartCondition"), Error));
    TestTrue(TEXT("Instanced object is duplicated under the destination"),
        Dest->StartCondition != nullptr && Dest->StartCondition != SourceCondition && Dest->StartCondition->GetOuter() == Dest
        && Dest->StartCondition->bInvert);
    UKataAction* Shared = NewObject<UKataAction>(GetTransientPackage());
    TestTrue(TEXT("Sharing copy succeeds"), KataPropertyOverride::CopyOverriddenProperty(Shared, Source, TEXT("StartCondition"), Error, false));
    TestTrue(TEXT("Sharing copy keeps the source object"), Shared->StartCondition == SourceCondition);

    // 객체 배열은 원본 길이를 따르고 원소마다 대상 소유 사본을 만든다. 빈 원소는 빈 채로 둔다.
    UKataCondition_Group* SourceGroup = NewObject<UKataCondition_Group>(GetTransientPackage());
    SourceGroup->Conditions.Add(NewObject<UKataCondition_Tag>(SourceGroup));
    SourceGroup->Conditions.Add(nullptr);
    UKataCondition_Group* DestGroup = NewObject<UKataCondition_Group>(GetTransientPackage());
    for (int32 Index = 0; Index < 3; ++Index)
    {
        DestGroup->Conditions.Add(NewObject<UKataCondition_Tag>(DestGroup));
    }
    TestTrue(TEXT("Object array copies"), KataPropertyOverride::CopyOverriddenProperty(DestGroup, SourceGroup, TEXT("Conditions"), Error));
    if (TestEqual(TEXT("Object array takes the source length"), DestGroup->Conditions.Num(), 2))
    {
        TestTrue(TEXT("Object array element is duplicated under the destination"),
            DestGroup->Conditions[0] != nullptr && DestGroup->Conditions[0] != SourceGroup->Conditions[0]
            && DestGroup->Conditions[0]->GetOuter() == DestGroup);
        TestNull(TEXT("Null element stays null"), DestGroup->Conditions[1].Get());
    }

    // 구조체 배열은 필드로 직접 가진 Instanced 객체만 대상 소유 사본으로 바꾼다.
    UKataTask* SourceTask = AddTask(Source, 0.25f);
    TestTrue(TEXT("Struct array copies"), KataPropertyOverride::CopyOverriddenProperty(Dest, Source, TEXT("TimelineTasks"), Error));
    if (TestEqual(TEXT("Struct array takes the source length"), Dest->TimelineTasks.Num(), 1))
    {
        const UKataTask* Copied = Dest->TimelineTasks[0].Task;
        TestTrue(TEXT("Instanced struct field is duplicated under the destination"),
            Copied != nullptr && Copied != SourceTask && Copied->GetOuter() == Dest);
        TestTrue(TEXT("Duplicated task keeps its id"), Copied != nullptr && Copied->TaskId == SourceTask->TaskId);
    }
    return true;
}

#endif // 개발용 자동화 테스트
