// Copyright Epic Games, Inc. All Rights Reserved.

#include "Testing/KataDebugTask.h"

#include "Engine/Engine.h"
#include "KataRuntimeLog.h"
#include "Runtime/KataActionInstance.h"

UKataTask_Debug::UKataTask_Debug()
{
    Phase = EKataTaskPhase::Gameplay;
}

TSubclassOf<UKataTaskInstance> UKataTask_Debug::GetTaskInstanceClass_Implementation() const
{
    return UKataTaskInstance_Debug::StaticClass();
}

void UKataTaskInstance_Debug::Report(const TCHAR* Event, const FString& Detail) const
{
    const UKataActionInstance* Instance = GetActionInstance();
    const float KataTime = Instance != nullptr ? Instance->GetCurrentTime() : 0.0f;
    const int32 LoopIteration = Instance != nullptr ? Instance->GetLoopIteration() : 0;

    const FString Message = FString::Printf(TEXT("[Kata t=%.3f loop=%d] %-5s %s%s"),
        KataTime, LoopIteration, Event, *GetDisplayName(), *Detail);

    UE_LOG(LogKata, Log, TEXT("%s"), *Message);

    if (GEngine != nullptr)
    {
        const UKataTask_Debug* Definition = Cast<UKataTask_Debug>(GetTaskDefinition());
        const FColor Color = Definition != nullptr ? Definition->DisplayColor : FColor::Cyan;
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, Color, Message);
    }
}

void UKataTaskInstance_Debug::OnTaskStarted_Implementation()
{
    ElapsedSeconds = 0.0f;
    Report(TEXT("START"), FString());
}

void UKataTaskInstance_Debug::OnTaskTick_Implementation(float DeltaTime)
{
    ElapsedSeconds += DeltaTime;

    const UKataTask_Debug* Definition = Cast<UKataTask_Debug>(GetTaskDefinition());
    if (Definition == nullptr)
    {
        return;
    }

    if (Definition->bLogTick)
    {
        Report(TEXT("TICK"), FString::Printf(TEXT(" elapsed=%.3f"), ElapsedSeconds));
    }

    if (Definition->bFinishEarly && ElapsedSeconds >= Definition->FinishAfterSeconds)
    {
        // 지속 시간보다 먼저 끝나는 경로를 확인한다.
        Report(TEXT("EARLY"), FString::Printf(TEXT(" elapsed=%.3f"), ElapsedSeconds));
        FinishTask();
    }
}

void UKataTaskInstance_Debug::OnTaskEnded_Implementation(EKataTaskEndReason Reason)
{
    const FString ReasonName = StaticEnum<EKataTaskEndReason>()->GetNameStringByValue(static_cast<int64>(Reason));
    Report(TEXT("END"), FString::Printf(TEXT(" reason=%s"), *ReasonName));
}
