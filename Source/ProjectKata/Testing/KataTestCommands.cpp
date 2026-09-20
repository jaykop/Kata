// Copyright Epic Games, Inc. All Rights Reserved.

#include "Testing/KataTestLogging.h"

#include "Definition/KataDefinition.h"
#include "Definition/KataResolvedDefinition.h"
#include "Definition/KataTask.h"
#include "EngineUtils.h"
#include "GameplayEffect.h"
#include "HAL/IConsoleManager.h"
#include "KataCondition.h"
#include "KataRuntimeLog.h"
#include "Runtime/KataComponent.h"
#include "Runtime/KataInstance.h"
#include "Testing/KataTestActor.h"
#include "UObject/Class.h"
#include "UObject/UObjectIterator.h"

namespace KataTestLogging
{
    UClass* FindDefinitionClass(const FString& ClassNameOrPath)
    {
        if (ClassNameOrPath.IsEmpty())
        {
            return nullptr;
        }

        // C++ 클래스의 짧은 이름이나 /Script/ 경로.
        if (UClass* Found = UClass::TryFindTypeSlow<UClass>(ClassNameOrPath))
        {
            if (Found->IsChildOf(UKataDefinition::StaticClass()))
            {
                return Found;
            }
        }

        // Blueprint 에셋 경로.
        if (UClass* Loaded = LoadClass<UKataDefinition>(nullptr, *ClassNameOrPath))
        {
            return Loaded;
        }
        if (!ClassNameOrPath.EndsWith(TEXT("_C")))
        {
            const FString WithSuffix = ClassNameOrPath + TEXT("_C");
            if (UClass* LoadedWithSuffix = LoadClass<UKataDefinition>(nullptr, *WithSuffix))
            {
                return LoadedWithSuffix;
            }
        }
        return nullptr;
    }

    void DumpResolvedDefinition(TSubclassOf<UKataDefinition> DefinitionClass)
    {
        if (DefinitionClass == nullptr)
        {
            UE_LOG(LogKata, Error, TEXT("Kata.Resolve: no definition class"));
            return;
        }

        UKataResolvedDefinition* Resolved = UKataDefinition::ResolveDefinition(DefinitionClass, GetTransientPackage());
        if (Resolved == nullptr)
        {
            UE_LOG(LogKata, Error, TEXT("Kata.Resolve: failed to resolve '%s'"), *DefinitionClass->GetName());
            return;
        }

        UE_LOG(LogKata, Log, TEXT("=== Resolved Kata: %s ==="), *DefinitionClass->GetName());
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
            UE_LOG(LogKata, Log, TEXT("    [%d] %-8s %-28s start=%.3f dur=%.3f phase=%s order=%d"),
                Index,
                *Task->GetDisplayName(),
                *Task->GetClass()->GetName(),
                Task->StartTime,
                Task->Duration,
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
        TEXT("Resolve a Kata definition class and log its timeline and diagnostics. Usage: Kata.Resolve <ClassName|AssetPath>"),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            if (Args.Num() < 1)
            {
                UE_LOG(LogKata, Error, TEXT("Usage: Kata.Resolve <ClassName|AssetPath>"));
                return;
            }

            UClass* DefinitionClass = KataTestLogging::FindDefinitionClass(Args[0]);
            if (DefinitionClass == nullptr)
            {
                UE_LOG(LogKata, Error, TEXT("Kata.Resolve: could not find definition class '%s'"), *Args[0]);
                return;
            }
            KataTestLogging::DumpResolvedDefinition(DefinitionClass);
        }));

    FAutoConsoleCommand GKataListCommand(
        TEXT("Kata.List"),
        TEXT("List every loaded Kata definition class."),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            UE_LOG(LogKata, Log, TEXT("=== Kata definition classes ==="));
            for (TObjectIterator<UClass> It; It; ++It)
            {
                UClass* Class = *It;
                if (!Class->IsChildOf(UKataDefinition::StaticClass()) || Class->HasAnyClassFlags(CLASS_Abstract))
                {
                    continue;
                }
                UE_LOG(LogKata, Log, TEXT("  %s"), *Class->GetPathName());
            }
        }));

    FAutoConsoleCommandWithWorldAndArgs GKataPlayCommand(
        TEXT("Kata.Play"),
        TEXT("Play a Kata on the first Kata Test Actor in the world. Usage: Kata.Play [ClassName|AssetPath]"),
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
                UClass* DefinitionClass = KataTestLogging::FindDefinitionClass(Args[0]);
                if (DefinitionClass == nullptr)
                {
                    UE_LOG(LogKata, Error, TEXT("Kata.Play: could not find definition class '%s'"), *Args[0]);
                    return;
                }
                TestActor->DefinitionToPlay = DefinitionClass;
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
