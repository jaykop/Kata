#pragma once

#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/StrongObjectPtr.h"

class APawn;
class UKataAction;
class UKataGraph;

/**
 * 런타임 스포너 도구가 만든 NPC에 Action 또는 Graph를 반복 실행시킨다.
 * NPC의 AI는 꺼져 있다고 가정하며, 대상은 실행할 때마다 PIE 월드의 첫 플레이어 Pawn으로 다시 고른다.
 * NPC가 쉬는 상태(Action·Graph 모두 실행 중이 아님)로 반복 간격 이상 지나면 다시 시작한다. 첫 실행은 준비되는 즉시 한다.
 * NPC는 약한 참조로 보관해 파괴되면 기록을 지우고, 실행할 에셋은 반복하는 동안 강한 참조로 유지한다.
 */
class FKataRuntimeSpawnerLoop
{
public:
    FKataRuntimeSpawnerLoop() = default;
    ~FKataRuntimeSpawnerLoop();

    /** Action 반복을 등록한다. Action이 null이면 무시한다. */
    void AddAction(APawn* Pawn, UKataAction* Action, float Interval);

    /** Graph 반복을 등록한다. 자동 진입이 없는 Graph는 EntryTrigger로 시작하며, 트리거가 없거나 받아들이지 않으면 경고 후 정지한다. */
    void AddGraph(APawn* Pawn, UKataGraph* Graph, FGameplayTag EntryTrigger, float Interval);

    /** 모든 반복을 멈추고 기록을 비운다. 이미 실행 중인 Action·Graph는 중단하지 않는다. */
    void Reset();

    int32 Num() const { return Entries.Num(); }

private:
    struct FEntry
    {
        TWeakObjectPtr<APawn> Pawn;
        TStrongObjectPtr<UKataAction> Action;
        TStrongObjectPtr<UKataGraph> Graph;
        FGameplayTag EntryTrigger;
        float Interval = 0.f;
        float IdleTime = 0.f;
        bool bWarnedEntry = false;
    };

    void AddEntry(FEntry&& Entry);
    bool Tick(float DeltaTime);

    /** 반환값이 false면 다시 시도하지 않을 실패라서 기록을 지운다. */
    bool PlayOnce(FEntry& Entry);

    TArray<FEntry> Entries;
    FTSTicker::FDelegateHandle TickHandle;
};
