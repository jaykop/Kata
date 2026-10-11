#include "Debug/GameplayDebuggerCategory_KataAction.h"

#if WITH_GAMEPLAY_DEBUGGER && KATA_WITH_ACTION_TRACE

#include "Action/KataAction.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/StringBuilder.h"
#include "Runtime/KataActionComponent.h"
#include "Runtime/KataActionInstance.h"

namespace
{
    /** 화면에 보일 최근 기록 수. 다른 카테고리를 덮지 않을 만큼만 보이고 전체는 Kata.ActionTrace.Dump로 본다. */
    constexpr int32 MaxDisplayedTraceEntries = 12;

    /** 원인을 찾을 때 먼저 볼 기록(거절, 불일치, 끊긴 종료)이 눈에 띄도록 색을 고른다. */
    const TCHAR* GetTraceEntryColor(const FKataActionTraceEntry& Entry)
    {
        switch (Entry.Event)
        {
        case EKataActionTraceEvent::ActionRejected:
        case EKataActionTraceEvent::TriggerUnmatched:
        case EKataActionTraceEvent::TriggerBufferUnmatched:
        case EKataActionTraceEvent::TriggerIgnored:
            return TEXT("{red}");
        case EKataActionTraceEvent::ActionEnded:
        case EKataActionTraceEvent::GraphEnded:
            return Entry.EndReason == EKataEndReason::Completed || Entry.EndReason == EKataEndReason::Branched
                ? TEXT("{white}") : TEXT("{orange}");
        case EKataActionTraceEvent::CancelRequested:
            return TEXT("{orange}");
        case EKataActionTraceEvent::ActionStarted:
        case EKataActionTraceEvent::TriggerTransition:
        case EKataActionTraceEvent::TriggerReserved:
            return TEXT("{green}");
        case EKataActionTraceEvent::TriggerBuffered:
            return TEXT("{yellow}");
        default:
            return TEXT("{grey}");
        }
    }

    /** 화면용 짧은 한 줄. 로그용 KataActionTrace::FormatEntry와 달리 시각을 지금부터 거슬러 간 초로 보인다. */
    FString FormatDisplayLine(const FKataActionTraceEntry& Entry, double NowSeconds)
    {
        TStringBuilder<192> Builder;
        Builder.Appendf(TEXT("%s-%.2fs  %s"), GetTraceEntryColor(Entry), NowSeconds - Entry.WorldSeconds,
            KataActionTrace::GetEventName(Entry.Event));

        if (!Entry.AssetName.IsNone())
        {
            Builder.Appendf(TEXT("  %s"), *Entry.AssetName.ToString());
        }
        if (!Entry.NodeName.IsNone())
        {
            Builder.Appendf(TEXT("  -> %s"), *Entry.NodeName.ToString());
        }
        if (Entry.Tag.IsValid())
        {
            Builder.Appendf(TEXT("  [%s]"), *Entry.Tag.ToString());
        }
        if (Entry.Event == EKataActionTraceEvent::ActionRejected)
        {
            Builder.Appendf(TEXT("  %s"), *StaticEnum<EKataStartResult>()->GetNameStringByValue(static_cast<int64>(Entry.StartResult)));
        }
        if (Entry.Event == EKataActionTraceEvent::ActionEnded
            || Entry.Event == EKataActionTraceEvent::GraphEnded
            || Entry.Event == EKataActionTraceEvent::CancelRequested)
        {
            Builder.Appendf(TEXT("  %s"), *StaticEnum<EKataEndReason>()->GetNameStringByValue(static_cast<int64>(Entry.EndReason)));
        }
        if (!Entry.Detail.IsNone())
        {
            Builder.Appendf(TEXT("  (%s)"), *Entry.Detail.ToString());
        }
        if (Entry.bHasConditionResult)
        {
            Builder.Appendf(TEXT("  cond %s"),
                *StaticEnum<EKataConditionStatus>()->GetNameStringByValue(static_cast<int64>(Entry.ConditionStatus)));
            if (!Entry.ConditionReason.IsNone())
            {
                Builder.Appendf(TEXT(" %s"), *Entry.ConditionReason.ToString());
            }
        }
        if (Entry.RepeatCount > 1)
        {
            Builder.Appendf(TEXT("  x%u"), static_cast<uint32>(Entry.RepeatCount));
        }
        return FString(Builder.ToView());
    }
}

FGameplayDebuggerCategory_KataAction::FGameplayDebuggerCategory_KataAction()
{
    // 디버그 대상을 고르지 않아도 플레이어 Pawn의 기록을 보인다.
    bShowOnlyWithDebugActor = false;
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_KataAction::MakeInstance()
{
    return MakeShareable(new FGameplayDebuggerCategory_KataAction());
}

void FGameplayDebuggerCategory_KataAction::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
    AActor* TargetActor = DebugActor;
    if (TargetActor == nullptr && OwnerPC != nullptr)
    {
        TargetActor = OwnerPC->GetPawn();
    }

    const UKataActionComponent* Component =
        TargetActor != nullptr ? TargetActor->FindComponentByClass<UKataActionComponent>() : nullptr;
    if (Component == nullptr)
    {
        AddTextLine(FString::Printf(TEXT("{white}Actor: {yellow}%s  {grey}no Kata Action Component"), *GetNameSafe(TargetActor)));
        return;
    }

    AddTextLine(FString::Printf(TEXT("{white}Actor: {yellow}%s"), *GetNameSafe(TargetActor)));
    const UKataActionInstance* Instance = Component->GetActiveInstance();
    if (Instance != nullptr && Instance->IsRunning())
    {
        AddTextLine(FString::Printf(TEXT("{white}Action: {green}%s  {white}t={yellow}%.2f / %.2f  {white}loop {yellow}%d"),
            *GetNameSafe(Instance->GetKataAction()), Instance->GetCurrentTime(), Instance->GetTimelineDuration(),
            Instance->GetLoopIteration()));
    }
    else
    {
        AddTextLine(TEXT("{white}Action: {grey}none"));
    }

    Component->GetTraceEntries(CollectedEntries);
    AddTextLine(FString::Printf(TEXT("{white}Trace: {yellow}%d {white}entries%s  {grey}(Kata.ActionTrace.Dump prints all)"),
        CollectedEntries.Num(), KataActionTrace::IsEnabled() ? TEXT("") : TEXT("  {orange}recording off (Kata.ActionTrace.Enable 0)")));

    const UWorld* World = TargetActor->GetWorld();
    const double NowSeconds = World != nullptr ? World->GetTimeSeconds() : 0.0;
    for (int32 Index = FMath::Max(0, CollectedEntries.Num() - MaxDisplayedTraceEntries); Index < CollectedEntries.Num(); ++Index)
    {
        AddTextLine(FormatDisplayLine(CollectedEntries[Index], NowSeconds));
    }
}

#endif // WITH_GAMEPLAY_DEBUGGER && KATA_WITH_ACTION_TRACE
