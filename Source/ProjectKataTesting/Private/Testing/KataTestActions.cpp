// Copyright Epic Games, Inc. All Rights Reserved.

#include "Testing/KataTestActions.h"

#include "Action/KataAction.h"
#include "Testing/KataDebugTask.h"

namespace
{
    /** 자식이 같은 항목을 가리킬 수 있도록 고정 GUID를 쓴다. 실제 에셋은 편집기가 자동 발급한다. */
    const FGuid GTaskGuidA(0xA0000001, 0x11114444, 0x88880000, 0x0000000A);
    const FGuid GTaskGuidB(0xA0000001, 0x11114444, 0x88880000, 0x0000000B);
    const FGuid GTaskGuidC(0xA0000001, 0x11114444, 0x88880000, 0x0000000C);
    const FGuid GTaskGuidD(0xA0000001, 0x11114444, 0x88880000, 0x0000000D);
    const FGuid GTaskGuidBad(0xA0000001, 0x11114444, 0x88880000, 0x000000EE);

    /** 디버그 태스크를 만들어 액션의 타임라인 항목으로 넣는다. Outer는 소유 액션이다. */
    UKataTask_Debug* AddDebugTask(UKataAction* Action, const FKataTaskId& TaskId, const TCHAR* Name,
        float StartTime, float Duration, EKataTaskPhase Phase, FColor DisplayColor)
    {
        UKataTask_Debug* Task = NewObject<UKataTask_Debug>(Action);
        Task->TaskId = TaskId;
        Task->TaskName = Name;
        Task->StartTime = StartTime;
        Task->Duration = Duration;
        Task->Phase = Phase;
        Task->DisplayColor = DisplayColor;

        Action->TimelineTasks.AddDefaulted_GetRef().Task = Task;
        return Task;
    }

    /** 부모 항목의 일부 프로퍼티만 바꾸는 오버라이드를 추가하고 오버라이드 값의 사본을 반환한다. */
    UKataTask_Debug* AddModifyOverride(UKataAction* Action, const FKataTaskId& TargetTaskId, FName OverriddenProperty)
    {
        UKataTask_Debug* Values = NewObject<UKataTask_Debug>(Action);

        FKataTaskOverride Override;
        Override.TargetTaskId = TargetTaskId;
        Override.Mode = EKataTimelineChangeMode::Modify;
        Override.OverriddenProperties.Add(OverriddenProperty);
        Override.OverrideValues = Values;
        Action->TaskOverrides.Add(Override);

        return Values;
    }

    UKataAction* MakeAction(UObject* Outer, UKataAction* Parent)
    {
        UKataAction* Action = NewObject<UKataAction>(Outer ? Outer : GetTransientPackage());
        Action->ParentAction = Parent;
        return Action;
    }

    /** A·B·C를 선언하는 루트 액션. 다른 테스트 액션이 모두 이 액션을 부모로 삼는다. */
    UKataAction* MakeBasic(UObject* Outer)
    {
        UKataAction* Action = MakeAction(Outer, nullptr);
        AddDebugTask(Action, KataTestActions::TaskIdA(), TEXT("A"), 0.0f, 1.0f, EKataTaskPhase::Gameplay, FColor::Green);
        AddDebugTask(Action, KataTestActions::TaskIdB(), TEXT("B"), 0.5f, 0.3f, EKataTaskPhase::Gameplay, FColor::Yellow);
        AddDebugTask(Action, KataTestActions::TaskIdC(), TEXT("C"), 0.5f, 0.5f, EKataTaskPhase::Animation, FColor::Cyan);
        return Action;
    }

    /** B의 StartTime만 바꾸고 순간 태스크 D를 추가한다. 나머지 값은 부모를 그대로 따라야 한다. */
    UKataAction* MakeOverride(UObject* Outer)
    {
        UKataAction* Action = MakeAction(Outer, MakeBasic(Outer));

        UKataTask_Debug* Values = AddModifyOverride(Action, KataTestActions::TaskIdB(),
            GET_MEMBER_NAME_CHECKED(UKataTask, StartTime));
        Values->StartTime = 0.8f;

        AddDebugTask(Action, KataTestActions::TaskIdD(), TEXT("D"), 1.2f, 0.0f, EKataTaskPhase::Presentation, FColor::Magenta);
        return Action;
    }

    /** C가 B의 완료를 기다리게 한다. C는 시작 시각 0.5가 아니라 B가 끝나는 0.8에 시작해야 한다. */
    UKataAction* MakeDependency(UObject* Outer)
    {
        UKataAction* Action = MakeAction(Outer, MakeBasic(Outer));

        UKataTask_Debug* Values = AddModifyOverride(Action, KataTestActions::TaskIdC(),
            GET_MEMBER_NAME_CHECKED(UKataTask, Dependencies));

        FKataTaskDependency Dependency;
        Dependency.TaskId = KataTestActions::TaskIdB();
        Dependency.Requirement = EKataTaskDependencyRequirement::AfterCompletion;
        Values->Dependencies.Add(Dependency);

        return Action;
    }

    /** 두 번 반복하고 종료한다. */
    UKataAction* MakeLoop(UObject* Outer)
    {
        UKataAction* Action = MakeAction(Outer, MakeBasic(Outer));

        Action->LoopPolicy.bLoop = true;
        Action->LoopPolicy.MaxLoopCount = 2;

        // 자식의 고유 설정은 OverriddenSettings에 등록해야 병합 결과에 남는다.
        // 클래스 상속과 달리 에셋 상속은 등록되지 않은 경로를 부모 값으로 덮는다.
        Action->OverriddenSettings.AddUnique(GET_MEMBER_NAME_CHECKED(UKataAction, LoopPolicy));

        return Action;
    }

    /** OrphanedOverride 경고와 미지원 업데이트 시점 Error를 함께 만든다. 실행은 거절되어야 한다. */
    UKataAction* MakeInvalid(UObject* Outer)
    {
        UKataAction* Action = MakeAction(Outer, MakeBasic(Outer));

        // C를 제거한다.
        FKataTaskOverride RemoveC;
        RemoveC.TargetTaskId = KataTestActions::TaskIdC();
        RemoveC.Mode = EKataTimelineChangeMode::Remove;
        Action->TaskOverrides.Add(RemoveC);

        // 이미 제거된 항목을 다시 가리켜 OrphanedOverride 경고를 만든다.
        UKataTask_Debug* Stale = AddModifyOverride(Action, KataTestActions::TaskIdC(),
            GET_MEMBER_NAME_CHECKED(UKataTask, StartTime));
        Stale->StartTime = 0.1f;

        // 지원하지 않는 업데이트 시점을 요구해 Error를 만든다.
        UKataTask_Debug* BadTask = AddDebugTask(Action, FKataTaskId(GTaskGuidBad), TEXT("Bad"),
            0.2f, 0.1f, EKataTaskPhase::Gameplay, FColor::Red);
        BadTask->UpdateHook = EKataTaskUpdateHook::AfterMeshPose;

        return Action;
    }
}

namespace KataTestActions
{
    FKataTaskId TaskIdA()
    {
        return FKataTaskId(GTaskGuidA);
    }

    FKataTaskId TaskIdB()
    {
        return FKataTaskId(GTaskGuidB);
    }

    FKataTaskId TaskIdC()
    {
        return FKataTaskId(GTaskGuidC);
    }

    FKataTaskId TaskIdD()
    {
        return FKataTaskId(GTaskGuidD);
    }

    UKataAction* Make(EKataTestAction Kind, UObject* Outer)
    {
        switch (Kind)
        {
        case EKataTestAction::Basic:      return MakeBasic(Outer);
        case EKataTestAction::Override:   return MakeOverride(Outer);
        case EKataTestAction::Dependency: return MakeDependency(Outer);
        case EKataTestAction::Loop:       return MakeLoop(Outer);
        case EKataTestAction::Invalid:    return MakeInvalid(Outer);
        case EKataTestAction::None:
        default:
            return nullptr;
        }
    }

    EKataTestAction ParseName(const FString& Name)
    {
        for (EKataTestAction Kind : GetAllKinds())
        {
            if (GetName(Kind).Equals(Name, ESearchCase::IgnoreCase))
            {
                return Kind;
            }
        }
        return EKataTestAction::None;
    }

    FString GetName(EKataTestAction Kind)
    {
        return StaticEnum<EKataTestAction>()->GetNameStringByValue(static_cast<int64>(Kind));
    }

    TArray<EKataTestAction> GetAllKinds()
    {
        return {
            EKataTestAction::Basic,
            EKataTestAction::Override,
            EKataTestAction::Dependency,
            EKataTestAction::Loop,
            EKataTestAction::Invalid
        };
    }
}
