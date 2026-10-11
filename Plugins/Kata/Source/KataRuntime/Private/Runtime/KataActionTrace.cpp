#include "Runtime/KataActionTrace.h"

#if KATA_WITH_ACTION_TRACE

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/StringBuilder.h"
#include "Runtime/KataActionComponent.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY(LogKataActionTrace);

namespace
{
    /** 콘솔 변수 값이 비정상이어도 저장 공간이 터무니없이 커지지 않게 막는 상한. */
    constexpr int32 MaxTraceCapacity = 1024;

    TAutoConsoleVariable<bool> CVarKataActionTraceEnable(
        TEXT("Kata.ActionTrace.Enable"),
        true,
        TEXT("Record Kata action trace entries (start, rejection, end, cancel, windows, graph triggers) on each Kata Action Component. ")
        TEXT("Existing entries are kept when disabled."),
        ECVF_Default);

    TAutoConsoleVariable<int32> CVarKataActionTraceCapacity(
        TEXT("Kata.ActionTrace.Capacity"),
        64,
        TEXT("Number of Kata action trace entries kept per Kata Action Component (1-1024). The oldest entries are overwritten first."),
        ECVF_Default);

    TAutoConsoleVariable<bool> CVarKataActionTraceLogLive(
        TEXT("Kata.ActionTrace.LogLive"),
        false,
        TEXT("Also write each new Kata action trace entry to LogKataActionTrace as a key=value line. Merged repeats are not written again."),
        ECVF_Default);

    template <typename EnumType>
    FString GetEnumValueName(EnumType Value)
    {
        return StaticEnum<EnumType>()->GetNameStringByValue(static_cast<int64>(Value));
    }

    /**
     * 콘솔에서 넘어온 월드가 게임 월드가 아니면 실행 중인 게임·PIE 월드를 찾는다.
     * 에디터 Output Log에서 실행하면 편집 월드가 넘어올 수 있기 때문이다.
     */
    UWorld* ResolveTraceWorld(UWorld* InWorld)
    {
        if (InWorld != nullptr && InWorld->IsGameWorld())
        {
            return InWorld;
        }
        if (GEngine != nullptr)
        {
            for (const FWorldContext& Context : GEngine->GetWorldContexts())
            {
                UWorld* World = Context.World();
                if (World != nullptr && World->IsGameWorld())
                {
                    return World;
                }
            }
        }
        return InWorld;
    }

    bool MatchesActorName(const AActor& Actor, const FString& Filter)
    {
        if (Actor.GetName().Contains(Filter))
        {
            return true;
        }
#if WITH_EDITOR
        return Actor.GetActorLabel().Contains(Filter);
#else
        return false;
#endif
    }

    /**
     * Kata.ActionTrace.Dump [All|ActorNameSubstring] [Count].
     *
     * 대상이 없으면 로컬 플레이어 Pawn을 쓴다. 코어는 KataTargeting에 의존하지 않으므로 치트의 대상 선택자를 쓰지 않는다.
     * 숫자 인자는 소유자마다 최근 몇 개를 출력할지 정하며 0이나 생략은 전부다.
     */
    void DumpActionTrace(const TArray<FString>& Args, UWorld* InWorld)
    {
        UWorld* World = ResolveTraceWorld(InWorld);
        if (World == nullptr)
        {
            UE_LOG(LogKataActionTrace, Warning, TEXT("Kata.ActionTrace.Dump: no world to read."));
            return;
        }

        FString TargetFilter;
        int32 MaxCount = 0;
        for (const FString& Arg : Args)
        {
            if (Arg.IsNumeric())
            {
                MaxCount = FMath::Max(0, FCString::Atoi(*Arg));
            }
            else if (TargetFilter.IsEmpty())
            {
                TargetFilter = Arg;
            }
        }

        const bool bAll = TargetFilter.Equals(TEXT("All"), ESearchCase::IgnoreCase);
        TArray<UKataActionComponent*> Components;
        if (TargetFilter.IsEmpty())
        {
            const APlayerController* PlayerController = World->GetFirstPlayerController();
            const APawn* Pawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
            if (UKataActionComponent* Component = Pawn != nullptr ? Pawn->FindComponentByClass<UKataActionComponent>() : nullptr)
            {
                Components.Add(Component);
            }
        }
        else
        {
            for (TObjectIterator<UKataActionComponent> It; It; ++It)
            {
                UKataActionComponent* Component = *It;
                if (!IsValid(Component) || Component->IsTemplate() || Component->GetWorld() != World)
                {
                    continue;
                }
                const AActor* Owner = Component->GetOwner();
                if (Owner != nullptr && (bAll || MatchesActorName(*Owner, TargetFilter)))
                {
                    Components.Add(Component);
                }
            }
        }

        if (Components.IsEmpty())
        {
            UE_LOG(LogKataActionTrace, Warning,
                TEXT("Kata.ActionTrace.Dump: no Kata Action Component found for '%s'. Usage: Kata.ActionTrace.Dump [All|ActorNameSubstring] [Count]"),
                TargetFilter.IsEmpty() ? TEXT("local player pawn") : *TargetFilter);
            return;
        }

        TArray<FKataActionTraceEntry> Entries;
        for (UKataActionComponent* Component : Components)
        {
            Component->GetTraceEntries(Entries);
            // All은 액션을 한 번도 실행하지 않은 소유자가 많으므로 기록이 없는 소유자는 건너뛴다.
            if (bAll && Entries.IsEmpty())
            {
                continue;
            }

            const AActor* Owner = Component->GetOwner();
            const int32 FirstIndex = MaxCount > 0 ? FMath::Max(0, Entries.Num() - MaxCount) : 0;
            UE_LOG(LogKataActionTrace, Display, TEXT("trace actor=%s entries=%d shown=%d enabled=%d"),
                *GetNameSafe(Owner), Entries.Num(), Entries.Num() - FirstIndex, KataActionTrace::IsEnabled() ? 1 : 0);
            for (int32 Index = FirstIndex; Index < Entries.Num(); ++Index)
            {
                UE_LOG(LogKataActionTrace, Display, TEXT("%s"), *KataActionTrace::FormatEntry(Entries[Index], Owner));
            }
        }
    }

    FAutoConsoleCommandWithWorldAndArgs GKataActionTraceDumpCommand(
        TEXT("Kata.ActionTrace.Dump"),
        TEXT("Print Kata action trace entries to LogKataActionTrace as key=value lines. ")
        TEXT("Usage: Kata.ActionTrace.Dump [All|ActorNameSubstring] [Count]. Without a target, uses the local player pawn."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DumpActionTrace));
}

void FKataActionTraceEntry::SetAsset(const UObject* InAsset)
{
    Asset = InAsset;
    AssetName = InAsset != nullptr ? InAsset->GetFName() : NAME_None;
}

void FKataActionTraceEntry::SetConditionResult(const FKataConditionResult& Result)
{
    bHasConditionResult = true;
    ConditionStatus = Result.Status;
    ConditionReason = Result.Reason;
}

bool FKataActionTraceEntry::HasSameContent(const FKataActionTraceEntry& Other) const
{
    return Event == Other.Event
        && Source == Other.Source
        && InstanceId == Other.InstanceId
        && AssetName == Other.AssetName
        && NodeName == Other.NodeName
        && Tag == Other.Tag
        && Detail == Other.Detail
        && StartResult == Other.StartResult
        && EndReason == Other.EndReason
        && bHasConditionResult == Other.bHasConditionResult
        && ConditionStatus == Other.ConditionStatus
        && ConditionReason == Other.ConditionReason;
}

bool FKataActionTraceBuffer::Add(FKataActionTraceEntry& InOutEntry, int32 InCapacity)
{
    if (InCapacity != Capacity)
    {
        Resize(InCapacity);
    }
    ++Serial;

    // AI Task처럼 받아들여질 때까지 매 프레임 같은 요청을 보내는 호출자가 있다. 같은 내용은 한 항목으로 합쳐 보관 개수를 지킨다.
    if (!Entries.IsEmpty())
    {
        FKataActionTraceEntry& Last = Entries[(Head + Entries.Num() - 1) % Entries.Num()];
        if (Last.HasSameContent(InOutEntry))
        {
            Last.RepeatCount = Last.RepeatCount < MAX_uint16 ? Last.RepeatCount + 1 : MAX_uint16;
            Last.WorldSeconds = InOutEntry.WorldSeconds;
            Last.FrameNumber = InOutEntry.FrameNumber;
            return false;
        }
    }

    InOutEntry.Sequence = NextSequence++;
    InOutEntry.RepeatCount = 1;
    if (Entries.Num() < Capacity)
    {
        Entries.Add(InOutEntry);
    }
    else
    {
        Entries[Head] = InOutEntry;
        Head = (Head + 1) % Capacity;
    }
    return true;
}

void FKataActionTraceBuffer::CopyEntries(TArray<FKataActionTraceEntry>& OutEntries) const
{
    OutEntries.Reset(Entries.Num());
    for (int32 Offset = 0; Offset < Entries.Num(); ++Offset)
    {
        OutEntries.Add(Entries[(Head + Offset) % Entries.Num()]);
    }
}

void FKataActionTraceBuffer::Reset()
{
    Entries.Empty();
    Capacity = 0;
    Head = 0;
    ++Serial;
}

void FKataActionTraceBuffer::Resize(int32 NewCapacity)
{
    NewCapacity = FMath::Clamp(NewCapacity, 1, MaxTraceCapacity);

    // 오래된 것부터 줄 세운 뒤 최근 기록만 남긴다. 줄 세운 결과는 가득 차 있어도 가장 오래된 기록이 0번이다.
    TArray<FKataActionTraceEntry> Ordered;
    CopyEntries(Ordered);
    const int32 KeepCount = FMath::Min(Ordered.Num(), NewCapacity);

    Entries.Empty(NewCapacity);
    Entries.Append(Ordered.GetData() + (Ordered.Num() - KeepCount), KeepCount);
    Capacity = NewCapacity;
    Head = 0;
}

namespace KataActionTrace
{
    bool IsEnabled()
    {
        return CVarKataActionTraceEnable.GetValueOnGameThread();
    }

    int32 GetCapacity()
    {
        return FMath::Clamp(CVarKataActionTraceCapacity.GetValueOnGameThread(), 1, MaxTraceCapacity);
    }

    bool IsLiveLogEnabled()
    {
        return CVarKataActionTraceLogLive.GetValueOnGameThread();
    }

    const TCHAR* GetEventName(EKataActionTraceEvent Event)
    {
        switch (Event)
        {
        case EKataActionTraceEvent::ActionStarted: return TEXT("ActionStarted");
        case EKataActionTraceEvent::ActionRejected: return TEXT("ActionRejected");
        case EKataActionTraceEvent::ActionEnded: return TEXT("ActionEnded");
        case EKataActionTraceEvent::CancelRequested: return TEXT("CancelRequested");
        case EKataActionTraceEvent::TransitionWindowOpened: return TEXT("TransitionWindowOpened");
        case EKataActionTraceEvent::TransitionWindowClosed: return TEXT("TransitionWindowClosed");
        case EKataActionTraceEvent::CancelWindowOpened: return TEXT("CancelWindowOpened");
        case EKataActionTraceEvent::CancelWindowClosed: return TEXT("CancelWindowClosed");
        case EKataActionTraceEvent::GraphStarted: return TEXT("GraphStarted");
        case EKataActionTraceEvent::GraphEnded: return TEXT("GraphEnded");
        case EKataActionTraceEvent::TriggerTransition: return TEXT("TriggerTransition");
        case EKataActionTraceEvent::TriggerReserved: return TEXT("TriggerReserved");
        case EKataActionTraceEvent::TriggerBuffered: return TEXT("TriggerBuffered");
        case EKataActionTraceEvent::TriggerUnmatched: return TEXT("TriggerUnmatched");
        case EKataActionTraceEvent::TriggerBufferUnmatched: return TEXT("TriggerBufferUnmatched");
        case EKataActionTraceEvent::TriggerIgnored: return TEXT("TriggerIgnored");
        }
        return TEXT("Unknown");
    }

    const TCHAR* GetSourceName(EKataActionTraceSource Source)
    {
        switch (Source)
        {
        case EKataActionTraceSource::None: return TEXT("None");
        case EKataActionTraceSource::Direct: return TEXT("Direct");
        case EKataActionTraceSource::Ability: return TEXT("Ability");
        case EKataActionTraceSource::Transition: return TEXT("Transition");
        }
        return TEXT("Unknown");
    }

    FString FormatEntry(const FKataActionTraceEntry& Entry, const AActor* Owner)
    {
        TStringBuilder<256> Builder;
        Builder.Appendf(TEXT("seq=%u t=%.3f f=%llu actor=%s ev=%s"),
            Entry.Sequence, Entry.WorldSeconds, static_cast<unsigned long long>(Entry.FrameNumber),
            *GetNameSafe(Owner), GetEventName(Entry.Event));

        if (Entry.Source != EKataActionTraceSource::None)
        {
            Builder.Appendf(TEXT(" src=%s"), GetSourceName(Entry.Source));
        }
        if (Entry.InstanceId != 0)
        {
            Builder.Appendf(TEXT(" id=%u"), Entry.InstanceId);
        }
        if (!Entry.AssetName.IsNone())
        {
            Builder.Appendf(TEXT(" asset=%s"), *Entry.AssetName.ToString());
        }
        if (!Entry.NodeName.IsNone())
        {
            Builder.Appendf(TEXT(" node=%s"), *Entry.NodeName.ToString());
        }
        if (Entry.Tag.IsValid())
        {
            Builder.Appendf(TEXT(" tag=%s"), *Entry.Tag.ToString());
        }
        if (Entry.Event == EKataActionTraceEvent::ActionRejected)
        {
            Builder.Appendf(TEXT(" result=%s"), *GetEnumValueName(Entry.StartResult));
        }
        if (Entry.Event == EKataActionTraceEvent::ActionEnded
            || Entry.Event == EKataActionTraceEvent::GraphEnded
            || Entry.Event == EKataActionTraceEvent::CancelRequested)
        {
            Builder.Appendf(TEXT(" end=%s"), *GetEnumValueName(Entry.EndReason));
        }
        if (!Entry.Detail.IsNone())
        {
            Builder.Appendf(TEXT(" detail=%s"), *Entry.Detail.ToString());
        }
        if (Entry.bHasConditionResult)
        {
            Builder.Appendf(TEXT(" cond=%s"), *GetEnumValueName(Entry.ConditionStatus));
            if (!Entry.ConditionReason.IsNone())
            {
                Builder.Appendf(TEXT(" reason=%s"), *Entry.ConditionReason.ToString());
            }
        }
        if (Entry.RepeatCount > 1)
        {
            Builder.Appendf(TEXT(" rep=%u"), static_cast<uint32>(Entry.RepeatCount));
        }
        return FString(Builder.ToView());
    }
}

#endif // KATA_WITH_ACTION_TRACE
