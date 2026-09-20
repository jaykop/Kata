// Copyright Epic Games, Inc. All Rights Reserved.

#include "Testing/KataTestLogging.h"

#include "Action/KataAction.h"
#include "Action/KataResolvedAction.h"
#include "Action/KataTask.h"
#include "EngineUtils.h"
#include "GameplayEffect.h"
#include "HAL/IConsoleManager.h"
#include "KataCondition.h"
#include "KataRuntimeLog.h"
#include "Runtime/KataActionInstance.h"
#include "Runtime/KataComponent.h"
#include "Testing/KataTestActions.h"
#include "Testing/KataTestActor.h"
#include "UObject/Class.h"
#include "UObject/UObjectIterator.h"

namespace KataTestLogging
{
    UKataAction* FindAction(const FString& NameOrPath, UObject* Outer)
    {
        if (NameOrPath.IsEmpty())
        {
            return nullptr;
        }

        // 내장 테스트 액션 이름을 우선 조회한다.
        const EKataTestAction Kind = KataTestActions::ParseName(NameOrPath);
        if (Kind != EKataTestAction::None)
        {
            return KataTestActions::Make(Kind, Outer);
        }

        // 내장 이름이 아니면 Kata Action 에셋 경로로 불러온다.
        return LoadObject<UKataAction>(nullptr, *NameOrPath);
    }

    void DumpResolvedAction(UKataAction* Action)
    {
        if (Action == nullptr)
        {
            UE_LOG(LogKata, Error, TEXT("Kata.Resolve: no action"));
            return;
        }

        UKataResolvedAction* Resolved = Action->Resolve(GetTransientPackage(), false);
        if (Resolved == nullptr)
        {
            UE_LOG(LogKata, Error, TEXT("Kata.Resolve: failed to resolve '%s'"), *Action->GetName());
            return;
        }

        UE_LOG(LogKata, Log, TEXT("=== Resolved Kata: %s ==="), *Action->GetName());
        UE_LOG(LogKata, Log, TEXT("  KataTags              : %s"), *Resolved->KataTags.ToStringSimple());
        UE_LOG(LogKata, Log, TEXT("  ActivationRequiredTags: %s"), *Resolved->ActivationRequiredTags.ToStringSimple());
        UE_LOG(LogKata, Log, TEXT("  ActivationBlockedTags : %s"), *Resolved->ActivationBlockedTags.ToStringSimple());
        UE_LOG(LogKata, Log, TEXT("  ActiveGrantedTags     : %s"), *Resolved->ActiveGrantedTags.ToStringSimple());
        UE_LOG(LogKata, Log, TEXT("  BlockedKataTags       : %s"), *Resolved->BlockingPolicy.BlockedKataTags.ToStringSimple());
        UE_LOG(LogKata, Log, TEXT("  BlockedAbilityTags    : %s"), *Resolved->BlockingPolicy.BlockedAbilityTags.ToStringSimple());
        UE_LOG(LogKata, Log, TEXT("  StartCondition        : %s"), *GetNameSafe(Resolved->StartCondition.Get()));
        UE_LOG(LogKata, Log, TEXT("  Cooldown              : enabled=%s duration=%.3fs start=%s groups=%s"),
            Resolved->CooldownPolicy.bEnabled ? TEXT("true") : TEXT("false"),
            Resolved->CooldownPolicy.Duration,
            *StaticEnum<EKataCooldownApplyTime>()->GetNameStringByValue(static_cast<int64>(Resolved->CooldownPolicy.ApplyTime)),
            *Resolved->CooldownPolicy.GroupTags.ToStringSimple());
        UE_LOG(LogKata, Log, TEXT("  Loop                  : enabled=%s maxCount=%d"),
            Resolved->LoopPolicy.bLoop ? TEXT("true") : TEXT("false"),
            Resolved->LoopPolicy.MaxLoopCount);
        UE_LOG(LogKata, Log, TEXT("  TimelineDuration      : %.3fs"), Resolved->GetTimelineDuration());

        UE_LOG(LogKata, Log, TEXT("  Tasks (%d, in execution order):"), Resolved->Tasks.Num());
        for (int32 Index = 0; Index < Resolved->Tasks.Num(); ++Index)
        {
            const UKataTask* Task = Resolved->Tasks[Index];
            if (Task == nullptr)
            {
                continue;
            }

            const FString PhaseName = StaticEnum<EKataTaskPhase>()->GetNameStringByValue(static_cast<int64>(Task->Phase));
            UE_LOG(LogKata, Log, TEXT("    [%d] %-8s %-28s start=%.3f dur=%.3f single=%s phase=%s order=%d"),
                Index,
                *Task->GetDisplayName(),
                *Task->GetClass()->GetName(),
                Task->StartTime,
                Task->Duration,
                Task->bSingleFrame ? TEXT("yes") : TEXT("no"),
                *PhaseName,
                Task->OrderHint);

            for (const FKataTaskDependency& Dependency : Task->Dependencies)
            {
                const FString RequirementName = StaticEnum<EKataTaskDependencyRequirement>()->GetNameStringByValue(static_cast<int64>(Dependency.Requirement));
                const UKataTask* Prerequisite = Resolved->FindTask(Dependency.TaskId);
                UE_LOG(LogKata, Log, TEXT("         depends on %s (%s)"),
                    Prerequisite != nullptr ? *Prerequisite->GetDisplayName() : *Dependency.TaskId.ToString(),
                    *RequirementName);
            }
        }

        if (Resolved->Diagnostics.IsEmpty())
        {
            UE_LOG(LogKata, Log, TEXT("  Diagnostics           : none"));
        }
        else
        {
            UE_LOG(LogKata, Log, TEXT("  Diagnostics (%d):"), Resolved->Diagnostics.Num());
            Resolved->LogDiagnostics();
        }

        UE_LOG(LogKata, Log, TEXT("  Playable              : %s"), Resolved->HasErrors() ? TEXT("NO (errors present)") : TEXT("yes"));
    }
}

namespace
{
    /** 지정한 월드에서 첫 번째 테스트 액터를 찾는다. */
    AKataTestActor* FindTestActor(UWorld* World)
    {
        if (World == nullptr)
        {
            return nullptr;
        }
        for (TActorIterator<AKataTestActor> It(World); It; ++It)
        {
            return *It;
        }
        return nullptr;
    }

    FAutoConsoleCommand GKataResolveCommand(
        TEXT("Kata.Resolve"),
        TEXT("Resolve a Kata action and log its timeline and diagnostics. Usage: Kata.Resolve <BuiltInName|AssetPath>"),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            if (Args.Num() < 1)
            {
                UE_LOG(LogKata, Error, TEXT("Usage: Kata.Resolve <BuiltInName|AssetPath>"));
                return;
            }

            // 해석 결과가 태스크 사본을 따로 소유하므로 임시 액션은 출력 후 버려도 된다.
            UKataAction* Action = KataTestLogging::FindAction(Args[0], GetTransientPackage());
            if (Action == nullptr)
            {
                UE_LOG(LogKata, Error, TEXT("Kata.Resolve: could not find action '%s'"), *Args[0]);
                return;
            }
            KataTestLogging::DumpResolvedAction(Action);
        }));

    FAutoConsoleCommand GKataListCommand(
        TEXT("Kata.List"),
        TEXT("List the built-in test actions and every loaded Kata action asset."),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            UE_LOG(LogKata, Log, TEXT("=== Built-in test actions ==="));
            for (EKataTestAction Kind : KataTestActions::GetAllKinds())
            {
                UE_LOG(LogKata, Log, TEXT("  %s"), *KataTestActions::GetName(Kind));
            }

            UE_LOG(LogKata, Log, TEXT("=== Loaded Kata action assets ==="));
            for (TObjectIterator<UKataAction> It; It; ++It)
            {
                UKataAction* Action = *It;
                if (Action->HasAnyFlags(RF_ClassDefaultObject | RF_Transient))
                {
                    continue;
                }
                UE_LOG(LogKata, Log, TEXT("  %s"), *Action->GetPathName());
            }
        }));

    FAutoConsoleCommandWithWorldAndArgs GKataPlayCommand(
        TEXT("Kata.Play"),
        TEXT("Play a Kata on the first Kata Test Actor in the world. Usage: Kata.Play [BuiltInName|AssetPath]"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
        {
            AKataTestActor* TestActor = FindTestActor(World);
            if (TestActor == nullptr)
            {
                UE_LOG(LogKata, Error, TEXT("Kata.Play: no AKataTestActor found in the current world"));
                return;
            }

            if (Args.Num() >= 1)
            {
                const EKataTestAction Kind = KataTestActions::ParseName(Args[0]);
                if (Kind != EKataTestAction::None)
                {
                    TestActor->BuiltInAction = Kind;
                }
                else if (UKataAction* Asset = LoadObject<UKataAction>(nullptr, *Args[0]))
                {
                    TestActor->BuiltInAction = EKataTestAction::None;
                    TestActor->ActionToPlay = Asset;
                }
                else
                {
                    UE_LOG(LogKata, Error, TEXT("Kata.Play: could not find action '%s'"), *Args[0]);
                    return;
                }
            }

            TestActor->PlayTestKata();
        }));

    FAutoConsoleCommandWithWorld GKataStopCommand(
        TEXT("Kata.Stop"),
        TEXT("Stop the Kata running on the first Kata Test Actor in the world."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
        {
            if (AKataTestActor* TestActor = FindTestActor(World))
            {
                TestActor->StopTestKata();
                return;
            }
            UE_LOG(LogKata, Error, TEXT("Kata.Stop: no AKataTestActor found in the current world"));
        }));
}
