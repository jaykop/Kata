#include "KataRuntimeSpawnerLoop.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Action/KataAction.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KataGraph.h"
#include "KataGraphComponent.h"
#include "KataGraphInstance.h"
#include "KataRuntimeSpawnerLog.h"
#include "Runtime/KataActionComponent.h"
#include "Runtime/KataActionInstance.h"

FKataRuntimeSpawnerLoop::~FKataRuntimeSpawnerLoop()
{
    Reset();
}

void FKataRuntimeSpawnerLoop::AddAction(APawn* Pawn, UKataAction* Action, float Interval)
{
    if (Pawn == nullptr || Action == nullptr)
    {
        return;
    }
    FEntry Entry;
    Entry.Pawn = Pawn;
    Entry.Action.Reset(Action);
    Entry.Interval = Interval;
    AddEntry(MoveTemp(Entry));
}

void FKataRuntimeSpawnerLoop::AddGraph(APawn* Pawn, UKataGraph* Graph, FGameplayTag EntryTrigger, float Interval)
{
    if (Pawn == nullptr || Graph == nullptr)
    {
        return;
    }
    FEntry Entry;
    Entry.Pawn = Pawn;
    Entry.Graph.Reset(Graph);
    Entry.EntryTrigger = EntryTrigger;
    Entry.Interval = Interval;
    AddEntry(MoveTemp(Entry));
}

void FKataRuntimeSpawnerLoop::AddEntry(FEntry&& Entry)
{
    Entry.Interval = FMath::Max(Entry.Interval, 0.f);
    // 첫 실행은 간격을 기다리지 않는다.
    Entry.IdleTime = Entry.Interval;
    Entries.Add(MoveTemp(Entry));
    if (!TickHandle.IsValid())
    {
        TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FKataRuntimeSpawnerLoop::Tick));
    }
}

void FKataRuntimeSpawnerLoop::Reset()
{
    Entries.Reset();
    if (TickHandle.IsValid())
    {
        FTSTicker::RemoveTicker(TickHandle);
        TickHandle.Reset();
    }
}

bool FKataRuntimeSpawnerLoop::Tick(float DeltaTime)
{
    for (int32 Index = Entries.Num() - 1; Index >= 0; --Index)
    {
        FEntry& Entry = Entries[Index];
        APawn* Pawn = Entry.Pawn.Get();
        if (!IsValid(Pawn))
        {
            Entries.RemoveAtSwap(Index);
            continue;
        }
        // 일시 정지 중에는 실행이 진행되지 않으므로 쉬는 시간도 세지 않는다.
        const UWorld* World = Pawn->GetWorld();
        if (World == nullptr || World->IsPaused())
        {
            continue;
        }
        // ASC가 아직 이 Pawn에 연결되지 않았으면 준비될 때까지 기다린다.
        const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(Pawn);
        const UAbilitySystemComponent* ASC = AbilityOwner != nullptr ? AbilityOwner->GetAbilitySystemComponent() : nullptr;
        if (ASC == nullptr || ASC->GetAvatarActor() != Pawn)
        {
            continue;
        }
        const UKataActionComponent* ActionComponent = Pawn->FindComponentByClass<UKataActionComponent>();
        const UKataGraphComponent* GraphComponent = Pawn->FindComponentByClass<UKataGraphComponent>();
        const bool bBusy = (ActionComponent != nullptr && ActionComponent->IsPlayingKata())
            || (GraphComponent != nullptr && GraphComponent->IsRunningGraph());
        if (bBusy)
        {
            Entry.IdleTime = 0.f;
            continue;
        }
        Entry.IdleTime += DeltaTime;
        if (Entry.IdleTime < Entry.Interval)
        {
            continue;
        }
        Entry.IdleTime = 0.f;
        if (!PlayOnce(Entry))
        {
            Entries.RemoveAtSwap(Index);
        }
    }
    if (Entries.IsEmpty())
    {
        TickHandle.Reset();
        return false;
    }
    return true;
}

bool FKataRuntimeSpawnerLoop::PlayOnce(FEntry& Entry)
{
    APawn* Pawn = Entry.Pawn.Get();
    const APlayerController* PlayerController = Pawn->GetWorld()->GetFirstPlayerController();
    AActor* Target = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;

    if (UKataAction* Action = Entry.Action.Get())
    {
        UKataActionComponent* ActionComponent = Pawn->FindComponentByClass<UKataActionComponent>();
        if (ActionComponent == nullptr)
        {
            UE_LOG(LogKataRuntimeSpawner, Warning, TEXT("Repeat stopped: %s has no Kata Action Component."), *Pawn->GetName());
            return false;
        }
        UKataActionInstance* Instance = nullptr;
        const EKataStartResult Result = ActionComponent->PlayKataActionOnSelf(Action, Target, Instance);
        // 조건·비용 거절은 다음 간격에 다시 시도한다.
        UE_CLOG(Result != EKataStartResult::Started, LogKataRuntimeSpawner, Verbose, TEXT("Repeat action '%s' rejected on %s: %s"),
            *Action->GetName(), *Pawn->GetName(), *UEnum::GetValueAsString(Result));
        return true;
    }

    UKataGraph* Graph = Entry.Graph.Get();
    UKataGraphComponent* GraphComponent = Pawn->FindComponentByClass<UKataGraphComponent>();
    if (Graph == nullptr || GraphComponent == nullptr)
    {
        UE_LOG(LogKataRuntimeSpawner, Warning, TEXT("Repeat stopped: %s has no Kata Graph Component."), *Pawn->GetName());
        return false;
    }
    UKataGraphInstance* Instance = nullptr;
    if (!GraphComponent->StartGraphOnSelf(Graph, Target, Instance) || !IsValid(Instance))
    {
        return true;
    }
    if (Instance->GetState() == EKataGraphInstanceState::WaitingForEntry && Entry.EntryTrigger.IsValid())
    {
        Instance->SendTrigger(Entry.EntryTrigger);
    }
    // 진입 대기에 머물면 그래프가 계속 실행 중으로 남아 반복이 멈추므로 정지시키고 원인을 한 번만 알린다.
    if (Instance->GetState() == EKataGraphInstanceState::WaitingForEntry)
    {
        GraphComponent->StopGraph(EKataEndReason::Cancelled);
        if (!Entry.bWarnedEntry)
        {
            Entry.bWarnedEntry = true;
            UE_LOG(LogKataRuntimeSpawner, Warning, TEXT("Graph '%s' on %s is waiting for an entry trigger. Set an Entry Trigger that the graph accepts (current: %s)."),
                *Graph->GetName(), *Pawn->GetName(), *Entry.EntryTrigger.ToString());
        }
    }
    return true;
}
