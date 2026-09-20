// Copyright Epic Games, Inc. All Rights Reserved.

#include "Testing/KataTestDefinitions.h"

#include "Testing/KataDebugTask.h"

namespace
{
    /** 자식이 같은 항목을 가리킬 수 있도록 고정 GUID를 쓴다. 실제 에셋은 편집기가 자동 발급한다. */
    const FGuid GTaskGuidA(0xA0000001, 0x11114444, 0x88880000, 0x0000000A);
    const FGuid GTaskGuidB(0xA0000001, 0x11114444, 0x88880000, 0x0000000B);
    const FGuid GTaskGuidC(0xA0000001, 0x11114444, 0x88880000, 0x0000000C);
    const FGuid GTaskGuidD(0xA0000001, 0x11114444, 0x88880000, 0x0000000D);
    const FGuid GTaskGuidBad(0xA0000001, 0x11114444, 0x88880000, 0x000000EE);
}

FKataTaskId UKataDefinition_TestBasic::TaskIdA()
{
    return FKataTaskId(GTaskGuidA);
}

FKataTaskId UKataDefinition_TestBasic::TaskIdB()
{
    return FKataTaskId(GTaskGuidB);
}

FKataTaskId UKataDefinition_TestBasic::TaskIdC()
{
    return FKataTaskId(GTaskGuidC);
}

FKataTaskId UKataDefinition_TestOverride::TaskIdD()
{
    return FKataTaskId(GTaskGuidD);
}

UKataDefinition_TestBasic::UKataDefinition_TestBasic()
{
    UKataTask_Debug* TaskA = CreateDefaultSubobject<UKataTask_Debug>(TEXT("KataTestTaskA"));
    TaskA->TaskId = TaskIdA();
    TaskA->TaskName = TEXT("A");
    TaskA->StartTime = 0.0f;
    TaskA->Duration = 1.0f;
    TaskA->Phase = EKataTaskPhase::Gameplay;
    TaskA->DisplayColor = FColor::Green;

    UKataTask_Debug* TaskB = CreateDefaultSubobject<UKataTask_Debug>(TEXT("KataTestTaskB"));
    TaskB->TaskId = TaskIdB();
    TaskB->TaskName = TEXT("B");
    TaskB->StartTime = 0.5f;
    TaskB->Duration = 0.3f;
    TaskB->Phase = EKataTaskPhase::Gameplay;
    TaskB->DisplayColor = FColor::Yellow;

    UKataTask_Debug* TaskC = CreateDefaultSubobject<UKataTask_Debug>(TEXT("KataTestTaskC"));
    TaskC->TaskId = TaskIdC();
    TaskC->TaskName = TEXT("C");
    TaskC->StartTime = 0.5f;
    TaskC->Duration = 0.5f;
    TaskC->Phase = EKataTaskPhase::Animation;
    TaskC->DisplayColor = FColor::Cyan;

    // DeclaringClass를 이 클래스로 기록한다. 자식 CDO에 복사되어 보이는 행은 해석기가 건너뛴다.
    for (UKataTask_Debug* Task : { TaskA, TaskB, TaskC })
    {
        FKataTimelineEntry Entry;
        Entry.Task = Task;
        Entry.DeclaringClass = UKataDefinition_TestBasic::StaticClass();
        TimelineTasks.Add(Entry);
    }
}

UKataDefinition_TestOverride::UKataDefinition_TestOverride()
{
    // B의 StartTime만 바꾼다. Duration, Phase 등 나머지는 부모 값을 그대로 따라야 한다.
    UKataTask_Debug* OverrideB = CreateDefaultSubobject<UKataTask_Debug>(TEXT("KataTestOverrideB"));
    OverrideB->StartTime = 0.8f;

    FKataTaskOverride Override;
    Override.TargetTaskId = UKataDefinition_TestBasic::TaskIdB();
    Override.Mode = EKataTimelineChangeMode::Modify;
    Override.OverriddenProperties.Add(GET_MEMBER_NAME_CHECKED(UKataTask, StartTime));
    Override.OverrideValues = OverrideB;
    Override.DeclaringClass = UKataDefinition_TestOverride::StaticClass();
    TaskOverrides.Add(Override);

    // 자식이 추가한 순간 태스크.
    UKataTask_Debug* TaskD = CreateDefaultSubobject<UKataTask_Debug>(TEXT("KataTestTaskD"));
    TaskD->TaskId = TaskIdD();
    TaskD->TaskName = TEXT("D");
    TaskD->StartTime = 1.2f;
    TaskD->Duration = 0.0f;
    TaskD->Phase = EKataTaskPhase::Presentation;
    TaskD->DisplayColor = FColor::Magenta;

    FKataTimelineEntry Entry;
    Entry.Task = TaskD;
    Entry.DeclaringClass = UKataDefinition_TestOverride::StaticClass();
    TimelineTasks.Add(Entry);
}

UKataDefinition_TestDependency::UKataDefinition_TestDependency()
{
    // C가 B의 완료를 기다리게 한다. Dependencies 프로퍼티만 오버라이드한다.
    UKataTask_Debug* OverrideC = CreateDefaultSubobject<UKataTask_Debug>(TEXT("KataTestOverrideC"));

    FKataTaskDependency Dependency;
    Dependency.TaskId = UKataDefinition_TestBasic::TaskIdB();
    Dependency.Requirement = EKataTaskDependencyRequirement::AfterCompletion;
    OverrideC->Dependencies.Add(Dependency);

    FKataTaskOverride Override;
    Override.TargetTaskId = UKataDefinition_TestBasic::TaskIdC();
    Override.Mode = EKataTimelineChangeMode::Modify;
    Override.OverriddenProperties.Add(GET_MEMBER_NAME_CHECKED(UKataTask, Dependencies));
    Override.OverrideValues = OverrideC;
    Override.DeclaringClass = UKataDefinition_TestDependency::StaticClass();
    TaskOverrides.Add(Override);
}

UKataDefinition_TestLoop::UKataDefinition_TestLoop()
{
    LoopPolicy.bLoop = true;
    LoopPolicy.MaxLoopCount = 2;
    LoopPolicy.MaxIterationsPerTick = 4;
}

UKataDefinition_TestInvalid::UKataDefinition_TestInvalid()
{
    // C를 제거한다.
    FKataTaskOverride RemoveC;
    RemoveC.TargetTaskId = UKataDefinition_TestBasic::TaskIdC();
    RemoveC.Mode = EKataTimelineChangeMode::Remove;
    RemoveC.DeclaringClass = UKataDefinition_TestInvalid::StaticClass();
    TaskOverrides.Add(RemoveC);

    // 이미 제거된 항목을 다시 가리켜 OrphanedOverride 경고를 만든다.
    UKataTask_Debug* StaleOverride = CreateDefaultSubobject<UKataTask_Debug>(TEXT("KataTestStaleOverride"));
    StaleOverride->StartTime = 0.1f;

    FKataTaskOverride Stale;
    Stale.TargetTaskId = UKataDefinition_TestBasic::TaskIdC();
    Stale.Mode = EKataTimelineChangeMode::Modify;
    Stale.OverriddenProperties.Add(GET_MEMBER_NAME_CHECKED(UKataTask, StartTime));
    Stale.OverrideValues = StaleOverride;
    Stale.DeclaringClass = UKataDefinition_TestInvalid::StaticClass();
    TaskOverrides.Add(Stale);

    // 지원하지 않는 업데이트 시점을 요구해 Error를 만든다. 실행은 거절되어야 한다.
    UKataTask_Debug* BadTask = CreateDefaultSubobject<UKataTask_Debug>(TEXT("KataTestBadTask"));
    BadTask->TaskId = FKataTaskId(GTaskGuidBad);
    BadTask->TaskName = TEXT("Bad");
    BadTask->StartTime = 0.2f;
    BadTask->Duration = 0.1f;
    BadTask->UpdateHook = EKataTaskUpdateHook::AfterMeshPose;

    FKataTimelineEntry Entry;
    Entry.Task = BadTask;
    Entry.DeclaringClass = UKataDefinition_TestInvalid::StaticClass();
    TimelineTasks.Add(Entry);
}
